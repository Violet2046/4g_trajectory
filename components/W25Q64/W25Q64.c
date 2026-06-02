#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "W25Q64.h"

#define W25Q64_TAG "W25Q64"

/* Default busy-wait timeout after erase/program (ms) */
#define W25Q64_DEF_TIMEOUT_MS  5000
/* Chip Erase max time is much longer */
#define W25Q64_CHIP_ERASE_TIMEOUT_MS  120000

struct w25q64_handle_t {
	w25q64_config_t        config;
	spi_device_handle_t    dev_handle;
	SemaphoreHandle_t      lock;
};

/* ------------------------------------------------------------------------- */
/*  Low-level SPI helper                                                       */
/* ------------------------------------------------------------------------- */
static esp_err_t w25q64_spi_tx(w25q64_handle_t *handle,
			       const uint8_t *tx, size_t tx_len)
{
	spi_transaction_t trans = {
		.length    = tx_len * 8,
		.tx_buffer = tx,
		.rx_buffer = NULL,
	};
	return spi_device_transmit(handle->dev_handle, &trans);
}

static esp_err_t w25q64_spi_txrx(w25q64_handle_t *handle,
				 const uint8_t *tx, size_t tx_len,
				 uint8_t *rx, size_t rx_len)
{
	/* Full-duplex: total transaction = tx_len (cmd/addr) + rx_len (data).
	 * The RX buffer captures everything; real data starts at offset tx_len. */
	size_t total = tx_len + rx_len;
	uint8_t *tx_buf = (uint8_t *)malloc(total);
	uint8_t *rx_buf = (uint8_t *)malloc(total);
	if ((tx_buf == NULL) || (rx_buf == NULL)) {
		free(tx_buf);
		free(rx_buf);
		return ESP_ERR_NO_MEM;
	}

	memcpy(tx_buf, tx, tx_len);
	memset(tx_buf + tx_len, 0xFF, rx_len);  /* dummy / don't-care bytes */

	spi_transaction_t trans = {
		.length    = total * 8,
		.tx_buffer = tx_buf,
		.rx_buffer = rx_buf,
	};

	esp_err_t err = spi_device_transmit(handle->dev_handle, &trans);
	if (err == ESP_OK && rx != NULL) {
		memcpy(rx, rx_buf + tx_len, rx_len);
	}

	free(tx_buf);
	free(rx_buf);
	return err;
}

/* Convenience: send 1 command byte, receive 1..4 bytes */
static esp_err_t w25q64_cmd_rx(w25q64_handle_t *handle,
			       uint8_t cmd, uint8_t *rx, size_t rx_len)
{
	uint8_t tx = cmd;
	return w25q64_spi_txrx(handle, &tx, 1, rx, rx_len);
}

/* Convenience: send command + 3-byte address */
static esp_err_t w25q64_cmd_addr(w25q64_handle_t *handle,
				 uint8_t cmd, uint32_t addr)
{
	uint8_t tx[4] = {
		cmd,
		(uint8_t)(addr >> 16),
		(uint8_t)(addr >> 8),
		(uint8_t)(addr),
	};
	return w25q64_spi_tx(handle, tx, sizeof(tx));
}

/* Convenience: send command + 3-byte address, then read rx_len bytes */
static esp_err_t w25q64_cmd_addr_rx(w25q64_handle_t *handle,
				    uint8_t cmd, uint32_t addr,
				    uint8_t *rx, size_t rx_len)
{
	uint8_t tx[4] = {
		cmd,
		(uint8_t)(addr >> 16),
		(uint8_t)(addr >> 8),
		(uint8_t)(addr),
	};
	return w25q64_spi_txrx(handle, tx, sizeof(tx), rx, rx_len);
}

