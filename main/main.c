/**
 * @file    main.c
 * @brief   4G Trajectory Logger  -- state machine with low-power idle mode.
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

#include "esp_err.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_task_wdt.h"
#include "esp_timer.h"
#include "nvs_flash.h"

#include "CT511N.h"
#include "W25Q64.h"
#include "sensor_hub.h"
#include "storage_mgr.h"
#include "app_utils.h"
#ifdef CONFIG_BT_NIMBLE_ENABLED
#include "ble_img_rx.h"
#endif
#include "config.h"
#include "epd_qyeg0397.h"
#include "wifi_cfg.h"

/* ======================================================================== */
/*  State Machine                                                           */
/* ======================================================================== */
typedef enum {
	STATE_INIT = 0,
	STATE_ACTIVE,
	STATE_LOW_POWER,
	STATE_SAMPLE,
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
		.uart_port   = CFG_CT_UART_PORT, .uart_num = CFG_CT_UART_PORT,
		.tx_pin      = CFG_CT_UART_TX_PIN, .rx_pin = CFG_CT_UART_RX_PIN,
		.rts_pin     = UART_PIN_NO_CHANGE, .cts_pin = UART_PIN_NO_CHANGE,
		.dtr_pin     = CFG_CT_DTR_PIN,
		.uart_config = {
			.baud_rate = CFG_CT_UART_BAUD, .data_bits = UART_DATA_8_BITS,
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
		.sda_io_num = CFG_I2C_SDA, .scl_io_num = CFG_I2C_SCL,
		.bmi160_int1_pin = CFG_BMI160_INT1_PIN,
		.bmi160_int1_type = GPIO_INTR_POSEDGE,
		.bmi160_int2_pin = CFG_BMI160_INT2_PIN,
		.bmi160_int2_type = GPIO_INTR_POSEDGE,
	};
	err = sensor_hub_init(&g_hub, &hub_cfg);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "sensor_hub_init: %s", esp_err_to_name(err));
		ct511n_destroy(g_ct511n);
		LOG_MEM_AFTER(TAG, "ct511n_destroy (error path)");
		return err;
	}

	/* Register any-motion ISR (INT1) */
	g_motion_sem = xSemaphoreCreateBinary();
	bmi160_int1_isr_add(sensor_hub_get_bmi160(g_hub), bmi160_motion_isr, NULL);

	/* Register double-tap ISR (INT2)  -- triggers IMG_RECEIVE */
	bmi160_int2_isr_add(sensor_hub_get_bmi160(g_hub),
			    bmi160_double_tap_isr, NULL);

	/* ---- W25Q64 SPI NOR Flash ---- */
	{
		w25q64_config_t flash_cfg = {
			.host      = CFG_SPI_HOST,
			.cs_gpio   = CFG_W25Q64_CS_GPIO,
			.sck_gpio  = CFG_SPI_SCK_GPIO,
			.mosi_gpio = CFG_SPI_MOSI_GPIO,
			.miso_gpio = CFG_SPI_MISO_GPIO,
			.wp_gpio   = -1,
			.hold_gpio = -1,
			.dma_chan  = SPI_DMA_CH_AUTO,
			.freq_hz   = CFG_W25Q64_SPI_FREQ_HZ,
		};
		err = w25q64_init(&g_w25q64, &flash_cfg);
		if (err != ESP_OK) {
			ESP_LOGW(TAG, "W25Q64 init: %s  -- Flash disabled",
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
		.resolution_hz = CFG_GPTIMER_RESOLUTION_HZ,
	};
	err = gptimer_new_timer(&tcfg, &g_timer);
	if (err != ESP_OK) { ESP_LOGE(TAG, "gptimer: %s", esp_err_to_name(err)); return err; }

	gptimer_event_callbacks_t cbs = { .on_alarm = timer_on_alarm };
	gptimer_register_event_callbacks(g_timer, &cbs, NULL);
	gptimer_enable(g_timer);

	timer_start(g_timer, CFG_SAMPLE_INTERVAL_S);
	ct511n_sleep_dtr_enable(g_ct511n);
	g_last_motion_us = esp_timer_get_time();

	ESP_LOGI(TAG, "hardware init complete  -- CT511N asleep");
	return ESP_OK;
}

/* ======================================================================== */
/*  app_main                                                                 */
/* ======================================================================== */
void app_main(void)
{
	ESP_LOGI(TAG, "=== 4G Trajectory Logger ===");

/* Init NVS  -- required by WiFi and other subsystems */
	esp_err_t nvs_err = nvs_flash_init();
	if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES ||
	    nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		nvs_flash_erase();
		nvs_flash_init();
	}
	/* Detect wakeup source after reset */
	uint32_t wakeup_causes = esp_sleep_get_wakeup_causes();
	if (wakeup_causes & (1U << ESP_SLEEP_WAKEUP_GPIO)) {
		ESP_LOGI(TAG, "woke from GPIO (BMI160 any-motion)");
	} else {
		ESP_LOGI(TAG, "cold boot (normal power-on or reset)");
	}

	/* Hardware init (SPI, UART, I2C, sensors, Flash, timer) */
	esp_err_t err = hardware_init();
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "hardware_init failed  -- HALTING");
		while (1) { vTaskDelay(pdMS_TO_TICKS(1000)); }
	}

	g_state = STATE_ACTIVE;
	ESP_LOGI(TAG, "entering main loop");

	/* ---- EPD display (shared SPI bus with W25Q64) ---- */
	if (g_w25q64 != NULL) {
		esp_err_t err = epd_app_init(CFG_SPI_HOST, g_w25q64);
		if (err != ESP_OK) {
			ESP_LOGW(TAG, "EPD init failed — continuing without EPD");
		}
	}

	/* ---- Phase 1: WiFi STA-only (clean heap, before BLE) ---- */
	{
		uint32_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);
		ESP_LOGI(TAG, "Before WiFi: largest free block=%lu bytes%s",
			 largest,
			 largest < 30000 ? " ⚠️ may fail!" : "");
	}
	ESP_LOGI(TAG, "WiFi STA-only: trying saved network...");
	if (wifi_cfg_start_sta_only() == ESP_OK && wifi_cfg_is_sta_connected()) {
		ESP_LOGI(TAG, "STA connected — syncing data");
		upload_send_all(g_ct511n);
	} else {
		ESP_LOGI(TAG, "STA not available at boot");
	}
	wifi_cfg_deinit(); /* fully release WiFi driver memory before BLE */
	LOG_MEM_AFTER(TAG, "wifi_cfg_deinit (boot)");

	/* ---- Phase 2: BLE init (heap clean after WiFi deinit) ---- */
