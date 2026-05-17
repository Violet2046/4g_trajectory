/**
 * @file    main.c
 * @brief   4G Trajectory Logger — state machine with low-power idle mode.
 *
 * Architecture:
 *   - sensor_hub  : manages shared I2C bus, BMI160 (any-motion INT1), AK09911C
 *   - CT511N      : 4G/GPS UART module (sleep via DTR)
 *   - W25Q64      : SPI NOR Flash (storage manager)
 *   - gptimer     : 1 s periodic sample trigger
 *   - sys_state_t : main loop state machine
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "driver/uart.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "CT511N.h"
#include "sensor_hub.h"
#include "storage_mgr.h"
#include "app_utils.h"

/* ======================================================================== */
/*  Pin Definitions                                                         */
/* ======================================================================== */
#define CT_UART_PORT           UART_NUM_1
#define CT_UART_TX_PIN         5
#define CT_UART_RX_PIN         6
#define CT_UART_BAUD           115200
#define CT_DTR_PIN             10

#define I2C_SDA                7
#define I2C_SCL                8
#define BMI160_INT1_PIN        9

#define GPTIMER_RESOLUTION_HZ  1000000

#define CT_WAKE_DELAY_MS       500
#define INACTIVITY_TIMEOUT_MS  5000
#define IDLE_POLL_MS           50
#define LP_POLL_MS             200

/* ======================================================================== */
/*  System State                                                            */
/* ======================================================================== */
typedef enum {
	STATE_INIT = 0,
	STATE_ACTIVE,
	STATE_LOW_POWER,
	STATE_SAMPLE,
} sys_state_t;

static const char *TAG = "main";

/* ---- Handles ---- */
static sensor_hub_t      *g_hub    = NULL;
static ct511n_handle_t   *g_ct511n = NULL;
static gptimer_handle_t   g_timer  = NULL;

/* ---- ISR flags ---- */
static volatile bool g_sample_flag = false;
static volatile bool g_motion_flag = false;
static SemaphoreHandle_t g_motion_sem = NULL;

/* ---- State tracking ---- */
static sys_state_t g_state          = STATE_INIT;
static int64_t     g_last_motion_us = 0;
static uint32_t    g_sample_count   = 0;

/* ======================================================================== */
/*  ISR Handlers                                                             */
/* ======================================================================== */

static bool IRAM_ATTR timer_on_alarm(gptimer_handle_t timer,
				     const gptimer_alarm_event_data_t *edata,
				     void *user_ctx)
{
	g_sample_flag = true;
	return true;
}

static void IRAM_ATTR bmi160_motion_isr(void *arg)
{
	g_motion_flag = true;
	g_last_motion_us = esp_timer_get_time();
	BaseType_t wake = pdFALSE;
	xSemaphoreGiveFromISR(g_motion_sem, &wake);
	portYIELD_FROM_ISR(wake);
}

