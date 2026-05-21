#include "ble_img_rx.h"

#include <string.h>
#include <stdio.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"

/*
 * NimBLE headers are only available when CONFIG_BT_NIMBLE_ENABLED is set
 * in menuconfig.  When disabled, this file compiles stubs that return
 * ESP_ERR_NOT_SUPPORTED — allowing the rest of the firmware to build
 * without BLE.
 */
#ifdef CONFIG_BT_NIMBLE_ENABLED
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#endif /* CONFIG_BT_NIMBLE_ENABLED */

/* Forward declaration of storage_mgr img_cache functions */
extern esp_err_t img_cache_write(uint32_t offset,
				 const uint8_t *data, size_t len);
extern esp_err_t img_cache_erase_all(void);

#define BLE_IMG_TAG "ble_img"

/* ------------------------------------------------------------------------- */
/*  Custom UUIDs                                                             */
/* ------------------------------------------------------------------------- */
const uint8_t BLE_IMG_SVC_UUID128[16] = {
	0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
	0x00, 0x10, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00,
};
const uint8_t BLE_IMG_DATA_CHR_UUID128[16] = {
	0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
	0x00, 0x10, 0x00, 0x00, 0xFF, 0x01, 0x00, 0x00,
};
const uint8_t BLE_IMG_CTRL_CHR_UUID128[16] = {
	0xFB, 0x34, 0x9B, 0x5F, 0x80, 0x00, 0x00, 0x80,
	0x00, 0x10, 0x00, 0x00, 0xFF, 0x02, 0x00, 0x00,
};

/* ------------------------------------------------------------------------- */
/*  Module state                                                              */
/* ------------------------------------------------------------------------- */
static ble_img_ready_cb_t g_ready_cb = NULL;
static bool g_busy = false;
static bool g_transfer_active = false;
static uint32_t g_total_bytes = 0;
static uint32_t g_received_bytes = 0;
static uint32_t g_write_offset = 0;

#ifdef CONFIG_BT_NIMBLE_ENABLED

static uint16_t g_data_chr_val_handle;
static uint16_t g_ctrl_chr_val_handle;

/* ---- Image data write handler ----------------------------------------- */

static int img_data_write(uint16_t conn_handle,
			  struct ble_gatt_access_ctxt *ctxt)
{
	if (!g_transfer_active) {
		ESP_LOGW(BLE_IMG_TAG, "data write but no transfer active");
		return 0;
	}

	uint32_t chunk_len = OS_MBUF_PKTLEN(ctxt->om);
	if (chunk_len == 0) return 0;

	if (g_received_bytes + chunk_len > g_total_bytes) {
		chunk_len = g_total_bytes - g_received_bytes;
	}

	uint8_t buf[BLE_IMG_CHUNK_SIZE_MAX];
	os_mbuf_copydata(ctxt->om, 0, chunk_len, buf);

	esp_err_t err = img_cache_write(g_write_offset, buf, chunk_len);
	if (err != ESP_OK) {
		ESP_LOGE(BLE_IMG_TAG, "img_cache_write: %s",
			 esp_err_to_name(err));
		return BLE_ATT_ERR_INSUFFICIENT_RES;
	}

	g_write_offset += chunk_len;
	g_received_bytes += chunk_len;
	return 0;
}

/* ---- Control command handler ------------------------------------------ */

static int img_ctrl_write(uint16_t conn_handle,
			  struct ble_gatt_access_ctxt *ctxt)
{
	uint8_t buf[8] = {0};
	uint32_t len = OS_MBUF_PKTLEN(ctxt->om);
	if (len < 1) return 0;

	os_mbuf_copydata(ctxt->om, 0, len > 8 ? 8 : len, buf);
	uint8_t cmd = buf[0];