#ifdef CONFIG_BT_NIMBLE_ENABLED
	esp_err_t ble_err = ble_img_init(epd_ble_img_ready);
	if (ble_err != ESP_OK) {
		ESP_LOGW(TAG, "BLE init: %s", esp_err_to_name(ble_err));
	} else {
		ESP_LOGI(TAG, "BLE advertising as \"4G-Tracker\"");
	}
#endif

	/* ================================================================== */
	/*  Main State Machine                                                */
	/* ================================================================== */
	while (1) {

		/* 检查 EPD 更新标志（由 BLE 回调在蓝牙任务中设置） */
		if (g_need_epd_update) {
			g_need_epd_update = false;
#ifdef CONFIG_BT_NIMBLE_ENABLED
			/* 暂停 BLE 广播，释放 RF 资源供 EPD SPI 独占使用 */
			ble_img_pause();
#endif
			ESP_LOGI(TAG, "EPD update from main task (BLE paused)...");
			epd_update_full_from_flash(IMG_CACHE_BASE_ADDR);
			ESP_LOGI(TAG, "EPD update done");
#ifdef CONFIG_BT_NIMBLE_ENABLED
			vTaskDelay(pdMS_TO_TICKS(100)); /* 让系统呼吸，释放碎片 */
			ble_img_resume();
#endif
		}

		switch (g_state) {

		case STATE_ACTIVE:
#ifdef CONFIG_BT_NIMBLE_ENABLED
			/* ⭐ 蓝牙图片传输中——立即切出 ACTIVE 状态，屏蔽 4G/WiFi 干扰 */
			if (ble_img_is_busy()) {
				g_state = STATE_IMG_RECEIVE;
				break;
			}
#endif
			/* Motion resets inactivity timer */
			if (g_motion_flag) {
				g_motion_flag = false;
				g_last_motion_us = esp_timer_get_time();
			}

			/*
			 * BMI160 double-tap:
			 *   Pause BLE → AP config mode (60s) → Resume BLE
			 *   20s for station connect; if connected, keep until config saved.
			 */
			if (g_img_trigger_flag) {
				g_img_trigger_flag = false;
				ESP_LOGI(TAG, "double-tap — AP config mode 60s");
#ifdef CONFIG_BT_NIMBLE_ENABLED
				ble_img_pause();  /* fully deinit BLE host+controller → frees ~50KB */
#endif
				vTaskDelay(pdMS_TO_TICKS(200));
				ESP_LOGI(TAG, "Free internal heap before WiFi: %d bytes",
					 heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
				if (wifi_cfg_start_ap_only(NULL) == ESP_OK) {
					int64_t ap_start = esp_timer_get_time();
					bool sta_conn = false;
					while (esp_timer_get_time() - ap_start < CFG_AP_TOTAL_TIMEOUT_US) {
						int64_t elapsed = esp_timer_get_time() - ap_start;
						if (!sta_conn && elapsed > CFG_AP_NO_STA_TIMEOUT_US) {
							ESP_LOGI(TAG, "AP: no station in 20s");
							break;
						}
						if (!sta_conn && wifi_cfg_is_ap_sta_connected()) {
							sta_conn = true;
							ESP_LOGI(TAG, "AP: station connected");
						}
						if (wifi_cfg_is_done()) {
							ESP_LOGI(TAG, "AP: config saved");
							break;
						}
						vTaskDelay(pdMS_TO_TICKS(100));
					}
				}
				wifi_cfg_deinit(); /* fully release WiFi memory before BLE resume */
				LOG_MEM_AFTER(TAG, "wifi_cfg_deinit (AP config)");
				ESP_LOGI(TAG, "AP closed — returning to BLE");
#ifdef CONFIG_BT_NIMBLE_ENABLED
				ble_img_resume();
#endif
				break;
			}

			/* Periodic STA scan — 时分复用: 全停 BLE → WiFi → 重启 BLE */
			{
				static int64_t next_wifi_us = 0;
				static bool prev_ok = false;
				int64_t nw = esp_timer_get_time();
				if (next_wifi_us == 0)
					next_wifi_us = nw + CFG_PERIODIC_SCAN_INTERVAL_US;
				if (nw >= next_wifi_us) {
					next_wifi_us = nw + (prev_ok ? CFG_WIFI_UPLOAD_INTERVAL_US
							      : CFG_PERIODIC_SCAN_INTERVAL_US);
					ESP_LOGI(TAG, "WiFi: pausing BLE...");
#ifdef CONFIG_BT_NIMBLE_ENABLED
					ble_img_pause();
#endif
					vTaskDelay(pdMS_TO_TICKS(200));
					ESP_LOGI(TAG, "Free internal heap before WiFi: %d bytes",
						 heap_caps_get_free_size(MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL));
					bool ok = (wifi_cfg_start_sta_only() == ESP_OK);
					if (ok) {
						ESP_LOGI(TAG, "WiFi: connected, uploading...");
						upload_send_all(g_ct511n);
					}
					wifi_cfg_deinit();
					LOG_MEM_AFTER(TAG, "wifi_cfg_deinit (periodic scan)");
					ESP_LOGI(TAG, "WiFi: done (ok=%d), resuming BLE", ok);
#ifdef CONFIG_BT_NIMBLE_ENABLED
					ble_img_resume();
#endif
					prev_ok = ok;
				}
			}

			/* Inactivity timeout  -- LOW_POWER */
			if ((esp_timer_get_time() - g_last_motion_us) >
			    (CFG_INACTIVITY_TIMEOUT_MS * 1000LL)) {
#ifdef CONFIG_BT_NIMBLE_ENABLED
				/* Don't sleep while BLE image transfer is active */
				if (ble_img_is_busy()) {
					g_last_motion_us = esp_timer_get_time();
					break;
				}
#endif
				low_power_enter(g_timer, g_ct511n,
						g_w25q64, g_hub);
				g_state = STATE_LOW_POWER;
				break;
			}

			/* Timer  -- sample */
			if (g_sample_flag) {
				g_sample_flag = false;
				g_state = STATE_SAMPLE;
				break;
			}

			vTaskDelay(pdMS_TO_TICKS(CFG_IDLE_POLL_MS));
			break;

		case STATE_LOW_POWER:
	{
		bmi160_handle_t *bmi = sensor_hub_get_bmi160(g_hub);

		enum { LP_DEEP_WAIT, LP_MODEM_WAIT } static lp_phase = LP_DEEP_WAIT;
		static bool entry_done = false;
		static int64_t motion_first_us = 0;

		if (!entry_done) {
			lp_phase = LP_DEEP_WAIT;
			low_power_sleep_configure();
			g_lp_enter_us = esp_timer_get_time();
			motion_first_us = 0;
			entry_done = true;
		}

		/* ---- Phase: DEEP_WAIT  -- automatic light sleep via PM ---- */
		if (lp_phase == LP_DEEP_WAIT) {
			/* Re-enable GPIO wakeup before every sleep cycle —
			 * esp_sleep_enable_gpio_wakeup() is consumed by each
			 * light-sleep and the PM framework in v6.0 does NOT
			 * automatically re-call it. */
			esp_sleep_enable_gpio_wakeup();
			if (xSemaphoreTake(g_motion_sem, pdMS_TO_TICKS(1000)) == pdTRUE) {
				ESP_LOGI(TAG, "motion #1 (GPIO wakeup)  -- MODEM_WAIT");
				lp_phase = LP_MODEM_WAIT;
				motion_first_us = esp_timer_get_time();
				g_motion_flag = false;
				break;
			}
			/* Timer expired  -- no motion, stay in DEEP_WAIT */
			g_motion_flag = false;
			break;
		}

		/* ---- Phase: MODEM_WAIT  -- CPU running, debounced wait ---- */
		bool motion_detected = false;
		int64_t now = 0;

		if (xSemaphoreTake(g_motion_sem, pdMS_TO_TICKS(1000)) == pdTRUE) {
			motion_detected = true;
			now = esp_timer_get_time();
		}
		if (g_motion_flag) {
			motion_detected = true;
			if (now == 0) now = esp_timer_get_time();
		}
		g_motion_flag = false;

		if (!motion_detected && bmi) {
			uint8_t istat[4];
			if (bmi160_int_status_read(bmi, istat) == ESP_OK
			    && (istat[0] & 0x07)) {
				motion_detected = true;
				now = esp_timer_get_time();
			}
		}

		if (motion_detected) {
			if ((now - motion_first_us) >= 2000000LL) {
				ESP_LOGI(TAG, "motion #2 after %lld ms  -- EXIT",
					 (now - motion_first_us) / 1000LL);
				lp_phase = LP_DEEP_WAIT;
				motion_first_us = 0;
				entry_done = false;
				low_power_sleep_unconfigure();
				low_power_exit(g_timer, g_ct511n,
					       g_w25q64, g_hub);
				g_state = STATE_SAMPLE;
				break;
			}
		} else {
			lp_phase = LP_DEEP_WAIT;
			motion_first_us = 0;
		}
	}
	break;

		case STATE_IMG_RECEIVE: {
#ifdef CONFIG_BT_NIMBLE_ENABLED
			static bool xfer_active = false;
			static int64_t last_data_time_us = 0;
			static uint32_t prev_bytes = 0;

			if (!xfer_active) {
				storage_set_ram_mode(true);
				xfer_active = true;
				prev_bytes = 0;
				last_data_time_us = esp_timer_get_time();
				ESP_LOGI(TAG, "IMG_RECEIVE started — waiting for BLE image...");
			}

			/* 图片传输期间跳过采样，防止 SPI 竞争 */
			if (g_sample_flag) {
				g_sample_flag = false;
				ESP_LOGW(TAG, "Sampling skipped to protect ongoing BLE transfer");
			}

			/* 🛑 动态进度监控：获取当前实际收到的字节数 */
			uint32_t cur_bytes = ble_img_bytes_received();
			if (cur_bytes != prev_bytes) {
				prev_bytes = cur_bytes;
				last_data_time_us = esp_timer_get_time(); // 只要收到新数据，刷新时间
			}

			/* 🛑【阶梯超时机制 - 核心修复】
			 * 如果 cur_bytes == 0 (代表手机还在准备中)，给 35 秒宽容期
			 * 如果 cur_bytes > 0  (代表已经开始传输)，给 6 秒断流超时 */
			int64_t timeout_threshold_us = (cur_bytes == 0) ? 35000000LL : 6000000LL;
			int64_t now_us = esp_timer_get_time();

			if (xfer_active && (now_us - last_data_time_us > timeout_threshold_us)) {
				ESP_LOGE(TAG, "BLE transfer TIMEOUT (Received: %d bytes) — forcing exit", cur_bytes);
				storage_set_ram_mode(false);
				ram_cache_flush();
				xfer_active = false;
				g_state = STATE_ACTIVE;
				break;
			}

			/* 正常传输完成 */
			if (!ble_img_is_busy() && xfer_active) {
				ESP_LOGI(TAG, "BLE transfer finished successfully! Total: %d bytes", cur_bytes);
				storage_set_ram_mode(false);
				ram_cache_flush();
				xfer_active = false;
				g_state = STATE_ACTIVE;
				break;
			}
			vTaskDelay(pdMS_TO_TICKS(50));
#else
			ESP_LOGW(TAG, "IMG_RECEIVE skipped — BLE not enabled");
			g_state = STATE_ACTIVE;
#endif
			break;
		}

		case STATE_SAMPLE: {
			/* 1. 进门立刻拦截 */
#ifdef CONFIG_BT_NIMBLE_ENABLED
			if (ble_img_is_busy()) {
				ESP_LOGW(TAG, "BLE busy BEFORE sample delay — intercept, goto IMG_RECEIVE");
				g_state = STATE_IMG_RECEIVE;
				break;
			}
#endif

			/* 2. 将原本整块的 Delay 拆碎，防止在 Delay 期间无法拦截蓝牙 */
			int delay_ms = CFG_CT_WAKE_DELAY_MS;
			bool ble_interrupted = false;
			while (delay_ms > 0) {
				vTaskDelay(pdMS_TO_TICKS(20));
				delay_ms -= 20;
#ifdef CONFIG_BT_NIMBLE_ENABLED
				if (ble_img_is_busy()) {
					ESP_LOGW(TAG, "BLE busy DURING sample delay — intercept, goto IMG_RECEIVE");
					g_state = STATE_IMG_RECEIVE;
					ble_interrupted = true;
					break;
				}
#endif
			}
			if (ble_interrupted) break; // 如果在延时期间蓝牙忙了，直接退出

			/* 3. 只有蓝牙不忙才执行耗时采样 */
			sample_sensors(g_hub, g_ct511n, g_sample_count++);

			/* 4. 采样完、上报前，进行最后一道钢铁防线拦截 */
#ifdef CONFIG_BT_NIMBLE_ENABLED
			if (ble_img_is_busy()) {
				ESP_LOGW(TAG, "BLE busy AFTER sample — DEFERRING 4G upload, goto IMG_RECEIVE");
				g_state = STATE_IMG_RECEIVE;
				break; // 🛑 坚决不调用 upload_send_all，防止 4G 拨号卡死蓝牙！
			}
#endif

			/* 5. 此时确保蓝牙绝对安全、空闲，才允许执行可能阻塞的 4G 上传 */
			upload_send_all(g_ct511n);
			g_state = STATE_ACTIVE;
			break;
		}

		case STATE_INIT:
		default:
			g_state = STATE_ACTIVE;
			break;
		}
	}
}
