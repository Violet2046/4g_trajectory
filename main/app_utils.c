#include "app_utils.h"
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "storage_mgr.h"
#include "wifi_cfg.h"
#ifdef CONFIG_BT_NIMBLE_ENABLED
#include "ble_img_rx.h"
#endif
#include "epd_qyeg0397.h"

#define TAG "main"

/* ========================================================================= */
/*  Timer helpers                                                            */
/* ========================================================================= */
esp_err_t timer_start(gptimer_handle_t timer, uint32_t interval_s)
{
	if (timer == NULL) return ESP_ERR_INVALID_STATE;
	gptimer_stop(timer);
	gptimer_alarm_config_t acfg = {
		.alarm_count = (uint64_t)interval_s * 1000000ULL,
		.reload_count = 0,
		.flags.auto_reload_on_alarm = true,
	};
	esp_err_t err = gptimer_set_alarm_action(timer, &acfg);
	if (err != ESP_OK) return err;
	return gptimer_start(timer);
}

esp_err_t timer_stop(gptimer_handle_t timer)
{
	if (timer == NULL) return ESP_ERR_INVALID_STATE;
	return gptimer_stop(timer);
}

/* ========================================================================= */
/*  Low-power helpers                                                        */
/* ========================================================================= */
void low_power_enter(gptimer_handle_t timer,
		     ct511n_handle_t *ct511n,
		     w25q64_handle_t *w25q64,
		     sensor_hub_t *hub)
{
	esp_err_t err;
	ESP_LOGI(TAG, ">> LOW_POWER (no motion for 5 s)");
	timer_stop(timer);
	if (w25q64 != NULL) w25q64_power_down(w25q64);
	err = sensor_hub_sleep(hub);
	if (err != ESP_OK) {
		ESP_LOGW(TAG, "sensor_hub_sleep failed: %s  -- wakeup may not work",
			 esp_err_to_name(err));
	}
#ifdef CONFIG_BT_NIMBLE_ENABLED
	ble_adv_stop();
	vTaskDelay(pdMS_TO_TICKS(150));   /* wait for NimBLE host to process stop */
#endif
	ESP_LOGI(TAG, "waiting for any-motion...");
}

void low_power_exit(gptimer_handle_t timer,
		    ct511n_handle_t *ct511n,
		    w25q64_handle_t *w25q64,
		    sensor_hub_t *hub)
{
	ESP_LOGI(TAG, "<< exiting LOW_POWER");
	if (w25q64 != NULL) w25q64_release_power_down(w25q64);
	vTaskDelay(pdMS_TO_TICKS(1));
	vTaskDelay(pdMS_TO_TICKS(500));
	sensor_hub_wake(hub);
	timer_start(timer, CFG_SAMPLE_INTERVAL_S);
#ifdef CONFIG_BT_NIMBLE_ENABLED
	/* Retry adv_start from main-task context (safe to vTaskDelay — won't
	 * block the NimBLE host task).  The sync callback's first attempt
	 * may fail if the GAP module hasn't finished init yet. */
	for (int i = 0; i < 5; i++) {
		if (ble_adv_start() == ESP_OK) break;
		vTaskDelay(pdMS_TO_TICKS(50));
	}
#endif
	ESP_LOGI(TAG, "all modules restored to ACTIVE");
}

/* ========================================================================= */
/*  Low-power sleep helpers                                                  */
/* ========================================================================= */
void low_power_sleep_configure(void)
{
	/* BMI160 INT1 pin  -- any-motion active-high pulse  -- wakeup.
	 * gpio_wakeup_enable() is persistent across sleep cycles;
	 * esp_sleep_enable_gpio_wakeup() is consumed each sleep and
	 * must be re-called before every esp_light_sleep_start(). */
	gpio_wakeup_enable(CFG_BMI160_INT1_PIN, GPIO_INTR_HIGH_LEVEL);
	esp_sleep_enable_gpio_wakeup();
	ESP_LOGI(TAG, "light-sleep wakeup configured on GPIO %d (HIGH_LEVEL)",
		 (int)CFG_BMI160_INT1_PIN);
}

