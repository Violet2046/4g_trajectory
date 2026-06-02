#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "esp_err.h"

#include "W25Q64.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*  ROM Layout                                                               */
/*                                                                           */
/*  Sector 0 (pages 0-15, 4KB):  system area  --  config + index               */
/*    Pages 0-1   : Configuration (512 B)                                   */
/*    Pages 2-7   : Index journal (1536 B)                                  */
/*    Pages 8-15  : Reserved (future use, pad to sector boundary)           */
/*                                                                           */
/*  Sectors 1+ (pages 16-32767): data circular buffer                       */
/*    Each page = 1 record (256 B)                                          */
/*    Total: 32752 pages = 8,384,512 B ≈ 8 MB                              */
/* ========================================================================= */
#define STORAGE_CONFIG_START_PAGE   0
#define STORAGE_CONFIG_PAGE_COUNT   2       /* 512 B   --  configuration        */
#define STORAGE_INDEX_START_PAGE    2
#define STORAGE_INDEX_PAGE_COUNT    6       /* 1536 B  --  index journal        */
#define STORAGE_RESERVED_START_PAGE 8       /* 8 pages reserved (pad to      */
#define STORAGE_RESERVED_PAGE_COUNT 8       /*   sector boundary)            */
#define STORAGE_DATA_START_PAGE     16      /* MUST be sector-aligned (16)   */
#define STORAGE_DATA_PAGE_COUNT     (W25Q64_TOTAL_PAGES - STORAGE_DATA_START_PAGE)
                                           /* 32752 pages = 8,384,512 B     */

#define STORAGE_PAGE_SIZE           W25Q64_PAGE_SIZE  /* 256                 */
#define STORAGE_SECTOR_SIZE         4096

/* Address helpers */
#define STORAGE_PAGE_TO_ADDR(p)     ((uint32_t)(p) * STORAGE_PAGE_SIZE)
#define STORAGE_ADDR_TO_PAGE(a)     ((uint32_t)(a) / STORAGE_PAGE_SIZE)

/* ------------------------------------------------------------------------- */
/*  Configuration (stored in config pages)                                   */
/* ------------------------------------------------------------------------- */
#define STORAGE_CONFIG_MAGIC        0x57454A42  /* "W25Q"                    */

typedef struct {
	uint32_t magic;                /* STORAGE_CONFIG_MAGIC                 */
	uint32_t sample_interval_s;    /* sampling interval, seconds           */
	char     server_ip[64];        /* e.g. "115.120.239.161"              */
	char     server_port[8];       /* e.g. "27413"                        */
	char     apn[32];              /* APN string, null-terminated          */
	char     wifi_ssid[33];        /* WiFi SSID (32 + null)               */
	char     wifi_password[65];    /* WiFi password (64 + null)           */
	uint8_t  reserved[44];         /* pad to 254                           */
	uint16_t crc16;                /* CRC-16 of bytes 0..253               */
} __attribute__((packed)) storage_config_t;

_Static_assert(sizeof(storage_config_t) == 256,
	       "storage_config_t must be 256 bytes");

/* Default configuration (pulled from main/config.h) */
#define STORAGE_DEFAULT_SAMPLE_INTERVAL_S  1
#define STORAGE_DEFAULT_SERVER_IP          CFG_SERVER_IP_DEFAULT
#define STORAGE_DEFAULT_SERVER_PORT        CFG_SERVER_PORT_STR
#define STORAGE_DEFAULT_APN                ""

/* ------------------------------------------------------------------------- */
/*  Index Journal Entry (16 bytes each)                                      */
/* ------------------------------------------------------------------------- */
#define STORAGE_INDEX_MAGIC         0xBEEFBEEF
#define STORAGE_INDEX_ENTRY_SIZE    16
#define STORAGE_INDEX_MAX_ENTRIES   ((STORAGE_INDEX_PAGE_COUNT * STORAGE_PAGE_SIZE) \
				     / STORAGE_INDEX_ENTRY_SIZE)  /* 96 */

typedef struct {
	uint32_t magic;                /* STORAGE_INDEX_MAGIC                  */
	uint32_t seq;                  /* monotonic sequence number            */
	uint32_t write_page;           /* next data page to write              */
	uint32_t read_page;            /* next data page to upload             */
} __attribute__((packed)) storage_index_entry_t;

_Static_assert(sizeof(storage_index_entry_t) == 16,
	       "storage_index_entry_t must be 16 bytes");

/* ------------------------------------------------------------------------- */
/*  Data Record (1 record = 1 page = 256 bytes)                              */
/* ------------------------------------------------------------------------- */
/* Status byte for data records (stored at byte 0 of each data page).
 * NOR flash: erased = 0xFF, we only change 1→0.                          */
#define STORAGE_REC_STATUS_EMPTY    0xFF
#define STORAGE_REC_STATUS_WRITTEN  0xFE   /* not uploaded yet              */
#define STORAGE_REC_STATUS_UPLOADED 0xFC   /* uploaded, can be erased       */
#define STORAGE_REC_STATUS_FAILED   0x00   /* upload permanently failed     */

