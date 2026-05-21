#include "ST7789.h"

#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_log.h"
#include "esp_timer.h"

#define ST7789_TAG "ST7789"

#define ST7789_SPI_TIMEOUT_MS   100

/* ------------------------------------------------------------------------- */
/*  Handle structure                                                         */
/* ------------------------------------------------------------------------- */
struct st7789_handle_t {
	st7789_config_t       config;
	spi_device_handle_t   spi_dev;
};

/* ------------------------------------------------------------------------- */
/*  Low-level SPI helpers                                                     */
/* ------------------------------------------------------------------------- */

static esp_err_t st7789_write_cmd(st7789_handle_t *handle, uint8_t cmd)
{
	spi_transaction_t t = {
		.length    = 8,
		.tx_buffer = &cmd,
		.user      = (void *)0,  /* DC=0 → command */
	};
	/* Assert DC low via the DC GPIO */
	gpio_set_level(handle->config.dc_gpio, 0);
	return spi_device_transmit(handle->spi_dev, &t);
}

static esp_err_t st7789_write_data(st7789_handle_t *handle,
				   const uint8_t *data, size_t len)
{
	spi_transaction_t t = {
		.length    = len * 8,
		.tx_buffer = data,
		.user      = (void *)1,  /* DC=1 → data */
	};
	gpio_set_level(handle->config.dc_gpio, 1);
	return spi_device_transmit(handle->spi_dev, &t);
}

static esp_err_t st7789_write_cmd_data(st7789_handle_t *handle,
				       uint8_t cmd,
				       const uint8_t *data, size_t len)
{
	esp_err_t err = st7789_write_cmd(handle, cmd);
	if (err != ESP_OK) return err;
	if (len > 0) {
		err = st7789_write_data(handle, data, len);
	}
	return err;
}

/* ------------------------------------------------------------------------- */
/*  Hardware reset                                                            */
/* ------------------------------------------------------------------------- */
static void st7789_hardware_reset(st7789_handle_t *handle)
{
	if (handle->config.rst_gpio < 0) return;

	gpio_set_level(handle->config.rst_gpio, 0);
	vTaskDelay(pdMS_TO_TICKS(10));
	gpio_set_level(handle->config.rst_gpio, 1);
	vTaskDelay(pdMS_TO_TICKS(120));
}

