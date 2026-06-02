#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/spi_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*  W25Q64JV 64M-bit (8M-byte) Serial NOR Flash                              */
/*  Datasheet: Revision J, March 2018                                        */
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/*  Memory Geometry                                                          */
/* ------------------------------------------------------------------------- */
#define W25Q64_CAPACITY_BYTES      (8UL * 1024 * 1024)  /*  8 MB               */
#define W25Q64_PAGE_SIZE           256                    /* bytes per page      */
#define W25Q64_SECTOR_SIZE         4096                   /* 4 KB sector erase   */
#define W25Q64_BLOCK_32K_SIZE      32768                  /* 32 KB block erase   */
#define W25Q64_BLOCK_64K_SIZE      65536                  /* 64 KB block erase   */
#define W25Q64_TOTAL_PAGES         32768
#define W25Q64_TOTAL_SECTORS       2048
#define W25Q64_TOTAL_BLOCKS        128

/* ------------------------------------------------------------------------- */
/*  Instruction Set (Standard SPI)                                           */
/* ------------------------------------------------------------------------- */
#define W25Q64_CMD_WRITE_ENABLE        0x06
#define W25Q64_CMD_VOLATILE_SR_WREN    0x50
#define W25Q64_CMD_WRITE_DISABLE       0x04
#define W25Q64_CMD_READ_STATUS1       0x05
#define W25Q64_CMD_WRITE_STATUS1      0x01
#define W25Q64_CMD_READ_STATUS2       0x35
#define W25Q64_CMD_WRITE_STATUS2      0x31
#define W25Q64_CMD_READ_STATUS3       0x15
#define W25Q64_CMD_WRITE_STATUS3      0x11
#define W25Q64_CMD_READ_DATA           0x03
#define W25Q64_CMD_FAST_READ           0x0B
#define W25Q64_CMD_PAGE_PROGRAM        0x02
#define W25Q64_CMD_SECTOR_ERASE        0x20  /*  4 KB                   */
#define W25Q64_CMD_BLOCK_ERASE_32K     0x52
#define W25Q64_CMD_BLOCK_ERASE_64K     0xD8
#define W25Q64_CMD_CHIP_ERASE          0xC7  /*  0x60 also valid        */
#define W25Q64_CMD_READ_JEDEC_ID       0x9F
#define W25Q64_CMD_READ_UNIQUE_ID      0x4B
#define W25Q64_CMD_READ_SFDP           0x5A
#define W25Q64_CMD_POWER_DOWN          0xB9
#define W25Q64_CMD_RELEASE_PD          0xAB
#define W25Q64_CMD_ENABLE_RESET        0x66
#define W25Q64_CMD_RESET_DEVICE        0x99
#define W25Q64_CMD_SUSPEND             0x75
#define W25Q64_CMD_RESUME              0x7A

/* ------------------------------------------------------------------------- */
/*  JEDEC ID                                                                  */
/* ------------------------------------------------------------------------- */
#define W25Q64_MFR_ID                 0xEF  /*  Winbond                         */
#define W25Q64_MFR_ID_MICRON           0x20  /*  Micron / ST                     */
#define W25Q64_DEVICE_ID              0x4017 /* IQ/JQ variant (QE=1 default)     */
#define W25Q64_JEDEC_ID_BYTES         3

/* ------------------------------------------------------------------------- */
/*  Status Register-1 bits (R: 05h / W: 01h)                                 */
/* ------------------------------------------------------------------------- */
#define W25Q64_SR1_BUSY               (1U << 0)  /* Erase/Write in progress     */
#define W25Q64_SR1_WEL                (1U << 1)  /* Write Enable Latch          */
#define W25Q64_SR1_BP0                (1U << 2)  /* Block Protect 0             */
#define W25Q64_SR1_BP1                (1U << 3)  /* Block Protect 1             */
#define W25Q64_SR1_BP2                (1U << 4)  /* Block Protect 2             */
#define W25Q64_SR1_TB                 (1U << 5)  /* Top/Bottom Protect          */
#define W25Q64_SR1_SEC                (1U << 6)  /* Sector/Block Protect        */
#define W25Q64_SR1_SRP                (1U << 7)  /* Status Register Protect     */

/* ------------------------------------------------------------------------- */
/*  Status Register-2 bits (R: 35h / W: 31h)                                 */
/* ------------------------------------------------------------------------- */
#define W25Q64_SR2_SRL                (1U << 0)  /* Status Register Lock       */
#define W25Q64_SR2_QE                 (1U << 1)  /* Quad Enable                */
#define W25Q64_SR2_LB1                (1U << 3)  /* Security Register Lock 1   */
#define W25Q64_SR2_LB2                (1U << 4)  /* Security Register Lock 2   */
#define W25Q64_SR2_LB3                (1U << 5)  /* Security Register Lock 3   */
#define W25Q64_SR2_CMP                (1U << 6)  /* Complement Protect         */
#define W25Q64_SR2_SUS                (1U << 7)  /* Suspend Status (read-only) */

/* ------------------------------------------------------------------------- */
/*  Status Register-3 bits (R: 15h / W: 11h)                                 */
/* ------------------------------------------------------------------------- */
#define W25Q64_SR3_DRV0               (1U << 1)  /* Output Driver Strength     */
#define W25Q64_SR3_DRV1               (1U << 2)  /* Output Driver Strength     */
#define W25Q64_SR3_WPS                (1U << 3)  /* Write Protect Selection    */