/* ------------------------------------------------------------------------- */
/*  Create / Destroy                                                          */
/* ------------------------------------------------------------------------- */
esp_err_t w25q64_create(w25q64_handle_t **out_handle,
			const w25q64_config_t *config)
{
	if ((out_handle == NULL) || (config == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	w25q64_handle_t *handle = (w25q64_handle_t *)calloc(1, sizeof(*handle));
	if (handle == NULL) {
		return ESP_ERR_NO_MEM;
	}

	handle->config = *config;
	handle->lock = xSemaphoreCreateMutex();
	if (handle->lock == NULL) {
		free(handle);
		return ESP_ERR_NO_MEM;
	}

	/* ---- Configure optional WP and HOLD GPIOs ---- */
	if (config->wp_gpio >= 0) {
		gpio_config_t wp_cfg = {
			.pin_bit_mask = (1ULL << config->wp_gpio),
			.mode         = GPIO_MODE_OUTPUT,
			.pull_up_en   = GPIO_PULLUP_ENABLE,
			.pull_down_en = GPIO_PULLDOWN_DISABLE,
			.intr_type    = GPIO_INTR_DISABLE,
		};
		gpio_config(&wp_cfg);
		gpio_set_level(config->wp_gpio, 1);  /* WP inactive high */
	}
	if (config->hold_gpio >= 0) {
		gpio_config_t hold_cfg = {
			.pin_bit_mask = (1ULL << config->hold_gpio),
			.mode         = GPIO_MODE_OUTPUT,
			.pull_up_en   = GPIO_PULLUP_ENABLE,
			.pull_down_en = GPIO_PULLDOWN_DISABLE,
			.intr_type    = GPIO_INTR_DISABLE,
		};
		gpio_config(&hold_cfg);
		gpio_set_level(config->hold_gpio, 1);  /* HOLD inactive high */
	}

	/* ---- Pre-init CS + release JTAG pins (GPIO 2,3,4 are JTAG on ESP32-C3) ---- */
	gpio_reset_pin(config->sck_gpio);
	gpio_reset_pin(config->mosi_gpio);
	gpio_reset_pin(config->miso_gpio);
	gpio_reset_pin(config->cs_gpio);

	{
		gpio_config_t cs_cfg = {
			.pin_bit_mask = (1ULL << config->cs_gpio),
			.mode         = GPIO_MODE_OUTPUT,
			.pull_up_en   = GPIO_PULLUP_ENABLE,
			.pull_down_en = GPIO_PULLDOWN_DISABLE,
			.intr_type    = GPIO_INTR_DISABLE,
		};
		gpio_config(&cs_cfg);
		gpio_set_level(config->cs_gpio, 1);
	}

	/* ---- Initialize SPI bus ---- */
	spi_bus_config_t bus_cfg = {
		.sclk_io_num     = config->sck_gpio,
		.mosi_io_num     = config->mosi_gpio,
		.miso_io_num     = config->miso_gpio,
		.quadwp_io_num   = -1,
		.quadhd_io_num   = -1,
		.max_transfer_sz = 0,  /* default to ~4092 bytes to allow large reads */
		.flags           = SPICOMMON_BUSFLAG_MASTER,
		.intr_flags      = 0,
	};

	esp_err_t err = spi_bus_initialize(config->host, &bus_cfg, config->dma_chan);
	if (err != ESP_OK) {
		ESP_LOGE(W25Q64_TAG, "spi_bus_initialize failed: %s", esp_err_to_name(err));
		vSemaphoreDelete(handle->lock);
		free(handle);
		return err;
	}

	/* ---- Add device ---- */
	spi_device_interface_config_t dev_cfg = {
		.mode             = 3,  /* SPI Mode 3 (CPOL=1, CPHA=1) — stable on ESP32-C3 */
		.clock_source     = SPI_CLK_SRC_DEFAULT,
		.clock_speed_hz   = config->freq_hz,
		.duty_cycle_pos   = 128,
		.cs_ena_pretrans  = 2,  /* CS setup time */
		.cs_ena_posttrans = 2,  /* CS hold time */
		.spics_io_num     = config->cs_gpio,
		.command_bits     = 0,
		.address_bits     = 0,
		.dummy_bits       = 0,
		.queue_size       = 4,
		.flags            = 0,  /* full-duplex; half-duplex flag conflicts with tx+rx transactions */
	};

	err = spi_bus_add_device(config->host, &dev_cfg, &handle->dev_handle);
	if (err != ESP_OK) {
		ESP_LOGE(W25Q64_TAG, "spi_bus_add_device failed: %s", esp_err_to_name(err));
		spi_bus_free(config->host);
		vSemaphoreDelete(handle->lock);
		free(handle);
		return err;
	}

	*out_handle = handle;
	return ESP_OK;
}

void w25q64_destroy(w25q64_handle_t *handle)
{
	if (handle == NULL) {
		return;
	}
	if (handle->dev_handle != NULL) {
		spi_bus_remove_device(handle->dev_handle);
	}
	spi_bus_free(handle->config.host);
	if (handle->lock != NULL) {
		vSemaphoreDelete(handle->lock);
	}
	free(handle);
}

/* ------------------------------------------------------------------------- */
/*  Init                                                                       */
/* ------------------------------------------------------------------------- */
esp_err_t w25q64_init(w25q64_handle_t **out_handle,
		      const w25q64_config_t *config)
{
	w25q64_handle_t *handle;
	esp_err_t err;
	uint8_t mfr;
	uint16_t dev;

	err = w25q64_create(&handle, config);
	if (err != ESP_OK) {
		return err;
	}

	/* Software reset (66h + 99h) */
	w25q64_reset(handle);
	vTaskDelay(pdMS_TO_TICKS(1));

	/* Verify JEDEC ID */
	err = w25q64_read_jedec_id(handle, &mfr, &dev);
	if (err != ESP_OK) {
		ESP_LOGE(W25Q64_TAG, "JEDEC ID read failed");
		goto fail;
	}
	if (mfr != W25Q64_MFR_ID && mfr != W25Q64_MFR_ID_MICRON) {
		ESP_LOGI(W25Q64_TAG, "JEDEC ID: %02X %02X %02X (Winbond=%02X, Micron=%02X)",
			 mfr, (dev >> 8) & 0xFF, dev & 0xFF,
			 W25Q64_MFR_ID, W25Q64_MFR_ID_MICRON);
		err = ESP_ERR_NOT_FOUND;
		goto fail;
	}
	if (dev != W25Q64_DEVICE_ID) {
		ESP_LOGW(W25Q64_TAG, "Unexpected device ID: 0x%04X (expected 0x%04X)",
			 dev, W25Q64_DEVICE_ID);
		/* Continue — some variants may have different ID */
	}

	/* Ensure device is not in power-down */
	w25q64_release_power_down(handle);

	*out_handle = handle;
	ESP_LOGI(W25Q64_TAG, "init ok — JEDEC ID EFh %04Xh, freq %lu Hz",
		 dev, config->freq_hz);
	return ESP_OK;

fail:
	w25q64_destroy(handle);
	return err;
}

/* ------------------------------------------------------------------------- */
/*  Write Enable / Disable                                                     */
/* ------------------------------------------------------------------------- */
esp_err_t w25q64_write_enable(w25q64_handle_t *handle)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	uint8_t cmd = W25Q64_CMD_WRITE_ENABLE;
	return w25q64_spi_tx(handle, &cmd, 1);
}

esp_err_t w25q64_write_disable(w25q64_handle_t *handle)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	uint8_t cmd = W25Q64_CMD_WRITE_DISABLE;
	return w25q64_spi_tx(handle, &cmd, 1);
}

