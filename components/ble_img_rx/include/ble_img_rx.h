#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Protocol constants ---- */
#define BLE_IMG_CHUNK_SIZE_MAX   512

/* Image control commands */
typedef enum {
    IMG_CMD_START     = 0x01,
    IMG_CMD_DATA      = 0x02,
    IMG_CMD_DONE      = 0x03,
    IMG_CMD_CANCEL    = 0x04,
    IMG_CMD_DISPLAY   = 0x05,
    IMG_CMD_FAKE_BUSY = 0xFA,   /* 占线锁：网页端准备传输前预锁 BLE */
} img_cmd_t;

/* Transfer status */
typedef enum {
    IMG_STATUS_OK          = 0x00,
    IMG_STATUS_BUSY        = 0x01,
    IMG_STATUS_CRC_ERR     = 0x02,
    IMG_STATUS_OOM         = 0x03,
    IMG_STATUS_DISPLAY_OK  = 0x10,
    IMG_STATUS_DISPLAY_ERR = 0x11,
} img_status_t;

/* ---- Callback ---- */
typedef int (*ble_img_ready_cb_t)(uint32_t total_bytes);

/* ---- API ---- */

/**
 * @brief  Initialize BT controller at boot (clean heap) — call before WiFi init.
 *         After this, ble_img_init() will skip controller init and use existing.
 * @return ESP_OK on success
 */
esp_err_t ble_img_ctrl_init(void);

esp_err_t ble_img_init(ble_img_ready_cb_t ready_cb);
void     ble_img_deinit(void);
bool     ble_img_is_busy(void);
esp_err_t ble_adv_start(void);
void     ble_adv_stop(void);
uint32_t ble_img_bytes_received(void);
uint32_t ble_img_total_expected(void);

/**
 * @brief  Pause BLE — stop advertising + deinit NimBLE (frees ~50KB heap).
 *         Call ble_img_resume() to restore.
 */
void ble_img_pause(void);

/**
 * @brief  Resume BLE after pause — re-init NimBLE + start advertising.
 */
esp_err_t ble_img_resume(void);

#ifdef __cplusplus
}
#endif