#include "app_utils.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"

#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"

#include "storage_mgr.h"

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
	ESP_LOGI(TAG, ">> LOW_POWER (no motion for 5 s)");
	timer_stop(timer);
	if (w25q64 != NULL) w25q64_power_down(w25q64);
	sensor_hub_sleep(hub);
	ESP_LOGI(TAG, "waiting for any-motion…");
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
	timer_start(timer, 1);
	ESP_LOGI(TAG, "all modules restored to ACTIVE");
}

/* ========================================================================= */
/*  Light-sleep / Deep-sleep helpers                                         */
/* ========================================================================= */

void low_power_sleep_configure(void)
{
	/* BMI160 INT1 pin — any-motion active-high pulse → wakeup */
	gpio_wakeup_enable(LP_WAKEUP_GPIO, GPIO_INTR_HIGH_LEVEL);
	esp_sleep_enable_gpio_wakeup();
	ESP_LOGD(TAG, "light-sleep wakeup configured on GPIO %d",
		 LP_WAKEUP_GPIO);
}

void low_power_sleep_enter(void)
{
	/* Enter Light-sleep; returns after any enabled wakeup source fires.
	 * The BMI160 GPIO ISR (bmi160_motion_isr) runs before return,
	 * so g_motion_flag / g_motion_sem are already set. */
	esp_light_sleep_start();
}

void low_power_sleep_unconfigure(void)
{
	esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_GPIO);
	gpio_wakeup_disable(LP_WAKEUP_GPIO);
}

void low_power_deep_sleep_enter(void)
{
	ESP_LOGI(TAG, ">> DEEP_SLEEP fallback (%u s no motion)",
		 DEEP_SLEEP_FALLBACK_S);

	/* RTC timer is the only reliable Deep-sleep wakeup on ESP32-C3
	 * for non-RTC GPIOs.  GPIO %d (BMI160 INT1) is NOT an RTC GPIO,
	 * so it cannot wake Deep-sleep — we rely on periodic RTC timer
	 * wakeup to re-check the world. */
	esp_sleep_enable_timer_wakeup((uint64_t)DEEP_SLEEP_FALLBACK_S *
				      1000000ULL);

	ESP_LOGI(TAG, "entering deep-sleep — see you in %u s",
		 DEEP_SLEEP_FALLBACK_S);
	esp_deep_sleep_start();
	/* never reached */
}

/* ========================================================================= */
/*  Upload helpers                                                           */
/* ========================================================================= */

/* Fallback RAM buffer for last sample (used when no flash) */
static storage_record_t g_last_rec;
static bool g_rec_valid = false;

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

	/* Use default server config */
	storage_config_t cfg;
	storage_config_default(&cfg);
	storage_config_read(&cfg);

	/* Scan all pending flash records */
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
			/* Send failed — reconnect and retry once */
			ESP_LOGW(TAG, "send failed — reconnecting...");
			vTaskDelay(pdMS_TO_TICKS(200));
			err = ct511n_tcp_single_connect(ct511n, cfg.server_ip, cfg.server_port);
			if (err == ESP_OK) {
				err = ct511n_4g_tcp_send(ct511n, payload);
			}
			if (err == ESP_OK) {
				storage_record_mark_uploaded();
				sent++;
			} else {
				storage_record_set_last_status(STORAGE_REC_STATUS_FAILED);
				failed++;
			}
		}
		vTaskDelay(pdMS_TO_TICKS(50));
	}

	/* RAM fallback (if no flash and nothing was sent/failed) */
	if (sent == 0 && failed == 0 && g_rec_valid) {
		build_payload(payload, sizeof(payload), &g_last_rec);
		if (ct511n_4g_tcp_send(ct511n, payload) == ESP_OK) {
			sent = 1;
			g_rec_valid = false;
		}
	}

	ESP_LOGI(TAG, "upload done — %lu sent, %lu failed", (unsigned long)sent, (unsigned long)failed);
}

/* ========================================================================= */
/*  IMG_RECEIVE helpers                                                      */
/* ========================================================================= */