	switch (cmd) {
	case IMG_CMD_START: {
		if (len < 5) return 0;
		uint32_t total = ((uint32_t)buf[1] << 24) |
				 ((uint32_t)buf[2] << 16) |
				 ((uint32_t)buf[3] << 8)  |
				 (uint32_t)buf[4];
		if (total == 0 || total > 4 * 1024 * 1024)
			return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
		img_cache_erase_all();
		g_total_bytes = total;
		g_received_bytes = 0;
		g_write_offset = 0;
		g_transfer_active = true;
		g_busy = true;
		ESP_LOGI(BLE_IMG_TAG, "transfer start: %lu bytes", total);
		break;
	}
	case IMG_CMD_DONE:
		if (!g_transfer_active) break;
		g_transfer_active = false;
		ESP_LOGI(BLE_IMG_TAG, "transfer done: %lu bytes",
			 g_received_bytes);
		if (g_ready_cb) g_ready_cb(g_received_bytes);
		g_busy = false;
		break;
	case IMG_CMD_CANCEL:
		g_transfer_active = false;
		g_busy = false;
		ESP_LOGI(BLE_IMG_TAG, "transfer cancelled");
		break;
	case IMG_CMD_DISPLAY:
		if (g_ready_cb) g_ready_cb(0);
		break;
	default:
		ESP_LOGW(BLE_IMG_TAG, "unknown cmd: 0x%02X", cmd);
		break;
	}
	return 0;
}

/* ---- GATT access callback --------------------------------------------- */

static int ble_img_gatt_access(uint16_t conn_handle, uint16_t attr_handle,
			       struct ble_gatt_access_ctxt *ctxt, void *arg)
{
	switch (ctxt->op) {
	case BLE_GATT_ACCESS_OP_WRITE_CHR:
		if (attr_handle == g_data_chr_val_handle)
			return img_data_write(conn_handle, ctxt);
		if (attr_handle == g_ctrl_chr_val_handle)
			return img_ctrl_write(conn_handle, ctxt);
		break;
	default:
		break;
	}
	return 0;
}

/* ---- GAP event handler ------------------------------------------------ */

static int ble_img_gap_event(struct ble_gap_event *event, void *arg)
{
	switch (event->type) {
	case BLE_GAP_EVENT_CONNECT:
		ESP_LOGI(BLE_IMG_TAG, "connected (handle=%d)",
			 event->connect.conn_handle);
		break;
	case BLE_GAP_EVENT_DISCONNECT:
		ESP_LOGI(BLE_IMG_TAG, "disconnected — restarting adv");
		g_transfer_active = false;
		g_busy = false;
		ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, 0,
				  NULL, ble_img_gap_event, NULL);
		break;
	case BLE_GAP_EVENT_ADV_COMPLETE:
		ESP_LOGI(BLE_IMG_TAG, "advertising complete");
		break;
	default:
		break;
	}
	return 0;
}

/* ---- GATT service definition ------------------------------------------ */

static const struct ble_gatt_svc_def ble_img_gatt_svcs[] = {
	{
		.type = BLE_GATT_SVC_TYPE_PRIMARY,
		.uuid = BLE_UUID128_DECLARE(
			BLE_IMG_SVC_UUID128[15], BLE_IMG_SVC_UUID128[14],
			BLE_IMG_SVC_UUID128[13], BLE_IMG_SVC_UUID128[12],
			BLE_IMG_SVC_UUID128[11], BLE_IMG_SVC_UUID128[10],
			BLE_IMG_SVC_UUID128[9],  BLE_IMG_SVC_UUID128[8],
			BLE_IMG_SVC_UUID128[7],  BLE_IMG_SVC_UUID128[6],
			BLE_IMG_SVC_UUID128[5],  BLE_IMG_SVC_UUID128[4],
			BLE_IMG_SVC_UUID128[3],  BLE_IMG_SVC_UUID128[2],
			BLE_IMG_SVC_UUID128[1],  BLE_IMG_SVC_UUID128[0]),
		.characteristics = (struct ble_gatt_chr_def[]) { {
			.uuid = BLE_UUID128_DECLARE(
				BLE_IMG_DATA_CHR_UUID128[15],
				BLE_IMG_DATA_CHR_UUID128[14],
				BLE_IMG_DATA_CHR_UUID128[13],
				BLE_IMG_DATA_CHR_UUID128[12],
				BLE_IMG_DATA_CHR_UUID128[11],
				BLE_IMG_DATA_CHR_UUID128[10],
				BLE_IMG_DATA_CHR_UUID128[9],
				BLE_IMG_DATA_CHR_UUID128[8],
				BLE_IMG_DATA_CHR_UUID128[7],
				BLE_IMG_DATA_CHR_UUID128[6],
				BLE_IMG_DATA_CHR_UUID128[5],
				BLE_IMG_DATA_CHR_UUID128[4],
				BLE_IMG_DATA_CHR_UUID128[3],
				BLE_IMG_DATA_CHR_UUID128[2],
				BLE_IMG_DATA_CHR_UUID128[1],
				BLE_IMG_DATA_CHR_UUID128[0]),
			.access_cb = ble_img_gatt_access,
			.flags = BLE_GATT_CHR_F_WRITE |
				 BLE_GATT_CHR_F_WRITE_ENC,
		}, {
			.uuid = BLE_UUID128_DECLARE(
				BLE_IMG_CTRL_CHR_UUID128[15],
				BLE_IMG_CTRL_CHR_UUID128[14],
				BLE_IMG_CTRL_CHR_UUID128[13],
				BLE_IMG_CTRL_CHR_UUID128[12],
				BLE_IMG_CTRL_CHR_UUID128[11],
				BLE_IMG_CTRL_CHR_UUID128[10],
				BLE_IMG_CTRL_CHR_UUID128[9],
				BLE_IMG_CTRL_CHR_UUID128[8],
				BLE_IMG_CTRL_CHR_UUID128[7],
				BLE_IMG_CTRL_CHR_UUID128[6],
				BLE_IMG_CTRL_CHR_UUID128[5],
				BLE_IMG_CTRL_CHR_UUID128[4],
				BLE_IMG_CTRL_CHR_UUID128[3],
				BLE_IMG_CTRL_CHR_UUID128[2],
				BLE_IMG_CTRL_CHR_UUID128[1],
				BLE_IMG_CTRL_CHR_UUID128[0]),
			.access_cb = ble_img_gatt_access,
			.flags = BLE_GATT_CHR_F_WRITE |
				 BLE_GATT_CHR_F_WRITE_ENC,
		}, { 0 } },
	},
	{ 0 },
};

