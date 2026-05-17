#include "app_utils.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
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
