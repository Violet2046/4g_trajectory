#include "storage_mgr.h"

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_log.h"
#include "esp_timer.h"

#include "config.h"

#define STORAGE_TAG "storage"

/* ---- internal helpers ---- */
static uint16_t crc16_ccitt(const uint8_t *data, size_t len);

/* ------------------------------------------------------------------------- */
/*  Module-level state                                                        */
/* ------------------------------------------------------------------------- */
static w25q64_handle_t *g_flash = NULL;

/* Cached configuration (in RAM) */
static storage_config_t g_cfg;

/* RAM redirect mode — when set, record_append goes to RAM cache */
static bool g_ram_mode = false;

/* Cached write/read position */
static uint32_t g_write_page = STORAGE_DATA_START_PAGE;
static uint32_t g_read_page  = STORAGE_DATA_START_PAGE;

/* The record most recently peeked (for mark-uploaded flow) */
static uint32_t g_peek_page  = 0;
static bool     g_has_peek   = false;

/* ------------------------------------------------------------------------- */
/*  Index journal helpers                                                     */
/* ------------------------------------------------------------------------- */

/** Find the latest valid index entry. @return ESP_OK and sets g_write_page/g_read_page. */
static esp_err_t index_recover(void)
{
	uint8_t buf[STORAGE_INDEX_MAX_ENTRIES * STORAGE_INDEX_ENTRY_SIZE];
	uint32_t addr = STORAGE_PAGE_TO_ADDR(STORAGE_INDEX_START_PAGE);
	uint32_t len  = STORAGE_INDEX_PAGE_COUNT * STORAGE_PAGE_SIZE;

	esp_err_t err = w25q64_read(g_flash, addr, buf, len);
	if (err != ESP_OK) return err;

	uint32_t best_seq = 0;
	int      best_idx = -1;

	for (int i = 0; i < STORAGE_INDEX_MAX_ENTRIES; i++) {
		storage_index_entry_t *e =
			(storage_index_entry_t *)(buf + i * STORAGE_INDEX_ENTRY_SIZE);
		if (e->magic != STORAGE_INDEX_MAGIC) continue;
		if (e->seq > best_seq) {
			best_seq = e->seq;
			best_idx = i;
		}
	}

	if (best_idx >= 0) {
		storage_index_entry_t *e =
			(storage_index_entry_t *)(buf + best_idx * STORAGE_INDEX_ENTRY_SIZE);
		g_write_page = e->write_page;
		g_read_page  = e->read_page;

		/* sanity check */
		if (g_write_page < STORAGE_DATA_START_PAGE ||
		    g_write_page >= STORAGE_DATA_START_PAGE + STORAGE_DATA_PAGE_COUNT) {
			g_write_page = STORAGE_DATA_START_PAGE;
		}
		if (g_read_page < STORAGE_DATA_START_PAGE ||
		    g_read_page >= STORAGE_DATA_START_PAGE + STORAGE_DATA_PAGE_COUNT) {
			g_read_page = STORAGE_DATA_START_PAGE;
		}

		ESP_LOGI(STORAGE_TAG, "index recovered: write=0x%lX read=0x%lX seq=%lu",
			 g_write_page, g_read_page, best_seq);
	} else {
		ESP_LOGI(STORAGE_TAG, "no valid index found, starting fresh");
	}

	return ESP_OK;
}

/**
 * @brief  Append a new index entry with current write/read pointers.
 *
 * If the index area is full, erases all index pages first.
 */