void low_power_sleep_unconfigure(void)
{
	gpio_wakeup_disable(CFG_BMI160_INT1_PIN);
}

/* ========================================================================= */
/*  Upload helpers                                                           */
/* ========================================================================= */
/* Fallback RAM buffer for last sample (used when no flash) */
static storage_record_t g_last_rec;
static bool g_rec_valid = false;

/* ---- EPD overlay scratch buffer (small, fits in DRAM) ---- */
#define OVERLAY_BUF_SIZE    ((OVERLAY_W * OVERLAY_H) / 4UL)  /* ~10 KB */
static uint8_t g_overlay_buf[OVERLAY_BUF_SIZE];
static bool g_overlay_inited = false;

/* ---- EPD is initialised? ---- */
static bool g_epd_ready = false;

/* ---- EPD update flag — set by BLE callback, consumed by main loop ---- */
volatile bool g_need_epd_update = false;

/* ---- EPD initialisation (call once after hardware_init) ---- */

esp_err_t epd_app_init(spi_host_device_t host, void *flash_handle)
{

    epd_config_t cfg = {
        .spi_host     = host,
        .clk_speed_hz = CFG_EPD_SPI_FREQ_HZ,
        .pin_cs       = CFG_EPD_CS_GPIO,
        .pin_dc       = CFG_EPD_DC_GPIO,
        .pin_rst      = CFG_EPD_RST_GPIO,
        .pin_busy     = CFG_EPD_BUSY_GPIO,
        .pin_mosi     = CFG_SPI_MOSI_GPIO,
        .pin_sclk     = CFG_SPI_SCK_GPIO,
    };

    esp_err_t err = epd_init_shared_bus(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "EPD init: %s", esp_err_to_name(err));
        return err;
    }

    epd_set_flash_handle(flash_handle);
    g_epd_ready = true;
    ESP_LOGI(TAG, "EPD ready");
    return ESP_OK;
}

/** Update EPD sensor overlay with latest sample data. */
static void epd_overlay_update(const sensor_sample_t *s)
{
    if (!g_epd_ready || !s) return;

    epd_sensor_data_t d = {
        .acc_x = s->acc.x, .acc_y = s->acc.y, .acc_z = s->acc.z,
        .gyr_x = s->gyr.x, .gyr_y = s->gyr.y, .gyr_z = s->gyr.z,
        .mag_x = s->mag.x, .mag_y = s->mag.y, .mag_z = s->mag.z,
        .gps_fix = s->gps_fix, .lat = s->lat, .lon = s->lon,
    };
    epd_sensor_overlay_window(g_overlay_buf, &d);
}

/* ---- BLE image → EPD display callback (runs in BLE host task) ---- */
#ifdef CONFIG_BT_NIMBLE_ENABLED
int epd_ble_img_ready(uint32_t total_bytes)
{
    if (!g_epd_ready) return -1;
    ESP_LOGI(TAG, "BLE image received (%lu B) — signaling main task",
             (unsigned long)(total_bytes > 0 ? total_bytes : EPD_DATA_SIZE));
    /* 仅设置标志位，由 main 任务在安全上下文中执行刷屏 */
    g_need_epd_update = true;
    return 0;
}
#else
int epd_ble_img_ready(uint32_t total_bytes)
{
    (void)total_bytes;
    ESP_LOGW(TAG, "BLE not configured — cannot display image");
    return -1;
}
#endif
/** Build payload string from a storage_record_t. Returns payload length. */
static int build_payload(char *buf, size_t bufsz, const storage_record_t *r)
{
	return snprintf(buf, bufsz,
		 "T:%lu F:%d LA:%.6f LO:%.6f "
		 "AX:%.3f AY:%.3f AZ:%.3f "
		 "GX:%.3f GY:%.3f GZ:%.3f "
		 "MX:%.1f MY:%.1f MZ:%.1f",
		 (unsigned long)r->timestamp, r->gps_fix,
		 r->lat, r->lon,
		 r->acc_x, r->acc_y, r->acc_z,
		 r->gyro_x, r->gyro_y, r->gyro_z,
		 r->mag_x, r->mag_y, r->mag_z);
}

