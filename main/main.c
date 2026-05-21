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
#include "W25Q64.h"
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
#define BMI160_INT1_PIN        LP_WAKEUP_GPIO

#define GPTIMER_RESOLUTION_HZ  1000000

#define CT_WAKE_DELAY_MS       500
#define INACTIVITY_TIMEOUT_MS  5000
#define IDLE_POLL_MS           50
#define LP_POLL_MS             200

/* BMI160 INT2 pin for double-tap → IMG_RECEIVE mode.
 * Double-tap the device to enter IMG_RECEIVE. */
#define BMI160_INT2_PIN         0

/* ======================================================================== */
/*  System State                                                            */
/* ======================================================================== */
typedef enum {
	STATE_INIT = 0,
	STATE_ACTIVE,
	STATE_LOW_POWER,
	STATE_SAMPLE,
	STATE_DEEP_SLEEP,           /* fallback: RTC timer wakeup only */
	STATE_IMG_RECEIVE,          /* BLE image receive + display */
} sys_state_t;

static const char *TAG = "main";

/* ---- Handles ---- */
static sensor_hub_t      *g_hub     = NULL;
static ct511n_handle_t   *g_ct511n  = NULL;
static w25q64_handle_t   *g_w25q64  = NULL;  /* SPI NOR Flash            */
static gptimer_handle_t   g_timer   = NULL;

/* ---- ISR flags ---- */
static volatile bool g_sample_flag = false;
static volatile bool g_motion_flag = false;
static volatile bool g_img_trigger_flag = false;  /* BMI160 double-tap      */
static SemaphoreHandle_t g_motion_sem = NULL;

/* ---- State tracking ---- */
static sys_state_t g_state          = STATE_INIT;
static int64_t     g_last_motion_us = 0;
static uint32_t    g_sample_count   = 0;
static int64_t     g_lp_enter_us    = 0;   /* when LOW_POWER was entered    */

/* ======================================================================== */
/*  ISR Handlers                                                             */
/* ======================================================================== */

