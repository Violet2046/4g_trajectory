#include "ble_img_rx.h"

#include <string.h>
#include <stdio.h>

#include "config.h"

/* BLE advertising interval (adjustable) — NimBLE units: 0.625 ms */
#ifndef CFG_BLE_ADV_ITVL_MIN
#define CFG_BLE_ADV_ITVL_MIN  1600   /* 1.0 s */
#endif
#ifndef CFG_BLE_ADV_ITVL_MAX
#define CFG_BLE_ADV_ITVL_MAX  3200   /* 2.0 s */
#endif

#include "esp_log.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

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
#include "host/ble_store.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
/* BT controller API */
#include "esp_bt.h"
#endif /* CONFIG_BT_NIMBLE_ENABLED */

/* Forward declaration of storage_mgr img_cache functions */
extern esp_err_t img_cache_write(uint32_t offset,
				 const uint8_t *data, size_t len);
extern esp_err_t img_cache_erase_all(void);
extern esp_err_t img_cache_erase_sector(uint32_t offset);

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
static ble_img_ready_cb_t g_ready_cb_backup = NULL;  /* 用于 resume 重建时恢复 */
static SemaphoreHandle_t g_adv_stop_sem = NULL;       /* 同步 ble_adv_stop */
static bool g_busy = false;
static bool g_transfer_active = false;
static bool g_sleeping = false;   /* set by ble_adv_stop; prevents DISCONNECT from restarting adv */
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
	uint32_t chunk_len = OS_MBUF_PKTLEN(ctxt->om);
	if (chunk_len == 0) return 0;

	/* 🔥 占线锁 0xFA：网页端准备传输前先发此信号预锁 BLE 忙碌状态，
	 * 阻止 4G/WiFi 上传干扰。不写入 Flash，仅设置状态。 */
	if (chunk_len == 1) {
		uint8_t first;
		os_mbuf_copydata(ctxt->om, 0, 1, &first);
		if (first == 0xFA) {
			ESP_LOGW(BLE_IMG_TAG, "🔔 0xFA lock — BLE busy locked, blocking 4G");
			g_busy = true;
			g_received_bytes = 0;
			return 0;
		}
	}

	if (!g_transfer_active) {
		ESP_LOGW(BLE_IMG_TAG, "data write but no transfer active");
		return 0;
	}

	/* 安全防线：禁止超过本地栈缓冲区大小，防止栈溢出崩溃 */
	if (chunk_len > BLE_IMG_CHUNK_SIZE_MAX) {
		ESP_LOGE(BLE_IMG_TAG, "chunk %lu > max %d — rejected",
			 chunk_len, BLE_IMG_CHUNK_SIZE_MAX);
		return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
	}

	if (g_received_bytes + chunk_len > g_total_bytes) {
		chunk_len = g_total_bytes - g_received_bytes;
	}

	uint8_t buf[BLE_IMG_CHUNK_SIZE_MAX];
	os_mbuf_copydata(ctxt->om, 0, chunk_len, buf);

	/* 注意：不再在传输中动态擦除扇区。整片擦除在 IMG_CMD_START
	 * 时通过 img_cache_erase_all() 一次性完成。 */

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
	case IMG_CMD_FAKE_BUSY:
		/* 占线锁：仅设状态，不警告、不涉及传输状态 */
		g_busy = true;
		g_received_bytes = 0;
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
		ESP_LOGI(BLE_IMG_TAG, "disconnected");
		g_transfer_active = false;
		g_busy = false;
		if (g_sleeping) {
			ESP_LOGI(BLE_IMG_TAG, "sleeping — don't restart adv");
			break;
		}
		ESP_LOGI(BLE_IMG_TAG, "restarting adv");
		ble_adv_start();
		break;
	case BLE_GAP_EVENT_ADV_COMPLETE:
		ESP_LOGI(BLE_IMG_TAG, "advertising complete (reason=%d)",
			 event->adv_complete.reason);
		if (g_adv_stop_sem) xSemaphoreGive(g_adv_stop_sem);
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
			.flags = BLE_GATT_CHR_F_WRITE_NO_RSP,
			.val_handle = &g_data_chr_val_handle,
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
			.flags = BLE_GATT_CHR_F_WRITE,
			.val_handle = &g_ctrl_chr_val_handle,
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
/*  Sync callback  --  called when NimBLE host is ready                     */
/* ========================================================================= */

#ifdef CONFIG_BT_NIMBLE_ENABLED
static uint8_t g_own_addr_type;