void upload_send_all(ct511n_handle_t *ct511n)
{
	char payload[512];
	uint32_t sent = 0;
	uint32_t failed = 0;
	esp_err_t err;
	/* ---- Prefer WiFi if STA is connected ---- */
	if (wifi_cfg_is_sta_connected()) {
		/* Rate-limit WiFi uploads: only every CFG_WIFI_UPLOAD_INTERVAL_US */
		static int64_t last_wifi_up_us = 0;
		int64_t now = esp_timer_get_time();
		if (now - last_wifi_up_us < CFG_WIFI_UPLOAD_INTERVAL_US) {
			return;  /* too soon — data stays in flash for next batch */
		}
		last_wifi_up_us = now;
		/* Close 4G data network + DTR sleep to save power */
		ct511n_4g_net_close(ct511n);
		vTaskDelay(pdMS_TO_TICKS(100));
		ct511n_sleep_dtr_enable(ct511n);
		for (;;) {
			storage_record_t rec;
			bool found = false;
			if (storage_record_peek(&rec, &found) != ESP_OK || !found) break;
			build_payload(payload, sizeof(payload), &rec);
			if (wifi_cfg_tcp_send(payload) > 0) {
				storage_record_mark_uploaded();
				sent++;
			} else {
				storage_record_set_last_status(STORAGE_REC_STATUS_FAILED);
				failed++;
			}
			vTaskDelay(pdMS_TO_TICKS(50));
		}
		if (sent == 0 && failed == 0 && g_rec_valid) {
			build_payload(payload, sizeof(payload), &g_last_rec);
			if (wifi_cfg_tcp_send(payload) > 0) {
				sent = 1; g_rec_valid = false;
			}
		}
		ESP_LOGI(TAG, "upload done (WiFi)  -- %lu sent, %lu failed",
			 (unsigned long)sent, (unsigned long)failed);
		return;
	}
	/* ---- Fallback: 4G via CT511N (use cached config, avoid SPI read) ---- */
	static bool s_server_connected = false;
	static bool s_first_connect_done = false;
	static storage_config_t s_cached_cfg;
	static bool s_cached_cfg_valid = false;

	if (!s_cached_cfg_valid) {
		storage_config_default(&s_cached_cfg);
		if (storage_config_read(&s_cached_cfg) == ESP_OK) {
			s_cached_cfg_valid = true;
		} else {
			ESP_LOGW(TAG, "storage_config_read failed — using defaults");
		}
	}
	ESP_LOGI(TAG, "4G TCP target: %s:%s", s_cached_cfg.server_ip, s_cached_cfg.server_port);

	/* ---- Connect phase: only when not already connected ---- */
	if (!s_server_connected) {
		int max_retries = s_first_connect_done ? 1 : 5;
		ESP_LOGI(TAG, "TCP connecting (max %d attempts)...", max_retries);
		for (int retry = 0; retry < max_retries; retry++) {
			err = ct511n_tcp_single_connect(ct511n, s_cached_cfg.server_ip,
							s_cached_cfg.server_port);
			if (err == ESP_OK) {
				s_server_connected = true;
				ESP_LOGI(TAG, "TCP connected (attempt %d/%d)",
					 retry + 1, max_retries);
				break;
			}
			ESP_LOGW(TAG, "TCP attempt %d/%d failed", retry + 1, max_retries);
		}
		s_first_connect_done = true;
	}

	/* ---- Send phase: only if connected ---- */
	if (s_server_connected) {
		for (;;) {
			storage_record_t rec;
			bool found = false;
			if (storage_record_peek(&rec, &found) != ESP_OK || !found) break;
			build_payload(payload, sizeof(payload), &rec);
			err = ct511n_4g_tcp_send(ct511n, payload);
			if (err == ESP_OK) {
				storage_record_mark_uploaded();
				sent++;
			} else {
				ESP_LOGW(TAG, "send failed — server disconnected");
				s_server_connected = false;
				storage_record_set_last_status(STORAGE_REC_STATUS_FAILED);
				failed++;
				break;  /* stop sending; data stays in flash for next time */
			}
			vTaskDelay(pdMS_TO_TICKS(50));
		}
		/* RAM fallback (only used when flash is unavailable) */
		if (s_server_connected && g_rec_valid) {
			build_payload(payload, sizeof(payload), &g_last_rec);
			if (ct511n_4g_tcp_send(ct511n, payload) == ESP_OK) {
				sent++; g_rec_valid = false;
			} else {
				s_server_connected = false;
			}
		}
	} else {
		ESP_LOGW(TAG, "TCP not connected — upload skipped (records stay in flash)");
	}

	ESP_LOGI(TAG, "upload done (4G)  -- %lu sent, %lu failed  connected=%d",
		 (unsigned long)sent, (unsigned long)failed, s_server_connected);
}