typedef struct {
	uint8_t  status;               /* 0xFF/0xFE/0xFC                       */
	uint32_t timestamp;            /* Unix time or uptime seconds          */
	uint8_t  gps_fix;              /* 0 = no fix, 1 = fix                  */
	float    lat;                  /* latitude, degrees                    */
	float    lon;                  /* longitude, degrees                   */
	float    acc_x;                /* accelerometer, g                     */
	float    acc_y;
	float    acc_z;
	float    gyro_x;               /* gyroscope, dps                       */
	float    gyro_y;
	float    gyro_z;
	float    mag_x;                /* magnetometer, µT                     */
	float    mag_y;
	float    mag_z;
	uint16_t crc16;                /* CRC-16 of bytes 1..49 (timestamp..mag_z) */
	uint8_t  reserved[204];        /* pad to 256; room for expansion       */
} __attribute__((packed)) storage_record_t;

_Static_assert(sizeof(storage_record_t) == 256,
	       "storage_record_t must be 256 bytes");

/* Record payload length (CRC covers timestamp..mag_z, excludes crc16)    */
#define STORAGE_REC_PAYLOAD_OFFSET  1
#define STORAGE_REC_PAYLOAD_LEN     49   /* bytes 1..49 (timestamp..mag_z) */

/* ------------------------------------------------------------------------- */
/*  Public API                                                                */
/* ------------------------------------------------------------------------- */

/**
 * @brief  Initialize the storage subsystem.
 * @param  flash_handle  Initialised W25Q64 handle.
 * @return ESP_OK on success.
 *
 * - On first boot (no valid config), writes default config.
 * - Recovers write/read pointers from index journal.
 * - Scans data area to validate pointers after unexpected reset.
 */
esp_err_t storage_init(w25q64_handle_t *flash_handle);

/**
 * @brief  Read the current configuration from ROM.
 * @param  cfg  [out] filled with config data.
 */
esp_err_t storage_config_read(storage_config_t *cfg);

/**
 * @brief  Write configuration to ROM (triggers sector erase).
 */
esp_err_t storage_config_write(const storage_config_t *cfg);

/**
 * @brief  Get default configuration.
 */
void storage_config_default(storage_config_t *cfg);

/* ---- Record I/O ---- */

/**
 * @brief  Append one record to the circular buffer.
 *
 * If write pointer would overwrite an unuploaded record, the oldest
 * unuploaded record is dropped (read pointer advances).
 *
 * @param  rec  [in] record to write (status byte set internally).
 * @return ESP_OK on success.
 */
esp_err_t storage_record_append(const storage_record_t *rec);

/**
 * @brief  Read the oldest unuploaded record without removing it.
 * @param  rec     [out] filled record data.
 * @param  found   [out] true if an unuploaded record exists.
 */
esp_err_t storage_record_peek(storage_record_t *rec, bool *found);

/**
 * @brief  Mark the current peeked record as uploaded.
 *
 * Updates the flash status byte and advances the read pointer.
 */
esp_err_t storage_record_mark_uploaded(void);

/**
 * @brief  Set the status byte of the last written record.
 * @param  status  new status byte (must be ≤ previous value for NOR flash)
 */
esp_err_t storage_record_set_last_status(uint8_t status);

/** @return Number of unuploaded records currently in the buffer. */
uint32_t storage_unuploaded_count(void);

/** @return Maximum number of records the data area can hold. */
uint32_t storage_capacity(void);

/** @return Current sampling interval in seconds (from cached config). */
uint32_t storage_sample_interval_s(void);

/* ========================================================================= */
/*  Image Cache Region (Flash second half: 0x400000 – 0x7FFFFF)             */
/* ========================================================================= */
#define IMG_CACHE_BASE_ADDR      0x400000
#define IMG_CACHE_SIZE            0x400000   /* 4 MB */

/**
 * @brief  Erase the entire image cache region (sector-by-sector).
 */
esp_err_t img_cache_erase_all(void);

/**
 * @brief  Write data to the image cache region.
 * @param  offset  byte offset within IMG_CACHE region (0 … IMG_CACHE_SIZE-1)
 * @param  data    source buffer
 * @param  len     number of bytes to write (must not cross 256B page boundary)
 */
esp_err_t img_cache_write(uint32_t offset, const uint8_t *data, size_t len);

/**
 * @brief  Read data from the image cache region.
 * @param  offset  byte offset within IMG_CACHE region
 * @param  data    destination buffer
 * @param  len     number of bytes to read
 */
esp_err_t img_cache_read(uint32_t offset, uint8_t *data, size_t len);

/**
 * @brief  When enabled, storage_record_append() writes to RAM cache instead
 *         of Flash.  Used during IMG_RECEIVE to avoid SPI bus contention.
 *         Call ram_cache_flush() later to persist records to Flash.
 */
void storage_set_ram_mode(bool enable);

/* ========================================================================= */
/*  RAM Cache (used during IMG_RECEIVE to buffer samples)                   */
/* ========================================================================= */
#define RAM_CACHE_CAPACITY  512   /* 512 records × 256 B = 128 KB */

/**
 * @brief  Initialise or reset the RAM circular buffer.
 */
void ram_cache_init(void);

/**
 * @brief  Append a sensor record to the RAM cache.
 * @return ESP_OK, or ESP_ERR_NO_MEM if full (oldest dropped).
 */
esp_err_t ram_cache_append(const storage_record_t *rec);

/**
 * @brief  Return the number of records currently in the RAM cache.
 */
uint32_t ram_cache_count(void);

/**
 * @brief  Flush all RAM-cached records to Flash (sensor data region).
 *         After flush the RAM cache is emptied.
 */
esp_err_t ram_cache_flush(void);

#ifdef __cplusplus
}
#endif
