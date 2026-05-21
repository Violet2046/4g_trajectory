#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*  BLE Image Receiver — NimBLE GATT server for over-the-air image upload   */
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/*  BLE Service UUIDs (custom)                                               */
/* ------------------------------------------------------------------------- */
#define BLE_IMG_SERVICE_UUID        0x180F  /* Use a custom 128-bit UUID?
                                               For simplicity we use a
                                               standard Battery Service
                                               as placeholder — replace
                                               with a custom UUID in
                                               production. */

/* Custom 128-bit UUID for image transfer service */
extern const uint8_t BLE_IMG_SVC_UUID128[16];
extern const uint8_t BLE_IMG_DATA_CHR_UUID128[16];
extern const uint8_t BLE_IMG_CTRL_CHR_UUID128[16];

/* ------------------------------------------------------------------------- */
/*  Image transfer protocol                                                   */
/* ------------------------------------------------------------------------- */
#define BLE_IMG_CHUNK_SIZE_MAX   512   /* max bytes per BLE write          */

/** Image control commands (written to CTRL characteristic) */
typedef enum {
	IMG_CMD_START     = 0x01,   /* [len:4] start transfer: total_bytes  */
	IMG_CMD_DATA      = 0x02,   /* [len:4][data:len] image chunk        */
	IMG_CMD_DONE      = 0x03,   /* transfer complete, trigger display    */
	IMG_CMD_CANCEL    = 0x04,   /* cancel current transfer               */
	IMG_CMD_DISPLAY   = 0x05,   /* display last received image           */
} img_cmd_t;

/** Image transfer status (notified to phone) */
typedef enum {
	IMG_STATUS_OK          = 0x00,
	IMG_STATUS_BUSY        = 0x01,
	IMG_STATUS_CRC_ERR     = 0x02,
	IMG_STATUS_OOM         = 0x03,
	IMG_STATUS_DISPLAY_OK  = 0x10,
	IMG_STATUS_DISPLAY_ERR = 0x11,
} img_status_t;

/* ------------------------------------------------------------------------- */
/*  Callbacks                                                                */
/* ------------------------------------------------------------------------- */

/** Called when a full image has been received and stored in Flash.
 *  The callee should display the image and return 0 on success. */
typedef int (*ble_img_ready_cb_t)(uint32_t total_bytes);

/* ------------------------------------------------------------------------- */
/*  Public API                                                               */
/* ------------------------------------------------------------------------- */

/**
 * @brief  Initialise NimBLE and start advertising.
 *
 * @param  img_ready_cb  callback invoked when a complete image is received.
 * @return ESP_OK on success.
 */
esp_err_t ble_img_init(ble_img_ready_cb_t img_ready_cb);

/**
 * @brief  Stop BLE advertising and de-initialise.
 */
void ble_img_deinit(void);

/**
 * @brief  Check whether an image transfer is in progress.
 */
bool ble_img_is_busy(void);

/**
 * @brief  Get the number of bytes received so far in the current transfer.
 */
uint32_t ble_img_bytes_received(void);

/**
 * @brief  Get the total expected bytes for the current transfer.
 */
uint32_t ble_img_total_expected(void);

#ifdef __cplusplus
}
#endif