/* ========================================================================= */
/*  Unix timestamp: GPS → WiFi SNTP → 4G CCLK → uptime                      */
/* ========================================================================= */
static uint32_t get_unix_timestamp(ct511n_handle_t *ct511n)
{
	struct tm tm = {0};
	time_t t;

	/* 1. Try GPS time (GPSST: DDMMYY HHMMSS) */
	if (ct511n) {
		char time_buf[32];
		if (ct511n_gps_get_time(ct511n, time_buf, sizeof(time_buf)) == ESP_OK
		    && time_buf[0]) {
			memset(&tm, 0, sizeof(tm));
			if (sscanf(time_buf, "%2d%2d%2d %2d%2d%2d",
				   &tm.tm_mday, &tm.tm_mon, &tm.tm_year,
				   &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6
			    && tm.tm_mday >= 1 && tm.tm_mday <= 31
			    && tm.tm_mon  >= 1 && tm.tm_mon  <= 12
			    && tm.tm_year >= 0 && tm.tm_year <= 99) {
				tm.tm_year += 100;
				tm.tm_mon  -= 1;
				t = mktime(&tm);
				if (t > 1000000000) {
					ESP_LOGD(TAG, "time source: GPS");
					return (uint32_t)t;
				}
			}
		}
	}

	/* 2. Try WiFi SNTP */
	t = time(NULL);
	if (t > 1700000000) {
		ESP_LOGD(TAG, "time source: SNTP");
		return (uint32_t)t;
	}

	/* 3. Try 4G CCLK (format: cclk=YY/MM/DD,HH:MM:SS+TZ) */
	if (ct511n) {
		char clk_buf[64];
		if (ct511n_4g_clk_get(ct511n, clk_buf, sizeof(clk_buf)) == ESP_OK
		    && clk_buf[0]) {
			memset(&tm, 0, sizeof(tm));
			if (sscanf(clk_buf, "cclk=%2d/%2d/%2d%*c%2d:%2d:%2d",
				   &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
				   &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 6
			    && tm.tm_year >= 20 && tm.tm_year <= 99
			    && tm.tm_mon  >= 1  && tm.tm_mon  <= 12
			    && tm.tm_mday >= 1  && tm.tm_mday <= 31) {
				tm.tm_year += 100;
				tm.tm_mon  -= 1;
				t = mktime(&tm);
				if (t > 1000000000) {
					ESP_LOGD(TAG, "time source: 4G CCLK");
					return (uint32_t)t;
				}
			}
		}
	}

	/* 4. Fallback: uptime */
	uint32_t uptime = (uint32_t)(esp_timer_get_time() / 1000000ULL);
	ESP_LOGD(TAG, "time source: uptime (%lu)", (unsigned long)uptime);
	return uptime;
}

/* ========================================================================= */
/*  Sensor power-save helpers (post-sample)                                  */
/* ========================================================================= */

/** Power down non-essential sensors after a sample:
 *  - AK09911C → POWERDOWN
 *  - BMI160 gyro → SUSPEND
 *  (BMI160 accelerometer stays on for any-motion wakeup) */
static void sensors_power_save(sensor_hub_t *hub)
{
    if (!hub) return;
    bmi160_handle_t *bmi = sensor_hub_get_bmi160(hub);
    ak09911_handle_t *mag = sensor_hub_get_ak09911(hub);
    if (bmi) bmi160_gyr_set_mode(bmi, BMI160_MODE_SUSPEND);
    if (mag) ak09911_mode_set(mag, AK09911_MODE_POWERDOWN);
}

/** Restore sensors before a sample (reverse of sensors_power_save). */
static void sensors_power_restore(sensor_hub_t *hub)
{
    if (!hub) return;
    bmi160_handle_t *bmi = sensor_hub_get_bmi160(hub);
    ak09911_handle_t *mag = sensor_hub_get_ak09911(hub);
    if (bmi) bmi160_gyr_set_mode(bmi, BMI160_MODE_NORMAL);
    if (mag) ak09911_mode_set(mag, AK09911_MODE_CONT_1);
}

/* ========================================================================= */
/*  Sample + upload                                                          */
/* ========================================================================= */
bool sample_sensors(sensor_hub_t *hub, ct511n_handle_t *ct511n,
		    uint32_t count)
{
	sensor_sample_t s;

	/* Restore sensors that were powered down last sample */
	sensors_power_restore(hub);

	sensor_hub_sample(hub, &s);
	s.timestamp = get_unix_timestamp(ct511n);
	/* GPS */
	char gps_buf[256] = {0};
	esp_err_t gps_err = ct511n_gps_get(ct511n, gps_buf, sizeof(gps_buf));
	if (gps_err == ESP_OK && strlen(gps_buf) > 10) {
		s.gps_fix = true;
		float lat = 0.0f, lon = 0.0f;
		if (sscanf(gps_buf, "%f,%f", &lat, &lon) == 2) {
			s.lat = lat; s.lon = lon;
		}
	}
	/* Log summary every 2 s */
	if ((count & 1) == 0) {
		ESP_LOGI(TAG, "S#%lu GPS:%d acc=(%.2f,%.2f,%.2f) raw=%s",
			 (unsigned long)count, s.gps_fix,
			 s.acc.x, s.acc.y, s.acc.z,
			 strlen(gps_buf) ? gps_buf : "--");
	}
	/* Build & store record */
	storage_record_t rec;
	memset(&rec, 0, sizeof(rec));
	rec.timestamp = s.timestamp;
	rec.gps_fix   = s.gps_fix ? 1 : 0;
	rec.lat       = s.lat;
	rec.lon       = s.lon;
	rec.acc_x     = s.acc.x;
	rec.acc_y     = s.acc.y;
	rec.acc_z     = s.acc.z;
	rec.gyro_x    = s.gyr.x;
	rec.gyro_y    = s.gyr.y;
	rec.gyro_z    = s.gyr.z;
	rec.mag_x     = s.mag.x;
	rec.mag_y     = s.mag.y;
	rec.mag_z     = s.mag.z;
	esp_err_t store_err = storage_record_append(&rec);
	if (store_err == ESP_ERR_NOT_SUPPORTED) {
		g_last_rec = rec;   /* RAM fallback */
		g_rec_valid = true;
	} else if (store_err != ESP_OK) {
		ESP_LOGW(TAG, "store: %s", esp_err_to_name(store_err));
	}

	/* Update EPD sensor overlay */
	epd_overlay_update(&s);

	/* Power down non-essential sensors between samples */
	sensors_power_save(hub);

	return s.gps_fix;
}