static esp_err_t index_commit(void)
{
	/* Find the next free slot */
	uint8_t entry_buf[STORAGE_INDEX_ENTRY_SIZE];
	uint32_t start_addr = STORAGE_PAGE_TO_ADDR(STORAGE_INDEX_START_PAGE);
	uint32_t end_addr   = start_addr + STORAGE_INDEX_PAGE_COUNT * STORAGE_PAGE_SIZE;

	static uint32_t seq = 0;
	seq++;

	bool need_erase  = true;
	uint32_t slot_addr = start_addr;

	/* Scan for an erased (0xFF) slot */
	for (uint32_t addr = start_addr; addr < end_addr;
	     addr += STORAGE_INDEX_ENTRY_SIZE) {
		uint8_t first_byte;
		esp_err_t err = w25q64_read(g_flash, addr, &first_byte, 1);
		if (err != ESP_OK) return err;
		if (first_byte == 0xFF) {
			slot_addr  = addr;
			need_erase = false;
			break;
		}
	}

	if (need_erase) {
		ESP_LOGI(STORAGE_TAG, "index full — erasing index area");
		/* Erase all 6 pages (2 sectors of 4KB each) */
		esp_err_t err = w25q64_sector_erase(g_flash, start_addr);
		if (err != ESP_OK) return err;
		err = w25q64_sector_erase(g_flash, start_addr + STORAGE_SECTOR_SIZE);
		if (err != ESP_OK) return err;
		slot_addr = start_addr;
	}

	/* Build entry */
	storage_index_entry_t entry = {
		.magic      = STORAGE_INDEX_MAGIC,
		.seq        = seq,
		.write_page = g_write_page,
		.read_page  = g_read_page,
	};
	memcpy(entry_buf, &entry, sizeof(entry));

	return w25q64_page_program(g_flash, slot_addr, entry_buf, sizeof(entry));
}

/* ------------------------------------------------------------------------- */
/*  Config helpers                                                            */
/* ------------------------------------------------------------------------- */
static uint16_t config_crc(const storage_config_t *cfg)
{
	return crc16_ccitt((const uint8_t *)cfg, offsetof(storage_config_t, crc16));
}