/* ------------------------------------------------------------------------- */
/*  Init sequence                                                             */
/* ------------------------------------------------------------------------- */
static esp_err_t st7789_init_sequence(st7789_handle_t *handle)
{
	/* Software reset */
	st7789_write_cmd(handle, ST7789_CMD_SWRESET);
	vTaskDelay(pdMS_TO_TICKS(150));

	/* Sleep Out */
	st7789_write_cmd(handle, ST7789_CMD_SLPOUT);
	vTaskDelay(pdMS_TO_TICKS(10));

	/* Colour mode: RGB565 */
	{
		uint8_t arg = ST7789_COLMOD_65K;
		st7789_write_cmd_data(handle, ST7789_CMD_COLMOD, &arg, 1);
	}

	/* Memory data access control: RGB order, orientation */
	{
		uint8_t arg = ST7789_MADCTL_MX | ST7789_MADCTL_MY |
			      ST7789_MADCTL_BGR;
		st7789_write_cmd_data(handle, ST7789_CMD_MADCTL, &arg, 1);
	}

	/* Set RGB interface control (if needed) */
	{
		uint8_t args[] = {0x00, 0x00};
		st7789_write_cmd_data(handle, ST7789_CMD_RAMCTRL, args, 2);
	}

	/* PORCTRL (B2h) — porch setting */
	{
		uint8_t args[] = {0x0C, 0x0C, 0x00, 0x33, 0x33};
		st7789_write_cmd_data(handle, ST7789_CMD_PORCTRL, args, 5);
	}

	/* Gate control (B7h) */
	{
		uint8_t arg = 0x35;
		st7789_write_cmd_data(handle, ST7789_CMD_GCTRL, &arg, 1);
	}

	/* VCOM setting (BBh) */
	{
		uint8_t arg = 0x28;
		st7789_write_cmd_data(handle, 0xBB, &arg, 1);
	}

	/* LCM control (C0h) */
	{
		uint8_t arg = 0x2C;
		st7789_write_cmd_data(handle, ST7789_CMD_PWCTRL1, &arg, 1);
	}

	/* VDV and VRH command enables (C2h) */
	{
		uint8_t arg = 0x01;
		st7789_write_cmd_data(handle, 0xC2, &arg, 1);
	}

	/* VRH set (C3h) */
	{
		uint8_t arg = 0x0B;
		st7789_write_cmd_data(handle, 0xC3, &arg, 1);
	}

	/* VDV set (C4h) */
	{
		uint8_t arg = 0x20;
		st7789_write_cmd_data(handle, 0xC4, &arg, 1);
	}

	/* GMCTRP (E0h) — positive gamma */
	{
		uint8_t args[] = {0xD0, 0x04, 0x0D, 0x11, 0x13,
				  0x2B, 0x3F, 0x54, 0x4C, 0x18,
				  0x0D, 0x0B, 0x1F, 0x23};
		st7789_write_cmd_data(handle, ST7789_CMD_GMCTRP1, args, 14);
	}

	/* GMCTRN (E1h) — negative gamma */
	{
		uint8_t args[] = {0xD0, 0x04, 0x0C, 0x11, 0x13,
				  0x2C, 0x3F, 0x44, 0x51, 0x2F,
				  0x1F, 0x1F, 0x20, 0x23};
		st7789_write_cmd_data(handle, ST7789_CMD_GMCTRN1, args, 14);
	}

	/* Display Inversion Off */
	st7789_write_cmd(handle, ST7789_CMD_INVOFF);

	/* Display On */
	st7789_write_cmd(handle, ST7789_CMD_DISPON);
	vTaskDelay(pdMS_TO_TICKS(10));

	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Public API — Init                                                         */
/* ------------------------------------------------------------------------- */
esp_err_t st7789_init(st7789_handle_t **out_handle,
		      const st7789_config_t *config)
{
	if (out_handle == NULL || config == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	st7789_handle_t *handle = (st7789_handle_t *)
		calloc(1, sizeof(*handle));
	if (handle == NULL) return ESP_ERR_NO_MEM;

	handle->config = *config;

	/* ---- Configure DC and RST GPIOs ---- */
	gpio_config_t io_conf = {
		.pin_bit_mask = (1ULL << config->dc_gpio) |
				((config->rst_gpio >= 0) ? (1ULL << config->rst_gpio) : 0) |
				((config->blk_gpio >= 0) ? (1ULL << config->blk_gpio) : 0),
		.mode         = GPIO_MODE_OUTPUT,
		.pull_up_en   = GPIO_PULLUP_DISABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type    = GPIO_INTR_DISABLE,
	};
	gpio_config(&io_conf);

	if (config->blk_gpio >= 0) {
		gpio_set_level(config->blk_gpio, 1);  /* backlight on */
	}

	/* ---- Add SPI device ---- */
	spi_device_interface_config_t dev_cfg = {
		.mode           = 3,           /* CPOL=1, CPHA=1 */
		.clock_speed_hz = config->freq_hz,
		.spics_io_num   = config->cs_gpio,
		.queue_size     = 1,
		.clock_source   = SPI_CLK_SRC_DEFAULT,
		.flags          = 0,
	};

	esp_err_t err = spi_bus_add_device(config->host, &dev_cfg,
					   &handle->spi_dev);
	if (err != ESP_OK) {
		ESP_LOGE(ST7789_TAG, "spi_bus_add_device: %s",
			 esp_err_to_name(err));
		free(handle);
		return err;
	}

	/* ---- Hardware reset ---- */
	st7789_hardware_reset(handle);

	/* ---- Init sequence ---- */
	err = st7789_init_sequence(handle);
	if (err != ESP_OK) {
		spi_bus_remove_device(handle->spi_dev);
		free(handle);
		return err;
	}

	*out_handle = handle;
	ESP_LOGI(ST7789_TAG, "ST7789 ready — 240×240 RGB565");
	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Destroy                                                                   */
/* ------------------------------------------------------------------------- */
void st7789_destroy(st7789_handle_t *handle)
{
	if (handle == NULL) return;
	spi_bus_remove_device(handle->spi_dev);
	if (handle->config.blk_gpio >= 0) {
		gpio_set_level(handle->config.blk_gpio, 0);
	}
	free(handle);
}

/* ------------------------------------------------------------------------- */
/*  Backlight                                                                */
/* ------------------------------------------------------------------------- */
esp_err_t st7789_set_backlight(st7789_handle_t *handle, uint8_t brightness)
{
	if (handle == NULL) return ESP_ERR_INVALID_ARG;
	if (handle->config.blk_gpio < 0) return ESP_ERR_NOT_SUPPORTED;

	/* Simple on/off via PWM could be added; for now use binary control */
	gpio_set_level(handle->config.blk_gpio, brightness > 0);
	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Display on/off                                                           */
/* ------------------------------------------------------------------------- */
esp_err_t st7789_display_on(st7789_handle_t *handle, bool on)
{
	if (handle == NULL) return ESP_ERR_INVALID_ARG;
	return st7789_write_cmd(handle,
				on ? ST7789_CMD_DISPON : ST7789_CMD_DISPOFF);
}

/* ------------------------------------------------------------------------- */
/*  Window                                                                   */
/* ------------------------------------------------------------------------- */
esp_err_t st7789_set_window(st7789_handle_t *handle,
			    uint16_t x0, uint16_t y0,
			    uint16_t x1, uint16_t y1)
{
	if (handle == NULL) return ESP_ERR_INVALID_ARG;

	uint8_t col_args[] = {
		(x0 >> 8) & 0xFF, x0 & 0xFF,
		(x1 >> 8) & 0xFF, x1 & 0xFF,
	};
	uint8_t row_args[] = {
		(y0 >> 8) & 0xFF, y0 & 0xFF,
		(y1 >> 8) & 0xFF, y1 & 0xFF,
	};

	esp_err_t err = st7789_write_cmd_data(handle, ST7789_CMD_CASET,
					      col_args, 4);
	if (err != ESP_OK) return err;
	err = st7789_write_cmd_data(handle, ST7789_CMD_RASET,
				    row_args, 4);
	return err;
}

/* ------------------------------------------------------------------------- */
/*  Write pixels                                                              */
/* ------------------------------------------------------------------------- */
esp_err_t st7789_write_pixels(st7789_handle_t *handle,
			      const uint8_t *data, size_t len)
{
	if (handle == NULL || data == NULL) return ESP_ERR_INVALID_ARG;

	esp_err_t err = st7789_write_cmd(handle, ST7789_CMD_RAMWR);
	if (err != ESP_OK) return err;

	return st7789_write_data(handle, data, len);
}

/* ------------------------------------------------------------------------- */
/*  Fill screen                                                               */
/* ------------------------------------------------------------------------- */
esp_err_t st7789_fill_screen(st7789_handle_t *handle, uint16_t color)
{
	if (handle == NULL) return ESP_ERR_INVALID_ARG;

	/* Full screen window */
	esp_err_t err = st7789_set_window(handle, 0, 0,
					  ST7789_LCD_WIDTH - 1,
					  ST7789_LCD_HEIGHT - 1);
	if (err != ESP_OK) return err;

	err = st7789_write_cmd(handle, ST7789_CMD_RAMWR);
	if (err != ESP_OK) return err;

	/* Fill in chunks to avoid huge stack allocation */
	uint8_t chunk[1024];  /* 512 pixels at a time */
	size_t total = ST7789_LCD_WIDTH * ST7789_LCD_HEIGHT;
	size_t chunk_pixels = sizeof(chunk) / 2;

	for (size_t i = 0; i < chunk_pixels; i++) {
		chunk[i * 2]     = (color >> 8) & 0xFF;
		chunk[i * 2 + 1] = color & 0xFF;
	}

	for (size_t sent = 0; sent < total; sent += chunk_pixels) {
		size_t n = (total - sent > chunk_pixels) ? chunk_pixels
							 : total - sent;
		gpio_set_level(handle->config.dc_gpio, 1);
		spi_transaction_t t = {
			.length    = n * 2 * 8,
			.tx_buffer = chunk,
		};
		err = spi_device_transmit(handle->spi_dev, &t);
		if (err != ESP_OK) return err;
	}

	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Display image from Flash                                                  */
/* ------------------------------------------------------------------------- */
esp_err_t st7789_display_from_flash(st7789_handle_t *handle,
				    uint32_t img_offset,
				    uint16_t width, uint16_t height)
{
	if (handle == NULL) return ESP_ERR_INVALID_ARG;
	if (width == 0 || height == 0) return ESP_ERR_INVALID_ARG;

	esp_err_t err = st7789_set_window(handle, 0, 0,
					  width - 1, height - 1);
	if (err != ESP_OK) return err;

	err = st7789_write_cmd(handle, ST7789_CMD_RAMWR);
	if (err != ESP_OK) return err;

	/* Stream from Flash to display in chunks via an external read
	 * function.  This function requires an external symbol from
	 * storage_mgr — declared here for coupling. */
	extern esp_err_t img_cache_read(uint32_t offset,
					uint8_t *data, size_t len);

	uint8_t buf[2048];  /* 1024 pixels per chunk */
	size_t total_pixels = (size_t)width * height;
	size_t pixels_per_chunk = sizeof(buf) / 2;

	for (size_t done = 0; done < total_pixels; done += pixels_per_chunk) {
		size_t n = total_pixels - done;
		if (n > pixels_per_chunk) n = pixels_per_chunk;
		size_t n_bytes = n * 2;

		err = img_cache_read(img_offset + done * 2, buf, n_bytes);
		if (err != ESP_OK) return err;

		gpio_set_level(handle->config.dc_gpio, 1);
		spi_transaction_t t = {
			.length    = n_bytes * 8,
			.tx_buffer = buf,
		};
		err = spi_device_transmit(handle->spi_dev, &t);
		if (err != ESP_OK) return err;
	}

	return ESP_OK;
}