/* ------------------------------------------------------------------------- */
/*  Busy Wait                                                                  */
/* ------------------------------------------------------------------------- */
esp_err_t w25q64_wait_busy(w25q64_handle_t *handle, uint32_t timeout_ms)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	uint8_t sr1;
	esp_err_t err;
	uint32_t elapsed = 0;

	while (elapsed < timeout_ms) {
		err = w25q64_cmd_rx(handle, W25Q64_CMD_READ_STATUS1, &sr1, 1);
		if (err == ESP_OK && !(sr1 & W25Q64_SR1_BUSY)) return ESP_OK;
		/* If SPI read fails (flash busy), just wait and retry */
		vTaskDelay(pdMS_TO_TICKS(10));
		elapsed += 10;
	}

	return ESP_ERR_TIMEOUT;
}

/* ------------------------------------------------------------------------- */
/*  Status Register                                                            */
/* ------------------------------------------------------------------------- */
esp_err_t w25q64_status_read(w25q64_handle_t *handle,
			     uint8_t *sr1, uint8_t *sr2, uint8_t *sr3)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t err;

	if (sr1 != NULL) {
		err = w25q64_cmd_rx(handle, W25Q64_CMD_READ_STATUS1, sr1, 1);
		if (err != ESP_OK) return err;
	}
	if (sr2 != NULL) {
		err = w25q64_cmd_rx(handle, W25Q64_CMD_READ_STATUS2, sr2, 1);
		if (err != ESP_OK) return err;
	}
	if (sr3 != NULL) {
		err = w25q64_cmd_rx(handle, W25Q64_CMD_READ_STATUS3, sr3, 1);
		if (err != ESP_OK) return err;
	}

	return ESP_OK;
}

