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
/*  ST7789V 240×240 RGB LCD Controller (SPI 4-wire serial interface)        */
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/*  Display Geometry                                                         */
/* ------------------------------------------------------------------------- */
#define ST7789_LCD_WIDTH   240
#define ST7789_LCD_HEIGHT  240
#define ST7789_BYTES_PER_PIXEL  2   /* RGB565 */

/* ------------------------------------------------------------------------- */
/*  ST7789 Command Set                                                       */
/* ------------------------------------------------------------------------- */
#define ST7789_CMD_NOP          0x00
#define ST7789_CMD_SWRESET      0x01
#define ST7789_CMD_RDDID        0x04
#define ST7789_CMD_RDDST        0x09
#define ST7789_CMD_SLPIN        0x10
#define ST7789_CMD_SLPOUT       0x11
#define ST7789_CMD_PTLON        0x12
#define ST7789_CMD_NORON        0x13
#define ST7789_CMD_INVOFF       0x20
#define ST7789_CMD_INVON        0x21
#define ST7789_CMD_DISPOFF      0x28
#define ST7789_CMD_DISPON       0x29
#define ST7789_CMD_CASET        0x2A
#define ST7789_CMD_RASET        0x2B
#define ST7789_CMD_RAMWR        0x2C
#define ST7789_CMD_RAMRD        0x2E
#define ST7789_CMD_PTLAR        0x30
#define ST7789_CMD_TEOFF        0x34
#define ST7789_CMD_TEON         0x35
#define ST7789_CMD_MADCTL       0x36
#define ST7789_CMD_IDMOFF       0x38
#define ST7789_CMD_IDMON        0x39
#define ST7789_CMD_COLMOD       0x3A
#define ST7789_CMD_RAMWRC       0x3C
#define ST7789_CMD_RAMRDC       0x3E
#define ST7789_CMD_STE          0x44
#define ST7789_CMD_GSCAN        0x45
#define ST7789_CMD_WRDISBV      0x51
#define ST7789_CMD_RDDISBV      0x52
#define ST7789_CMD_WRCTRLD      0x53
#define ST7789_CMD_WRCACE       0x55
#define ST7789_CMD_WRCABCMB     0x5E
#define ST7789_CMD_RDID1        0xDA
#define ST7789_CMD_RDID2        0xDB
#define ST7789_CMD_RDID3        0xDC
#define ST7789_CMD_RAMCTRL      0xB0
#define ST7789_CMD_RGBCTRL      0xB1
#define ST7789_CMD_PORCTRL      0xB2
#define ST7789_CMD_FRCTRL1      0xB3
#define ST7789_CMD_PARCTRL      0xB5
#define ST7789_CMD_GCTRL        0xB7
#define ST7789_CMD_GTIMING      0xB8
#define ST7789_CMD_DGMEN        0xBA
#define ST7789_CMD_PWCTRL1      0xC0
#define ST7789_CMD_PWCTRL2      0xC1
#define ST7789_CMD_VMCTRL1      0xC5
#define ST7789_CMD_VMOFCTRL     0xC7
#define ST7789_CMD_PWCTRL3      0xC8
#define ST7789_CMD_FRCTRL2      0xC6
#define ST7789_CMD_PWCTRL4      0xCB
#define ST7789_CMD_PWCTRL5      0xCC
#define ST7789_CMD_GMCTRP1      0xE0
#define ST7789_CMD_GMCTRN1      0xE1
#define ST7789_CMD_PWCTRL6      0xFC

/* MADCTL bits */
#define ST7789_MADCTL_MY        0x80
#define ST7789_MADCTL_MX        0x40
#define ST7789_MADCTL_MV        0x20
#define ST7789_MADCTL_ML        0x10
#define ST7789_MADCTL_BGR       0x08
#define ST7789_MADCTL_MH        0x04
#define ST7789_MADCTL_RGB       0x00

/* COLMOD values */
#define ST7789_COLMOD_65K       0x05    /* 16-bit RGB565 */
#define ST7789_COLMOD_262K      0x06    /* 18-bit RGB666 */
#define ST7789_COLMOD_16M       0x07    /* 24-bit RGB888 */

/* ------------------------------------------------------------------------- */
/*  Configuration                                                            */
/* ------------------------------------------------------------------------- */
typedef struct {
	spi_host_device_t  host;        /* SPI host (e.g. SPI2_HOST)         */
	int                cs_gpio;     /* Chip Select GPIO                  */
	int                dc_gpio;     /* Data/Command GPIO                 */
	int                rst_gpio;    /* Reset GPIO, -1 to skip            */
	int                blk_gpio;    /* Backlight GPIO, -1 if none        */
	uint32_t           freq_hz;     /* SPI clock frequency (Hz)          */
} st7789_config_t;

/* ------------------------------------------------------------------------- */
/*  Handle                                                                   */
/* ------------------------------------------------------------------------- */
typedef struct st7789_handle_t st7789_handle_t;

/* ------------------------------------------------------------------------- */
/*  Public API                                                               */
/* ------------------------------------------------------------------------- */

/**
 * @brief  Create and initialise ST7789 display.
 *
 * Performs hardware reset, sends init sequence (SLPOUT, COLMOD, MADCTL, etc.)
 * and turns the display on.
 *
 * @param  out_handle  [out] display handle
 * @param  config      pin and SPI configuration
 * @return ESP_OK on success
 */
esp_err_t st7789_init(st7789_handle_t **out_handle,
		      const st7789_config_t *config);

/**
 * @brief  Destroy handle and free resources.
 */
void st7789_destroy(st7789_handle_t *handle);

/**
 * @brief  Set the backlight brightness (if BLK GPIO configured).
 * @param  brightness  0 (off) … 100 (max)
 */
esp_err_t st7789_set_backlight(st7789_handle_t *handle, uint8_t brightness);

/**
 * @brief  Turn display on/off.
 */
esp_err_t st7789_display_on(st7789_handle_t *handle, bool on);

/**
 * @brief  Set the drawing window (column and row range).
 *
 * Subsequent st7789_write_pixels() calls will write into this window.
 */
esp_err_t st7789_set_window(st7789_handle_t *handle,
			    uint16_t x0, uint16_t y0,
			    uint16_t x1, uint16_t y1);

/**
 * @brief  Write a buffer of RGB565 pixel data to the current window.
 *
 * Buffer length must be (width * height * 2) bytes.
 */
esp_err_t st7789_write_pixels(st7789_handle_t *handle,
			      const uint8_t *data, size_t len);

/**
 * @brief  Fill the entire screen with a single RGB565 colour.
 */
esp_err_t st7789_fill_screen(st7789_handle_t *handle, uint16_t color);

/**
 * @brief  Display an RGB565 image from Flash (image cache region).
 *
 * Reads pixel data from the Flash image cache via storage_mgr and writes
 * to the display.  This avoids having the full frame buffer in RAM.
 *
 * @param  handle     display handle
 * @param  img_offset Flash byte offset within IMG_CACHE region
 * @param  width      image width in pixels
 * @param  height     image height in pixels
 */
esp_err_t st7789_display_from_flash(st7789_handle_t *handle,
				    uint32_t img_offset,
				    uint16_t width, uint16_t height);

#ifdef __cplusplus
}
#endif