/* ------------------------------------------------------------------------- */
/*  Data structures                                                           */
/* ------------------------------------------------------------------------- */
typedef struct w25q64_handle_t w25q64_handle_t;

/** SPI configuration passed to w25q64_create() */
typedef struct {
	spi_host_device_t  host;        /* SPI peripheral, e.g. SPI2_HOST     */
	int                cs_gpio;     /* Chip Select GPIO number            */
	int                sck_gpio;    /* Serial Clock GPIO                  */
	int                mosi_gpio;   /* MOSI (IO0/DI) GPIO                 */
	int                miso_gpio;   /* MISO (IO1/DO) GPIO                 */
	int                wp_gpio;     /* Write Protect GPIO, -1 if unused   */
	int                hold_gpio;   /* Hold/Reset GPIO, -1 if unused      */
	int                dma_chan;    /* DMA channel: SPI_DMA_CH_AUTO for auto-alloc */
	uint32_t           freq_hz;     /* SPI clock frequency, Hz            */
} w25q64_config_t;

/* ------------------------------------------------------------------------- */
/*  Create / Destroy / Init                                                   */
/* ------------------------------------------------------------------------- */
esp_err_t w25q64_create(w25q64_handle_t **out_handle,
			const w25q64_config_t *config);
void      w25q64_destroy(w25q64_handle_t *handle);

/**
 * @brief  Create handle, verify JEDEC ID, and reset to known state.
 *
 * - Issues software reset (66h + 99h)
 * - Verifies Manufacturer ID (EFh) and Device ID (4017h)
 * - Leaves device in SPI mode (QE=0 by default) ready for operation
 */
esp_err_t w25q64_init(w25q64_handle_t **out_handle,
		      const w25q64_config_t *config);

/* ------------------------------------------------------------------------- */
/*  Basic SPI Flash operations                                                */
/* ------------------------------------------------------------------------- */
/** Read @p len bytes from @p addr into @p data.  Uses Read Data (03h). */
esp_err_t w25q64_read(w25q64_handle_t *handle,
		      uint32_t addr, uint8_t *data, size_t len);

/**
 * @brief  Program up to 256 bytes within a single page.
 *
 * A page boundary is always at (addr & ~(PAGE_SIZE-1)).  This function does
 * NOT cross a page boundary; if @p addr + @p len exceeds the page, the
 * exceeding portion will be truncated (wrapped within the same page).
 *
 * The caller is responsible for erasing the sector before programming.
 */
esp_err_t w25q64_page_program(w25q64_handle_t *handle,
			      uint32_t addr, const uint8_t *data, size_t len);

/** Erase one 4 KB sector containing @p addr. Blocks until done. */
esp_err_t w25q64_sector_erase(w25q64_handle_t *handle, uint32_t addr);

/** Erase one 32 KB block containing @p addr. Blocks until done. */
esp_err_t w25q64_block_erase_32k(w25q64_handle_t *handle, uint32_t addr);

/** Erase one 64 KB block containing @p addr. Blocks until done. */
esp_err_t w25q64_block_erase_64k(w25q64_handle_t *handle, uint32_t addr);

/** Erase the entire chip.  May take tens of seconds. */
esp_err_t w25q64_chip_erase(w25q64_handle_t *handle);

/* ------------------------------------------------------------------------- */
/*  Status / Control                                                          */
/* ------------------------------------------------------------------------- */
/** Read Status Register-1, -2, or -3. */
esp_err_t w25q64_status_read(w25q64_handle_t *handle,
			     uint8_t *sr1, uint8_t *sr2, uint8_t *sr3);

/** Write Status Register-1 and optionally -2, -3.
 *  Automatically sets Write Enable (06h) before each register write. */
esp_err_t w25q64_status_write(w25q64_handle_t *handle,
			      uint8_t sr1, uint8_t sr2, uint8_t sr3,
			      bool write_sr2, bool write_sr3);

/** Set Write Enable Latch (06h). Must be called before program/erase. */
esp_err_t w25q64_write_enable(w25q64_handle_t *handle);

/** Clear Write Enable Latch (04h). */
esp_err_t w25q64_write_disable(w25q64_handle_t *handle);

/**
 * @brief  Wait for BUSY bit to clear (operation complete).
 * @param  timeout_ms  maximum wait time in milliseconds.
 */
esp_err_t w25q64_wait_busy(w25q64_handle_t *handle, uint32_t timeout_ms);

/* ------------------------------------------------------------------------- */
/*  Power / Reset                                                             */
/* ------------------------------------------------------------------------- */
esp_err_t w25q64_power_down(w25q64_handle_t *handle);
esp_err_t w25q64_release_power_down(w25q64_handle_t *handle);
esp_err_t w25q64_reset(w25q64_handle_t *handle);

/* ------------------------------------------------------------------------- */
/*  ID / Info                                                                 */
/* ------------------------------------------------------------------------- */
/**
 * @brief  Read JEDEC Manufacturer + Device ID.
 * @param  mfr_id   [out] Manufacturer ID (should be 0xEF).
 * @param  dev_id   [out] Device ID (should be 0x4017 for IQ/JQ).
 */
esp_err_t w25q64_read_jedec_id(w25q64_handle_t *handle,
			       uint8_t *mfr_id, uint16_t *dev_id);

/** Read 64-bit Unique ID.  @p uid must point to 8 bytes. */
esp_err_t w25q64_read_unique_id(w25q64_handle_t *handle, uint8_t *uid);

#ifdef __cplusplus
}
#endif