/* ---- NimBLE host task ------------------------------------------------- */

static void ble_img_host_task(void *param)
{
	ESP_LOGI(BLE_IMG_TAG, "NimBLE host task started");
	nimble_port_run();
	nimble_port_freertos_deinit();
}

#endif /* CONFIG_BT_NIMBLE_ENABLED */

/* ========================================================================= */
/*  Public API                                                               */
/* ========================================================================= */

esp_err_t ble_img_init(ble_img_ready_cb_t img_ready_cb)
{
#ifndef CONFIG_BT_NIMBLE_ENABLED
	ESP_LOGW(BLE_IMG_TAG, "BLE not configured — enable in menuconfig");
	return ESP_ERR_NOT_SUPPORTED;
#else
	esp_err_t err;

	err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
	    err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		nvs_flash_erase();
		err = nvs_flash_init();
	}
	if (err != ESP_OK) {
		ESP_LOGE(BLE_IMG_TAG, "nvs_flash_init: %s",
			 esp_err_to_name(err));
		return err;
	}

	g_ready_cb = img_ready_cb;
	nimble_port_init();
	ble_svc_gap_device_name_set("4G-Tracker");

	err = ble_gatts_count_cfg(ble_img_gatt_svcs);
	if (err != 0) { nimble_port_deinit(); return ESP_FAIL; }
	err = ble_gatts_add_svcs(ble_img_gatt_svcs);
	if (err != 0) { nimble_port_deinit(); return ESP_FAIL; }

	struct ble_hs_adv_fields adv_fields = { 0 };
	adv_fields.flags = BLE_HS_ADV_F_DISC_GEN |
			   BLE_HS_ADV_F_BREDR_UNSUP;
	adv_fields.tx_pwr_lvl_is_present = 1;
	adv_fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
	adv_fields.name = (uint8_t *)"4G-Tracker";
	adv_fields.name_len = strlen("4G-Tracker");
	adv_fields.name_is_complete = 1;

	err = ble_gap_adv_set_fields(&adv_fields);
	if (err != 0) { nimble_port_deinit(); return ESP_FAIL; }

	err = ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, 0,
				NULL, ble_img_gap_event, NULL);
	if (err != 0) { nimble_port_deinit(); return ESP_FAIL; }

	nimble_port_freertos_init(ble_img_host_task);
	ESP_LOGI(BLE_IMG_TAG, "advertising as \"4G-Tracker\"");
	return ESP_OK;
#endif
}

void ble_img_deinit(void)
{
#ifndef CONFIG_BT_NIMBLE_ENABLED
	return;
#else
	g_ready_cb = NULL;
	g_busy = false;
	g_transfer_active = false;
	nimble_port_stop();
	nimble_port_deinit();
#endif
}

bool ble_img_is_busy(void)
{
	return g_busy;
}

uint32_t ble_img_bytes_received(void)
{
	return g_received_bytes;
}

uint32_t ble_img_total_expected(void)
{
	return g_total_bytes;
}