/* Store status callback — the default handler prints
 * "Failed to restore IRKs from store; status=8" and may leave the
 * host in a bad state when NVS is corrupted.  We just log and
 * move on — actual store data is not critical for this device. */
static int ble_img_store_status(struct ble_store_status_event *event, void *arg)
{
	(void)arg;
	ESP_LOGV(BLE_IMG_TAG, "store event: %d", event->event_code);
	return 0;
}

static void ble_img_on_sync(void)
{
	int rc;

	ble_store_clear();

	rc = ble_hs_id_infer_auto(0, &g_own_addr_type);
	if (rc != 0) {
		g_own_addr_type = BLE_OWN_ADDR_PUBLIC;
	}

	ble_svc_gap_device_name_set("4G-Tracker");

	struct ble_hs_adv_fields adv_fields = { 0 };
	adv_fields.flags = BLE_HS_ADV_F_DISC_GEN |
			   BLE_HS_ADV_F_BREDR_UNSUP;
	adv_fields.name = (uint8_t *)"4G-Track";
	adv_fields.name_len = 8;
	adv_fields.name_is_complete = 1;

	rc = ble_gap_adv_set_fields(&adv_fields);
	if (rc != 0) {
		ESP_LOGE(BLE_IMG_TAG, "adv_set_fields: %d", rc);
		return;
	}

	ble_adv_start();
}
#endif

/* ========================================================================= */
/*  Public API                                                               */
/* ========================================================================= */

static bool g_bt_ctrl_inited = false;
static bool g_ble_init_ok    = false; /* set only after successful host init */

esp_err_t ble_img_ctrl_init(void)
{
#ifndef CONFIG_BT_NIMBLE_ENABLED
	return ESP_ERR_NOT_SUPPORTED;
#else
	/* If already enabled, nothing to do */
	if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
		g_bt_ctrl_inited = true;
		return ESP_OK;
	}

	/* If inited but not enabled, just enable */
	if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED) {
		g_bt_ctrl_inited = true;
		esp_err_t ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
		if (ret == ESP_OK) {
			ESP_LOGI(BLE_IMG_TAG, "BT controller enabled at boot");
		} else {
			ESP_LOGW(BLE_IMG_TAG, "BT controller enable: %s", esp_err_to_name(ret));
		}
		return ret;
	}

	/* First-time init + enable */
	esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
	esp_err_t ret = esp_bt_controller_init(&cfg);
	if (ret != ESP_OK) {
		ESP_LOGW(BLE_IMG_TAG, "BT controller init: %s", esp_err_to_name(ret));
		return ret;
	}
	g_bt_ctrl_inited = true;
	ESP_LOGI(BLE_IMG_TAG, "BT controller pre-initialized at boot");

	ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
	if (ret != ESP_OK) {
		ESP_LOGW(BLE_IMG_TAG, "BT controller enable: %s", esp_err_to_name(ret));
		return ret;
	}
	ESP_LOGI(BLE_IMG_TAG, "BT controller enabled");
	return ESP_OK;
#endif
}

esp_err_t ble_img_init(ble_img_ready_cb_t img_ready_cb)
{
#ifndef CONFIG_BT_NIMBLE_ENABLED
	ESP_LOGW(BLE_IMG_TAG, "BLE not configured — enable in menuconfig");
	return ESP_ERR_NOT_SUPPORTED;
#else
	esp_err_t err;

	g_ready_cb_backup = img_ready_cb;   /* 备份，供 resume 重建时使用 */

	/* NVS only init once — skip if already done */
	static bool nvs_done = false;
	if (!nvs_done) {
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
		nvs_done = true;
	}

	g_ready_cb = img_ready_cb;
	ble_hs_cfg.sync_cb = ble_img_on_sync;
	ble_hs_cfg.store_status_cb = ble_img_store_status;

	/*
	 * Controller is already init'ed + enabled by ble_img_ctrl_init()
	 * (called once at boot).  Just initialise the NimBLE host stack.
	 */
	if (g_bt_ctrl_inited) {
		err = esp_nimble_init();
		if (err != ESP_OK) {
			ESP_LOGE(BLE_IMG_TAG, "host init: %s", esp_err_to_name(err));
			return err;
		}
	} else {
		err = nimble_port_init();
		if (err != ESP_OK) {
			ESP_LOGE(BLE_IMG_TAG, "nimble_port_init: %s", esp_err_to_name(err));
			return err;
		}
	}

	/* Register GATT services BEFORE starting the host task (must be done
	 * after host init but before nimble_port_run()). */
	ble_svc_gap_init();
	ble_svc_gatt_init();

	int rc = ble_gatts_count_cfg(ble_img_gatt_svcs);
	if (rc == 0) {
		rc = ble_gatts_add_svcs(ble_img_gatt_svcs);
	}
	if (rc != 0) {
		ESP_LOGE(BLE_IMG_TAG, "Failed to register GATT services: %d", rc);
		return ESP_FAIL;
	}

	/* Only create host task if init succeeded */
	g_ble_init_ok = true;
	nimble_port_freertos_init(ble_img_host_task);

	ESP_LOGI(BLE_IMG_TAG, "BLE init OK — host task started");
	return ESP_OK;
#endif
}