/* ======================================================================== */
/*  Hardware Init                                                            */
/* ======================================================================== */
static esp_err_t hardware_init(void)
{
	esp_err_t err;

	/* Power-up delay for external I2C devices */
	vTaskDelay(pdMS_TO_TICKS(500));

	/* ---- CT511N UART ---- */
	ct511n_uart_config_t ct_cfg = {
		.uart_port   = CT_UART_PORT, .uart_num = CT_UART_PORT,
		.tx_pin      = CT_UART_TX_PIN, .rx_pin = CT_UART_RX_PIN,
		.rts_pin     = UART_PIN_NO_CHANGE, .cts_pin = UART_PIN_NO_CHANGE,
		.dtr_pin     = CT_DTR_PIN,
		.uart_config = {
			.baud_rate = CT_UART_BAUD, .data_bits = UART_DATA_8_BITS,
			.parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1,
			.flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
			.source_clk = UART_SCLK_DEFAULT,
		},
	};
	err = ct511n_create(&g_ct511n, &ct_cfg);
	if (err != ESP_OK) { ESP_LOGE(TAG, "ct511n_create: %s", esp_err_to_name(err)); return err; }

	/* Initialise GPS module (start positioning) */
	esp_err_t gps_err = ct511n_gps_init(g_ct511n);
	if (gps_err != ESP_OK) {
		ESP_LOGW(TAG, "gps_init: %s (will retry on each sample)", esp_err_to_name(gps_err));
	}
	ESP_LOGI(TAG, "CT511N ready");

	/* ---- Sensor hub (I2C bus + BMI160 + AK09911C) ---- */
	sensor_hub_config_t hub_cfg = {
		.sda_io_num = I2C_SDA, .scl_io_num = I2C_SCL,
		.bmi160_int1_pin = BMI160_INT1_PIN,
		.bmi160_int1_type = GPIO_INTR_POSEDGE,
		.bmi160_int2_pin = GPIO_NUM_NC,
		.bmi160_int2_type = GPIO_INTR_DISABLE,
	};
	err = sensor_hub_init(&g_hub, &hub_cfg);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "sensor_hub_init: %s", esp_err_to_name(err));
		ct511n_destroy(g_ct511n);
		return err;
	}

	/* Register motion ISR */
	g_motion_sem = xSemaphoreCreateBinary();
	bmi160_int1_isr_add(sensor_hub_get_bmi160(g_hub), bmi160_motion_isr, NULL);

	/* ---- gptimer ---- */
	gptimer_config_t tcfg = {
		.clk_src = GPTIMER_CLK_SRC_DEFAULT,
		.direction = GPTIMER_COUNT_UP,
		.resolution_hz = GPTIMER_RESOLUTION_HZ,
	};
	err = gptimer_new_timer(&tcfg, &g_timer);
	if (err != ESP_OK) { ESP_LOGE(TAG, "gptimer: %s", esp_err_to_name(err)); return err; }

	gptimer_event_callbacks_t cbs = { .on_alarm = timer_on_alarm };
	gptimer_register_event_callbacks(g_timer, &cbs, NULL);
	gptimer_enable(g_timer);

	timer_start(g_timer, 1);  /* default 1 s interval */
	ct511n_sleep_dtr_enable(g_ct511n);
	g_last_motion_us = esp_timer_get_time();

	ESP_LOGI(TAG, "hardware init complete — CT511N asleep");
	return ESP_OK;
}

/* ======================================================================== */
/*  app_main                                                                 */
/* ======================================================================== */
void app_main(void)
{
	ESP_LOGI(TAG, "=== 4G Trajectory Logger ===");

	esp_err_t err = hardware_init();
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "hardware_init failed — HALTING");
		while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
	}

	g_state = STATE_ACTIVE;
	ESP_LOGI(TAG, "entering main loop");

	/* ================================================================== */
	/*  Main State Machine                                                */
	/* ================================================================== */
	while (1) {

		switch (g_state) {

		case STATE_ACTIVE:
			/* Motion resets inactivity timer */
			if (g_motion_flag) {
				g_motion_flag = false;
				g_last_motion_us = esp_timer_get_time();
			}

			/* Inactivity timeout → LOW_POWER */
			if ((esp_timer_get_time() - g_last_motion_us) >
			    (INACTIVITY_TIMEOUT_MS * 1000LL)) {
				low_power_enter(g_timer, g_ct511n, NULL, g_hub);
				g_state = STATE_LOW_POWER;
				break;
			}

			/* Timer → sample */
			if (g_sample_flag) {
				g_sample_flag = false;
				g_state = STATE_SAMPLE;
				break;
			}

			vTaskDelay(pdMS_TO_TICKS(IDLE_POLL_MS));
			break;

		case STATE_LOW_POWER:
			/* ISR or polling — wake and go straight to sample+upload */
			if (g_motion_flag) {
				g_motion_flag = false;
				low_power_exit(g_timer, g_ct511n, NULL, g_hub);
				g_state = STATE_SAMPLE;
				break;
			}
			{
				static uint64_t lp_last = 0;
				uint64_t now = esp_timer_get_time();
				if (now - lp_last > 1000000ULL) {
					lp_last = now;
					uint8_t istat[4];
					bmi160_handle_t *bmi = sensor_hub_get_bmi160(g_hub);
					if (bmi && bmi160_int_status_read(bmi, istat) == ESP_OK) {
						if (istat[0] & 0x07) {
							ESP_LOGI(TAG, "any-motion via polling");
							low_power_exit(g_timer, g_ct511n, NULL, g_hub);
							g_state = STATE_SAMPLE;
							break;
						}
					}
				}
			}
			vTaskDelay(pdMS_TO_TICKS(LP_POLL_MS));
			break;

		case STATE_SAMPLE:
			vTaskDelay(pdMS_TO_TICKS(CT_WAKE_DELAY_MS));

			sample_sensors(g_hub, g_ct511n, g_sample_count++);

			/* Upload all pending records (connect retry inside) */
			upload_send_all(g_ct511n);

			g_state = STATE_ACTIVE;
			break;

		default:
			g_state = STATE_ACTIVE;
			break;
		}
	}
}