esp_err_t w25q64_status_write(w25q64_handle_t *handle,
			      uint8_t sr1, uint8_t sr2, uint8_t sr3,
			      bool write_sr2, bool write_sr3)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t err;
	uint8_t buf[2];

	/* Write Status Register-1 */
	err = w25q64_write_enable(handle);
	if (err != ESP_OK) return err;
	buf[0] = W25Q64_CMD_WRITE_STATUS1;
	buf[1] = sr1;
	err = w25q64_spi_tx(handle, buf, sizeof(buf));
	if (err != ESP_OK) return err;
	err = w25q64_wait_busy(handle, W25Q64_DEF_TIMEOUT_MS);
	if (err != ESP_OK) return err;

	/* Write Status Register-2 */
	if (write_sr2) {
		err = w25q64_write_enable(handle);
		if (err != ESP_OK) return err;
		buf[0] = W25Q64_CMD_WRITE_STATUS2;
		buf[1] = sr2;
		err = w25q64_spi_tx(handle, buf, sizeof(buf));
		if (err != ESP_OK) return err;
		err = w25q64_wait_busy(handle, W25Q64_DEF_TIMEOUT_MS);
		if (err != ESP_OK) return err;
	}

	/* Write Status Register-3 */
	if (write_sr3) {
		err = w25q64_write_enable(handle);
		if (err != ESP_OK) return err;
		buf[0] = W25Q64_CMD_WRITE_STATUS3;
		buf[1] = sr3;
		err = w25q64_spi_tx(handle, buf, sizeof(buf));
		if (err != ESP_OK) return err;
		err = w25q64_wait_busy(handle, W25Q64_DEF_TIMEOUT_MS);
		if (err != ESP_OK) return err;
	}

	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Read Data                                                                  */
/* ------------------------------------------------------------------------- */
esp_err_t w25q64_read(w25q64_handle_t *handle,
		      uint32_t addr, uint8_t *data, size_t len)
{
	if ((handle == NULL) || (data == NULL) || (len == 0)) {
		return ESP_ERR_INVALID_ARG;
	}
	if (addr + len > W25Q64_CAPACITY_BYTES) {
		return ESP_ERR_INVALID_ARG;
	}

	return w25q64_cmd_addr_rx(handle, W25Q64_CMD_READ_DATA, addr, data, len);
}