static void IRAM_ATTR bmi160_double_tap_isr(void *arg)
{
	g_img_trigger_flag = true;
}

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
		.bmi160_int2_pin = BMI160_INT2_PIN,
		.bmi160_int2_type = GPIO_INTR_POSEDGE,
	};
	err = sensor_hub_init(&g_hub, &hub_cfg);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "sensor_hub_init: %s", esp_err_to_name(err));
		ct511n_destroy(g_ct511n);
		return err;
	}

	/* Register any-motion ISR (INT1) */
	g_motion_sem = xSemaphoreCreateBinary();
	bmi160_int1_isr_add(sensor_hub_get_bmi160(g_hub), bmi160_motion_isr, NULL);

	/* Register double-tap ISR (INT2) — triggers IMG_RECEIVE */
	bmi160_int2_isr_add(sensor_hub_get_bmi160(g_hub),
			    bmi160_double_tap_isr, NULL);

	/* ---- W25Q64 SPI NOR Flash ---- */
	{
		w25q64_config_t flash_cfg = {
			.host      = SPI_HOST,
			.cs_gpio   = W25Q64_CS_GPIO,
			.sck_gpio  = SPI_SCK_GPIO,
			.mosi_gpio = SPI_MOSI_GPIO,
			.miso_gpio = SPI_MISO_GPIO,
			.wp_gpio   = -1,
			.hold_gpio = -1,
			.dma_chan  = SPI_DMA_CH_AUTO,
			.freq_hz   = 26 * 1000 * 1000,  /* 26 MHz */
		};
		err = w25q64_init(&g_w25q64, &flash_cfg);
		if (err != ESP_OK) {
			ESP_LOGW(TAG, "W25Q64 init: %s — Flash disabled",
				 esp_err_to_name(err));
			g_w25q64 = NULL;
		} else {
			err = storage_init(g_w25q64);
			if (err != ESP_OK) {
				ESP_LOGW(TAG, "storage_init: %s",
					 esp_err_to_name(err));
			}
		}
	}

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

	/* Detect wakeup source after Deep-sleep / Light-sleep */
	uint32_t wakeup_causes = esp_sleep_get_wakeup_causes();
	if (wakeup_causes & BIT(ESP_SLEEP_WAKEUP_TIMER)) {
		ESP_LOGI(TAG, "woke from DEEP_SLEEP (RTC timer)");
	} else if (wakeup_causes & BIT(ESP_SLEEP_WAKEUP_GPIO)) {
		ESP_LOGI(TAG, "woke from Light-sleep (GPIO any-motion)");
	} else {
		ESP_LOGI(TAG, "cold boot (normal power-on or reset)");
	}

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

			/* BMI160 double-tap → IMG_RECEIVE */
			if (g_img_trigger_flag) {
				g_img_trigger_flag = false;
				g_state = STATE_IMG_RECEIVE;
				break;
			}

			/* Inactivity timeout → LOW_POWER */
			if ((esp_timer_get_time() - g_last_motion_us) >
			    (INACTIVITY_TIMEOUT_MS * 1000LL)) {
				low_power_enter(g_timer, g_ct511n,
						g_w25q64, g_hub);
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
			/* --- First entry: configure GPIO wakeup for Light-sleep --- */
			{
				static bool sleep_cfg_done = false;
				if (!sleep_cfg_done) {
					low_power_sleep_configure();
					g_lp_enter_us = esp_timer_get_time();
					sleep_cfg_done = true;
				}

				/* Motion ISR fired (GPIO wakeup or edge while awake) */
				if (g_motion_flag) {
					g_motion_flag = false;
					sleep_cfg_done = false;
					low_power_sleep_unconfigure();
					low_power_exit(g_timer, g_ct511n,
						       g_w25q64, g_hub);
					g_state = STATE_SAMPLE;
					break;
				}

				/* Poll INT_STATUS as backup (runs after timer wakeup) */
				{
					uint8_t istat[4];
					bmi160_handle_t *bmi = sensor_hub_get_bmi160(g_hub);
					if (bmi && bmi160_int_status_read(bmi, istat) == ESP_OK) {
						if (istat[0] & 0x07) {
							ESP_LOGI(TAG, "any-motion via poll wakeup");
							sleep_cfg_done = false;
							low_power_sleep_unconfigure();
							low_power_exit(g_timer, g_ct511n,
								       g_w25q64, g_hub);
							g_state = STATE_SAMPLE;
							break;
						}
					}
				}

				/* Deep-sleep fallback: no motion for too long */
				if (DEEP_SLEEP_FALLBACK_S > 0) {
					int64_t elapsed_us = esp_timer_get_time() - g_lp_enter_us;
					if (elapsed_us > (int64_t)DEEP_SLEEP_FALLBACK_S * 1000000LL) {
						ESP_LOGI(TAG, "no motion for %u s — deep-sleep",
							 DEEP_SLEEP_FALLBACK_S);
						sleep_cfg_done = false;
						low_power_sleep_unconfigure();
						g_state = STATE_DEEP_SLEEP;
						break;
					}
				}

				/* --- Light-sleep until GPIO or 1 s timer wakeup --- */
				esp_sleep_enable_timer_wakeup(1000000ULL);
				low_power_sleep_enter();
				/* wakeup: either motion (ISR set flag) or timer (poll above) */
			}
			break;

		case STATE_DEEP_SLEEP:
			low_power_deep_sleep_enter();
			/* never reached */
			break;

		case STATE_IMG_RECEIVE: {
			static bool img_init_done = false;
			if (!img_init_done) {
				esp_err_t err = img_recv_enter(SPI_HOST,
							       g_w25q64);
				if (err != ESP_OK) {
					ESP_LOGE(TAG, "img_recv_enter: %s",
						 esp_err_to_name(err));
					g_state = STATE_ACTIVE;
					break;
				}
				img_init_done = true;
			}

			/* Sample at 1 s intervals (driven by gptimer) */
			if (g_sample_flag) {
				g_sample_flag = false;
				if (!img_recv_poll(g_hub, g_ct511n,
						   g_sample_count++)) {
					img_recv_exit();
					img_init_done = false;
					g_state = STATE_ACTIVE;
					break;
				}
			}

			/* Check if BLE transfer finished (non-sampling poll) */
			if (!ble_img_is_busy() && img_init_done) {
				img_recv_exit();
				img_init_done = false;
				g_state = STATE_ACTIVE;
				break;
			}

			vTaskDelay(pdMS_TO_TICKS(50));
			break;
		}

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