void ble_img_deinit(void)
{
#ifndef CONFIG_BT_NIMBLE_ENABLED
	return;
#else
	g_ready_cb = NULL;
	g_ready_cb_backup = NULL;
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

esp_err_t ble_adv_start(void)
{
#ifndef CONFIG_BT_NIMBLE_ENABLED
	return ESP_ERR_NOT_SUPPORTED;
#else
	/* If host is not enabled, we cannot advertise */
	if (!ble_hs_is_enabled()) {
		ESP_LOGW(BLE_IMG_TAG, "BLE host not enabled — adv_start deferred");
		return ESP_ERR_INVALID_STATE;
	}

	struct ble_gap_adv_params adv_params = { 0 };
	adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
	adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
	adv_params.itvl_min = CFG_BLE_ADV_ITVL_MIN;   /* config.h: units 0.625 ms */
	adv_params.itvl_max = CFG_BLE_ADV_ITVL_MAX;
	g_sleeping = false;

	int rc = ble_gap_adv_start(g_own_addr_type, NULL, BLE_HS_FOREVER,
				   &adv_params, ble_img_gap_event, NULL);
	if (rc != 0) {
		ESP_LOGE(BLE_IMG_TAG, "adv_start failed: %d", rc);
		return ESP_FAIL;
	}
	ESP_LOGI(BLE_IMG_TAG, "advertising started");
	return ESP_OK;
#endif
}

void ble_adv_stop(void)
{
#ifndef CONFIG_BT_NIMBLE_ENABLED
	return;
#else
	g_sleeping = true;
	if (!ble_hs_is_enabled()) {
		ESP_LOGW(BLE_IMG_TAG, "BLE host not enabled — skip adv_stop");
		return;
	}
	/* 信号量同步：等待 ADV_COMPLETE 事件确认停止完成 */
	if (!g_adv_stop_sem) g_adv_stop_sem = xSemaphoreCreateBinary();
	xSemaphoreTake(g_adv_stop_sem, 0);  /* 清空可能残留的信号 */
	int rc = ble_gap_adv_stop();
	if (rc == 0) {
		if (xSemaphoreTake(g_adv_stop_sem, pdMS_TO_TICKS(1000)) != pdTRUE) {
			ESP_LOGI(BLE_IMG_TAG, "adv_stop: no complete event (timeout), assuming stopped");
		}
	} else {
		ESP_LOGW(BLE_IMG_TAG, "ble_gap_adv_stop: %d", rc);
	}
	ESP_LOGI(BLE_IMG_TAG, "advertising stopped");
#endif
}

uint32_t ble_img_bytes_received(void)
{
	return g_received_bytes;
}

uint32_t ble_img_total_expected(void)
{
	return g_total_bytes;
}

void ble_img_pause(void)
{
#ifndef CONFIG_BT_NIMBLE_ENABLED
	return;
#else
	ESP_LOGI(BLE_IMG_TAG, "pause — full BLE release for WiFi");
	ble_img_deinit();                    /* 停止 host task + deinit host */
	esp_bt_controller_disable();         /* 断电控制器 */
	esp_bt_controller_deinit();          /* 释放 32KB EM 内存 */
	g_bt_ctrl_inited = false;
	g_ble_init_ok = false;
#endif
}

esp_err_t ble_img_resume(void)
{
#ifndef CONFIG_BT_NIMBLE_ENABLED
	return ESP_ERR_NOT_SUPPORTED;
#else
	ESP_LOGI(BLE_IMG_TAG, "resume — reinit BLE from scratch");
	/* 1. 重新初始化并启用控制器 */
	esp_err_t ret = ble_img_ctrl_init();
	if (ret != ESP_OK) {
		ESP_LOGE(BLE_IMG_TAG, "ctrl reinit failed: %s", esp_err_to_name(ret));
		return ret;
	}
	/* 2. 重新初始化 NimBLE host + GATT + 广播 */
	return ble_img_init(g_ready_cb_backup);
#endif
}