/* ------------------------------------------------------------------------- */
/*  Page Program                                                               */
/* ------------------------------------------------------------------------- */
esp_err_t w25q64_page_program(w25q64_handle_t *handle,
			      uint32_t addr, const uint8_t *data, size_t len)
{
	if ((handle == NULL) || (data == NULL) || (len == 0)) {
		return ESP_ERR_INVALID_ARG;
	}

	/* Clamp to one page boundary */
	uint32_t page_start = addr & ~((uint32_t)W25Q64_PAGE_SIZE - 1);
	uint32_t page_end   = page_start + W25Q64_PAGE_SIZE;
	if (addr + len > page_end) {
		len = page_end - addr;
		ESP_LOGW(W25Q64_TAG, "page_program clamped to page boundary, "
			 "new len=%zu", len);
	}
	if (addr + len > W25Q64_CAPACITY_BYTES) {
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t err;

	/* 1. Write Enable */
	err = w25q64_write_enable(handle);
	if (err != ESP_OK) return err;

	/* 2. Send Page Program command + address + data in one transaction */
	size_t total = 4 + len;  /* command + 3 addr bytes + data */
	uint8_t *tx_buf = (uint8_t *)malloc(total);
	if (tx_buf == NULL) return ESP_ERR_NO_MEM;

	tx_buf[0] = W25Q64_CMD_PAGE_PROGRAM;
	tx_buf[1] = (uint8_t)(addr >> 16);
	tx_buf[2] = (uint8_t)(addr >> 8);
	tx_buf[3] = (uint8_t)(addr);
	memcpy(tx_buf + 4, data, len);

	err = w25q64_spi_tx(handle, tx_buf, total);
	free(tx_buf);
	if (err != ESP_OK) return err;

	/* 3. Wait for completion */
	return w25q64_wait_busy(handle, W25Q64_DEF_TIMEOUT_MS);
}

/* ------------------------------------------------------------------------- */
/*  Erase Operations                                                           */
/* ------------------------------------------------------------------------- */
static esp_err_t w25q64_erase(w25q64_handle_t *handle,
			      uint8_t cmd, uint32_t addr,
			      uint32_t timeout_ms)
{
	esp_err_t err;

	/* 1. Write Enable */
	err = w25q64_write_enable(handle);
	if (err != ESP_OK) return err;

	/* 2. Send erase command + address */
	err = w25q64_cmd_addr(handle, cmd, addr);
	if (err != ESP_OK) return err;

	/* 3. Wait for completion */
	return w25q64_wait_busy(handle, timeout_ms);
}

esp_err_t w25q64_sector_erase(w25q64_handle_t *handle, uint32_t addr)
{
	if (handle == NULL) return ESP_ERR_INVALID_ARG;
	if (addr >= W25Q64_CAPACITY_BYTES) return ESP_ERR_INVALID_ARG;
	return w25q64_erase(handle, W25Q64_CMD_SECTOR_ERASE, addr,
			    W25Q64_DEF_TIMEOUT_MS);
}

esp_err_t w25q64_block_erase_32k(w25q64_handle_t *handle, uint32_t addr)
{
	if (handle == NULL) return ESP_ERR_INVALID_ARG;
	if (addr >= W25Q64_CAPACITY_BYTES) return ESP_ERR_INVALID_ARG;
	return w25q64_erase(handle, W25Q64_CMD_BLOCK_ERASE_32K, addr,
			    W25Q64_DEF_TIMEOUT_MS);
}

esp_err_t w25q64_block_erase_64k(w25q64_handle_t *handle, uint32_t addr)
{
	if (handle == NULL) return ESP_ERR_INVALID_ARG;
	if (addr >= W25Q64_CAPACITY_BYTES) return ESP_ERR_INVALID_ARG;
	return w25q64_erase(handle, W25Q64_CMD_BLOCK_ERASE_64K, addr,
			    W25Q64_DEF_TIMEOUT_MS);
}

esp_err_t w25q64_chip_erase(w25q64_handle_t *handle)
{
	if (handle == NULL) return ESP_ERR_INVALID_ARG;

	esp_err_t err;

	err = w25q64_write_enable(handle);
	if (err != ESP_OK) return err;

	uint8_t cmd = W25Q64_CMD_CHIP_ERASE;
	err = w25q64_spi_tx(handle, &cmd, 1);
	if (err != ESP_OK) return err;

	return w25q64_wait_busy(handle, W25Q64_CHIP_ERASE_TIMEOUT_MS);
}

/* ------------------------------------------------------------------------- */
/*  Power / Reset                                                              */
/* ------------------------------------------------------------------------- */
esp_err_t w25q64_power_down(w25q64_handle_t *handle)
{
	if (handle == NULL) return ESP_ERR_INVALID_ARG;
	uint8_t cmd = W25Q64_CMD_POWER_DOWN;
	return w25q64_spi_tx(handle, &cmd, 1);
}

esp_err_t w25q64_release_power_down(w25q64_handle_t *handle)
{
	if (handle == NULL) return ESP_ERR_INVALID_ARG;
	uint8_t cmd = W25Q64_CMD_RELEASE_PD;
	return w25q64_cmd_rx(handle, cmd, NULL, 0);
}

esp_err_t w25q64_reset(w25q64_handle_t *handle)
{
	if (handle == NULL) return ESP_ERR_INVALID_ARG;

	esp_err_t err;
	uint8_t cmd;

	/* Enable Reset (66h) */
	cmd = W25Q64_CMD_ENABLE_RESET;
	err = w25q64_spi_tx(handle, &cmd, 1);
	if (err != ESP_OK) return err;

	/* Reset Device (99h) */
	cmd = W25Q64_CMD_RESET_DEVICE;
	err = w25q64_spi_tx(handle, &cmd, 1);
	if (err != ESP_OK) return err;

	/* tRST ≈ 30 µs */
	vTaskDelay(pdMS_TO_TICKS(1));

	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  JEDEC ID / Unique ID                                                       */
/* ------------------------------------------------------------------------- */
esp_err_t w25q64_read_jedec_id(w25q64_handle_t *handle,
			       uint8_t *mfr_id, uint16_t *dev_id)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	uint8_t rx[W25Q64_JEDEC_ID_BYTES];
	esp_err_t err = w25q64_cmd_rx(handle, W25Q64_CMD_READ_JEDEC_ID,
				      rx, sizeof(rx));
	if (err != ESP_OK) return err;

	if (mfr_id != NULL) {
		*mfr_id = rx[0];
	}
	if (dev_id != NULL) {
		*dev_id = ((uint16_t)rx[1] << 8) | rx[2];
	}

	return ESP_OK;
}

esp_err_t w25q64_read_unique_id(w25q64_handle_t *handle, uint8_t *uid)
{
	if ((handle == NULL) || (uid == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	/* Read Unique ID: 4Bh + 4 dummy bytes (addr=00000000h) + 8 bytes UID */
	uint8_t tx[5] = {
		W25Q64_CMD_READ_UNIQUE_ID,
		0x00, 0x00, 0x00, 0x00,  /* 4 dummy address bytes */
	};

	return w25q64_spi_txrx(handle, tx, sizeof(tx), uid, 8);
}