/** Handle to the ST7789 display (NULL until img_recv_enter). */
static st7789_handle_t *g_display = NULL;

/** W25Q64 handle passed via img_recv_enter, used by BLE callback. */
static w25q64_handle_t *g_img_flash = NULL;

/** Callback invoked by BLE component when image transfer is complete. */
static int img_recv_on_ready(uint32_t total_bytes)
{
	if (g_display == NULL) return -1;

	ESP_LOGI(TAG, "image received (%lu B) — displaying…", total_bytes);

	if (total_bytes == 0) {
		/* Re-display the last image already in Flash */
		total_bytes = ST7789_LCD_WIDTH * ST7789_LCD_HEIGHT * 2;
	}

	/* Display the image from Flash onto the screen */
	esp_err_t err = st7789_display_from_flash(g_display, 0,
						  ST7789_LCD_WIDTH,
						  ST7789_LCD_HEIGHT);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "display_from_flash: %s", esp_err_to_name(err));
		return -1;
	}

	ESP_LOGI(TAG, "image displayed");
	return 0;
}

esp_err_t img_recv_enter(spi_host_device_t host,
			 w25q64_handle_t *flash_handle)
{
	esp_err_t err;

	/* Store the flash handle for BLE callback */
	g_img_flash = flash_handle;

	/* ---- Initialise ST7789 display (SPI bus already init'd by W25Q64) ---- */
	st7789_config_t disp_cfg = {
		.host     = host,
		.cs_gpio  = ST7789_CS_GPIO,
		.dc_gpio  = ST7789_DC_GPIO,
		.rst_gpio = ST7789_RST_GPIO,
		.blk_gpio = ST7789_BLK_GPIO,
		.freq_hz  = 40 * 1000 * 1000,  /* 40 MHz */
	};
	err = st7789_init(&g_display, &disp_cfg);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "st7789_init: %s", esp_err_to_name(err));
		return err;
	}

	/* Clear screen to black */
	st7789_fill_screen(g_display, 0x0000);

	/* ---- Start BLE advertising ---- */
	err = ble_img_init(img_recv_on_ready);
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "ble_img_init: %s", esp_err_to_name(err));
		st7789_destroy(g_display);
		g_display = NULL;
		return err;
	}

	/* ---- Redirect sensor records to RAM (avoid SPI conflict) ---- */
	storage_set_ram_mode(true);
	ESP_LOGI(TAG, "IMG_RECEIVE started — BLE advertising");

	return ESP_OK;
}

void img_recv_exit(void)
{
	ESP_LOGI(TAG, "exiting IMG_RECEIVE");

	/* Stop BLE */
	ble_img_deinit();

	/* Flush RAM-cached samples to Flash, restore normal mode */
	storage_set_ram_mode(false);
	esp_err_t flush_err = ram_cache_flush();
	if (flush_err != ESP_OK) {
		ESP_LOGW(TAG, "ram_cache_flush: %s", esp_err_to_name(flush_err));
	}

	/* Turn off display */
	if (g_display != NULL) {
		st7789_display_on(g_display, false);
		st7789_destroy(g_display);
		g_display = NULL;
	}

	g_img_flash = NULL;
	ESP_LOGI(TAG, "IMG_RECEIVE done");
}

bool img_recv_poll(sensor_hub_t *hub, ct511n_handle_t *ct511n,
		   uint32_t count)
{
	/* Sample sensors into RAM cache (called at 1s intervals) */
	extern bool sample_sensors(sensor_hub_t *, ct511n_handle_t *,
				   uint32_t);
	sample_sensors(hub, ct511n, count);

	/* Return false when BLE transfer is done */
	return ble_img_is_busy();
}

/* ========================================================================= */
/*  Sample + upload (combined — store then flush pending)                    */
/* ========================================================================= */
bool sample_sensors(sensor_hub_t *hub, ct511n_handle_t *ct511n,
		    uint32_t count)
{
	sensor_sample_t s;
	sensor_hub_sample(hub, &s);

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
			 strlen(gps_buf) ? gps_buf : "—");
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

	return s.gps_fix;
}