static esp_err_t config_validate(const storage_config_t *cfg)
{
	if (cfg->magic != STORAGE_CONFIG_MAGIC) {
		return ESP_ERR_INVALID_STATE;
	}
	if (cfg->crc16 != config_crc(cfg)) {
		return ESP_ERR_INVALID_CRC;
	}
	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Public API — Init                                                         */
/* ------------------------------------------------------------------------- */
esp_err_t storage_init(w25q64_handle_t *flash_handle)
{
	if (flash_handle == NULL) return ESP_ERR_INVALID_ARG;

	g_flash = flash_handle;

	/* 1. Try to read config */
	esp_err_t err = storage_config_read(&g_cfg);
	if (err != ESP_OK) {
		ESP_LOGW(STORAGE_TAG, "no valid config — writing defaults");
		storage_config_default(&g_cfg);
		err = storage_config_write(&g_cfg);
		if (err != ESP_OK) {
			ESP_LOGE(STORAGE_TAG, "config write failed: %s",
				 esp_err_to_name(err));
			return err;
		}
	}

	/* 2. Recover index */
	err = index_recover();
	if (err != ESP_OK) return err;

	/* 3. Validate write pointer (page should be empty) */
	uint8_t status;
	err = w25q64_read(g_flash, STORAGE_PAGE_TO_ADDR(g_write_page),
			  &status, 1);
	if (err != ESP_OK || status != STORAGE_REC_STATUS_EMPTY) {
		/* Scan forward to find the next empty page */
		ESP_LOGW(STORAGE_TAG, "write_page invalid — scanning");
		uint32_t p = g_write_page;
		uint32_t end = STORAGE_DATA_START_PAGE + STORAGE_DATA_PAGE_COUNT;
		for (; p < end; p++) {
			w25q64_read(g_flash, STORAGE_PAGE_TO_ADDR(p), &status, 1);
			if (status == STORAGE_REC_STATUS_EMPTY) break;
		}
		if (p >= end) {
			/* All full — wrap to start */
			p = STORAGE_DATA_START_PAGE;
		}
		g_write_page = p;
	}

	/* 4. Validate read pointer */
	err = w25q64_read(g_flash, STORAGE_PAGE_TO_ADDR(g_read_page),
			  &status, 1);
	if (err != ESP_OK ||
	    (status != STORAGE_REC_STATUS_WRITTEN &&
	     status != STORAGE_REC_STATUS_UPLOADED)) {
		/* Scan forward for first non-empty, non-uploaded record */
		uint32_t p = g_read_page;
		uint32_t end = STORAGE_DATA_START_PAGE + STORAGE_DATA_PAGE_COUNT;
		for (; p < end; p++) {
			w25q64_read(g_flash, STORAGE_PAGE_TO_ADDR(p), &status, 1);
			if (status == STORAGE_REC_STATUS_WRITTEN) break;
		}
		if (p >= end) {
			p = g_write_page;  /* nothing to upload */
		}
		g_read_page = p;
	}

	/* 5. Commit recovered state */
	index_commit();

	ESP_LOGI(STORAGE_TAG, "init done — %lu/%lu records used, "
		 "sample every %lu s",
		 storage_unuploaded_count(), storage_capacity(),
		 (unsigned long)g_cfg.sample_interval_s);

	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Config                                                                    */
/* ------------------------------------------------------------------------- */
esp_err_t storage_config_read(storage_config_t *cfg)
{
	if (cfg == NULL) return ESP_ERR_INVALID_ARG;
	if (g_flash == NULL) return ESP_ERR_NOT_SUPPORTED;

	esp_err_t err = w25q64_read(g_flash,
				    STORAGE_PAGE_TO_ADDR(STORAGE_CONFIG_START_PAGE),
				    (uint8_t *)cfg, sizeof(*cfg));
	if (err != ESP_OK) return err;

	return config_validate(cfg);
}

esp_err_t storage_config_write(const storage_config_t *cfg)
{
	if (cfg == NULL) return ESP_ERR_INVALID_ARG;
	if (g_flash == NULL) return ESP_ERR_NOT_SUPPORTED;

	/* Set magic and CRC */
	storage_config_t copy = *cfg;
	copy.magic = STORAGE_CONFIG_MAGIC;
	copy.crc16  = config_crc(&copy);

	/* Erase config sector, then write */
	esp_err_t err = w25q64_sector_erase(g_flash,
				STORAGE_PAGE_TO_ADDR(STORAGE_CONFIG_START_PAGE));
	if (err != ESP_OK) return err;

	/* Write page 0 */
	err = w25q64_page_program(g_flash,
			STORAGE_PAGE_TO_ADDR(STORAGE_CONFIG_START_PAGE),
			(const uint8_t *)&copy, sizeof(copy));
	if (err != ESP_OK) return err;

	/* Also write page 1 as backup */
	err = w25q64_page_program(g_flash,
			STORAGE_PAGE_TO_ADDR(STORAGE_CONFIG_START_PAGE + 1),
			(const uint8_t *)&copy, sizeof(copy));

	return err;
}

void storage_config_default(storage_config_t *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
	cfg->sample_interval_s = STORAGE_DEFAULT_SAMPLE_INTERVAL_S;
	strncpy(cfg->server_ip,  STORAGE_DEFAULT_SERVER_IP,  sizeof(cfg->server_ip) - 1);
	strncpy(cfg->server_port, STORAGE_DEFAULT_SERVER_PORT, sizeof(cfg->server_port) - 1);
	strncpy(cfg->apn,         STORAGE_DEFAULT_APN,        sizeof(cfg->apn) - 1);
}

/* ------------------------------------------------------------------------- */
/*  Record I/O                                                                */
/* ------------------------------------------------------------------------- */

/* Track last written page for status updates (used by set_last_status) */
static uint32_t g_last_write_page = 0;
static bool     g_last_write_valid = false;

esp_err_t storage_record_append(const storage_record_t *rec)
{
	if (rec == NULL) return ESP_ERR_INVALID_ARG;

	/* RAM redirect mode: write to RAM cache instead of Flash */
	if (g_ram_mode) {
		return ram_cache_append(rec);
	}

	if (g_flash == NULL) return ESP_ERR_NOT_SUPPORTED;

	uint32_t page = g_write_page;
	uint32_t addr = STORAGE_PAGE_TO_ADDR(page);

	/* ---- Handle wrap-around / overwrite ---- */

	/* If this page currently holds an unuploaded record, we must drop it */
	uint8_t old_status;
	esp_err_t err = w25q64_read(g_flash, addr, &old_status, 1);
	if (err == ESP_OK && old_status == STORAGE_REC_STATUS_WRITTEN) {
		/* Skip it — advance read pointer past it */
		ESP_LOGW(STORAGE_TAG, "dropping unuploaded record at page %lu", page);
		g_read_page = (page + 1);
		if (g_read_page >= STORAGE_DATA_START_PAGE + STORAGE_DATA_PAGE_COUNT) {
			g_read_page = STORAGE_DATA_START_PAGE;
		}
	}

	/* ---- Erase sector (only when entering a new sector) ---- */
	/* A 4KB sector holds 16 pages.  We erase the entire sector once at
	 * the start of each 16-page group.  Subsequent pages in the same
	 * sector are already 0xFF and do not need re-erasing.  This prevents
	 * destroying adjacent data pages. */
	if ((page % 16) == 0) {
		uint32_t sector_addr = addr & ~(STORAGE_SECTOR_SIZE - 1);
		err = w25q64_sector_erase(g_flash, sector_addr);
		if (err != ESP_OK) return err;
	}

	/* ---- Write record ---- */
	storage_record_t buf = *rec;
	buf.status = STORAGE_REC_STATUS_WRITTEN;
	buf.crc16  = crc16_ccitt((const uint8_t *)&buf + 1,
				 STORAGE_REC_PAYLOAD_LEN);

	err = w25q64_page_program(g_flash, addr, (const uint8_t *)&buf, sizeof(buf));
	if (err != ESP_OK) return err;

	/* Remember this page for set_last_status */
	g_last_write_page = page;
	g_last_write_valid = true;

	/* ---- Advance write pointer ---- */
	g_write_page = page + 1;
	if (g_write_page >= STORAGE_DATA_START_PAGE + STORAGE_DATA_PAGE_COUNT) {
		g_write_page = STORAGE_DATA_START_PAGE;
	}

	/* Persist index */
	index_commit();

	return ESP_OK;
}

esp_err_t storage_record_peek(storage_record_t *rec, bool *found)
{
	if (rec == NULL || found == NULL) return ESP_ERR_INVALID_ARG;
	if (g_flash == NULL) { *found = false; return ESP_OK; }

	if (g_read_page == g_write_page) {
		*found = false;
		return ESP_OK;
	}

	uint32_t addr = STORAGE_PAGE_TO_ADDR(g_read_page);
	esp_err_t err = w25q64_read(g_flash, addr, (uint8_t *)rec, sizeof(*rec));
	if (err != ESP_OK) return err;

	if (rec->status != STORAGE_REC_STATUS_WRITTEN) {
		/* This shouldn't happen; try to recover */
		ESP_LOGW(STORAGE_TAG, "unexpected record status 0x%02X at page %lu",
			 rec->status, g_read_page);
		*found = false;
		g_read_page = g_write_page;
		index_commit();
		return ESP_OK;
	}

	/* Validate CRC */
	uint16_t calc_crc = crc16_ccitt((const uint8_t *)rec + 1,
					STORAGE_REC_PAYLOAD_LEN);
	if (calc_crc != rec->crc16) {
		ESP_LOGW(STORAGE_TAG, "CRC mismatch at page %lu — skipping",
			 g_read_page);
		g_read_page++;
		if (g_read_page >= STORAGE_DATA_START_PAGE + STORAGE_DATA_PAGE_COUNT) {
			g_read_page = STORAGE_DATA_START_PAGE;
		}
		*found = false;
		return ESP_OK;
	}

	g_peek_page = g_read_page;
	g_has_peek  = true;
	*found = true;
	return ESP_OK;
}

esp_err_t storage_record_mark_uploaded(void)
{
	if (!g_has_peek) return ESP_ERR_INVALID_STATE;
	if (g_flash == NULL) return ESP_ERR_NOT_SUPPORTED;

	uint32_t addr = STORAGE_PAGE_TO_ADDR(g_peek_page);
	uint8_t new_status = STORAGE_REC_STATUS_UPLOADED;

	/* NOR flash: programming changes 1 bits to 0 bits.
	 * 0xFE (11111110) → 0xFC (11111100) is valid: bit 1 becomes 0. */
	esp_err_t err = w25q64_page_program(g_flash, addr, &new_status, 1);
	if (err != ESP_OK) {
		ESP_LOGE(STORAGE_TAG, "mark uploaded failed: %s",
			 esp_err_to_name(err));
		return err;
	}

	/* Advance read pointer */
	g_read_page = g_peek_page + 1;
	if (g_read_page >= STORAGE_DATA_START_PAGE + STORAGE_DATA_PAGE_COUNT) {
		g_read_page = STORAGE_DATA_START_PAGE;
	}

	g_has_peek = false;
	index_commit();

	return ESP_OK;
}

esp_err_t storage_record_set_last_status(uint8_t status)
{
	if (g_flash == NULL) return ESP_ERR_NOT_SUPPORTED;
	if (!g_last_write_valid) return ESP_ERR_INVALID_STATE;

	uint32_t addr = STORAGE_PAGE_TO_ADDR(g_last_write_page);
	/* Write just the first byte (status) — NOR flash can only clear bits */
	return w25q64_page_program(g_flash, addr, &status, 1);
}

uint32_t storage_unuploaded_count(void)
{
	if (g_flash == NULL) return 0;

	if (g_read_page <= g_write_page) {
		return g_write_page - g_read_page;
	}
	/* wrapped */
	uint32_t end = STORAGE_DATA_START_PAGE + STORAGE_DATA_PAGE_COUNT;
	return (end - g_read_page) + (g_write_page - STORAGE_DATA_START_PAGE);
}

uint32_t storage_capacity(void)
{
	return STORAGE_DATA_PAGE_COUNT;
}

uint32_t storage_sample_interval_s(void)
{
	return g_cfg.sample_interval_s;
}

/* ------------------------------------------------------------------------- */
/*  CRC-16 CCITT (XMODEM)                                                    */
/* ------------------------------------------------------------------------- */
static uint16_t crc16_ccitt(const uint8_t *data, size_t len)
{
	uint16_t crc = 0x0000;
	while (len--) {
		crc ^= (uint16_t)(*data++) << 8;
		for (int i = 0; i < 8; i++) {
			if (crc & 0x8000) {
				crc = (crc << 1) ^ 0x1021;
			} else {
				crc <<= 1;
			}
		}
	}
	return crc;
}

/* ========================================================================= */
/*  Image Cache Region (Flash second half)                                    */
/* ========================================================================= */

esp_err_t img_cache_erase_all(void)
{
	if (g_flash == NULL) return ESP_ERR_NOT_SUPPORTED;

	ESP_LOGI(STORAGE_TAG, "erasing image cache region (4 MB)…");
	uint32_t addr = IMG_CACHE_BASE_ADDR;
	uint32_t end  = IMG_CACHE_BASE_ADDR + IMG_CACHE_SIZE;

	/* Erase sector by sector (4 KB each) */
	while (addr < end) {
		esp_err_t err = w25q64_sector_erase(g_flash, addr);
		if (err != ESP_OK) {
			ESP_LOGE(STORAGE_TAG, "sector erase @ 0x%lX: %s",
				 addr, esp_err_to_name(err));
			return err;
		}
		addr += W25Q64_SECTOR_SIZE;
	}
	ESP_LOGI(STORAGE_TAG, "image cache erase done");
	return ESP_OK;
}

esp_err_t img_cache_erase_sector(uint32_t offset)
{
	if (g_flash == NULL) return ESP_ERR_NOT_SUPPORTED;
	if (offset >= IMG_CACHE_SIZE || (offset & (W25Q64_SECTOR_SIZE - 1)) != 0) {
		return ESP_ERR_INVALID_ARG;
	}
	uint32_t flash_addr = IMG_CACHE_BASE_ADDR + offset;
	return w25q64_sector_erase(g_flash, flash_addr);
}

esp_err_t img_cache_write(uint32_t offset, const uint8_t *data, size_t len)
{
	if (g_flash == NULL) return ESP_ERR_NOT_SUPPORTED;
	if (offset + len > IMG_CACHE_SIZE) return ESP_ERR_INVALID_ARG;

	uint32_t addr = IMG_CACHE_BASE_ADDR + offset;
	/* Loop across 256-byte page boundaries — never truncate */
	while (len > 0) {
		size_t max_write = W25Q64_PAGE_SIZE - (addr % W25Q64_PAGE_SIZE);
		size_t chunk = (len < max_write) ? len : max_write;
		esp_err_t err = w25q64_page_program(g_flash, addr, data, chunk);
		if (err != ESP_OK) return err;
		addr   += chunk;
		data   += chunk;
		len    -= chunk;
	}
	return ESP_OK;
}

esp_err_t img_cache_read(uint32_t offset, uint8_t *data, size_t len)
{
	if (g_flash == NULL) return ESP_ERR_NOT_SUPPORTED;
	if (offset + len > IMG_CACHE_SIZE) return ESP_ERR_INVALID_ARG;

	return w25q64_read(g_flash, IMG_CACHE_BASE_ADDR + offset, data, len);
}

/* ========================================================================= */
/*  RAM mode switch                                                          */
/* ========================================================================= */

void storage_set_ram_mode(bool enable)
{
	g_ram_mode = enable;
	if (enable) {
		ram_cache_init();
		ESP_LOGI(STORAGE_TAG, "RAM mode ON — records go to RAM cache");
	} else {
		ESP_LOGI(STORAGE_TAG, "RAM mode OFF — records go to Flash");
	}
}

/* ========================================================================= */
/*  RAM Cache (circular buffer for samples during IMG_RECEIVE)               */
/* ========================================================================= */

static storage_record_t g_ram_cache[RAM_CACHE_CAPACITY];
static uint32_t g_ram_write = 0;
static uint32_t g_ram_count = 0;

void ram_cache_init(void)
{
	g_ram_write = 0;
	g_ram_count = 0;
}

esp_err_t ram_cache_append(const storage_record_t *rec)
{
	if (rec == NULL) return ESP_ERR_INVALID_ARG;

	g_ram_cache[g_ram_write] = *rec;
	g_ram_write = (g_ram_write + 1) % RAM_CACHE_CAPACITY;

	if (g_ram_count < RAM_CACHE_CAPACITY) {
		g_ram_count++;
	} /* else: oldest record silently overwritten */

	return ESP_OK;
}

uint32_t ram_cache_count(void)
{
	return g_ram_count;
}

esp_err_t ram_cache_flush(void)
{
	if (g_flash == NULL) return ESP_ERR_NOT_SUPPORTED;
	if (g_ram_count == 0) return ESP_OK;

	ESP_LOGI(STORAGE_TAG, "flushing %lu RAM-cached records to Flash…",
		 g_ram_count);

	uint32_t start = (g_ram_write >= g_ram_count)
			 ? (g_ram_write - g_ram_count)
			 : (RAM_CACHE_CAPACITY + g_ram_write - g_ram_count);
	start %= RAM_CACHE_CAPACITY;

	esp_err_t last_err = ESP_OK;
	for (uint32_t i = 0; i < g_ram_count; i++) {
		uint32_t idx = (start + i) % RAM_CACHE_CAPACITY;
		esp_err_t err = storage_record_append(&g_ram_cache[idx]);
		if (err != ESP_OK) {
			ESP_LOGW(STORAGE_TAG, "flush rec %lu: %s",
				 i, esp_err_to_name(err));
			last_err = err;
			/* Continue flushing remaining records */
		}
	}

	ram_cache_init();
	ESP_LOGI(STORAGE_TAG, "RAM cache flush done (last_err=%d)", last_err);
	return last_err;
}
