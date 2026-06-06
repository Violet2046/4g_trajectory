/**
 * @file    epd_qyeg0397.h
 * @brief   Driver for QYEG0397RYS677F3 4-colour e-paper display (800×480)
 *
 * Specifications:
 *   - Size:      3.97" 800×480 pixels
 *   - Colours:   Black / White / Red / Yellow (2-bit per pixel)
 *   - SPI:       4-wire, Mode 0, ≤20 MHz
 *   - Controller: Solomon SSD1680 / compatible
 *   - Waveform:  Pre-stored in on-chip OTP
 *
 * Data format (2-bit per pixel, packed 4 pixels/byte):
 *   byte = [px3(2b)] [px2(2b)] [px1(2b)] [px0(2b)]
 *   pixel codes: 00=Black, 01=White, 10=Yellow, 11=Red
 *   Scan order:  left→right, top→bottom
 */

#ifndef EPD_QYEG0397_H
#define EPD_QYEG0397_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*  Display constants                                                        */
/* ========================================================================= */
#define EPD_WIDTH           800UL
#define EPD_HEIGHT          480UL
#define EPD_DATA_SIZE       (EPD_WIDTH * EPD_HEIGHT / 4UL)  /* 96000 B */

/* ========================================================================= */
/*  Pixel colour codes (2-bit)                                               */
/* ========================================================================= */
#define EPD_COLOR_BLACK     0x00
#define EPD_COLOR_WHITE     0x01
#define EPD_COLOR_YELLOW    0x02
#define EPD_COLOR_RED       0x03

/* ========================================================================= */
/*  Hardware configuration structure                                         */
/* ========================================================================= */
typedef struct {
    uint8_t  spi_host;          /* SPI2_HOST / SPI3_HOST                    */
    uint32_t clk_speed_hz;      /* SPI clock, ≤20 MHz recommended           */
    int      pin_cs;            /* Chip-select (active low)                 */
    int      pin_dc;            /* Data/Command (H=data, L=command)         */
    int      pin_rst;           /* Reset (active low)                       */
    int      pin_busy;          /* Busy (L=busy, H=ready)                   */
    int      pin_mosi;          /* MOSI (SDA)                               */
    int      pin_sclk;          /* SCK (SCL)                                */
} epd_config_t;

/* ========================================================================= */
/*  Public API                                                               */
/* ========================================================================= */

/**
 * @brief  Initialise SPI bus, GPIOs and EPD controller registers.
 *
 * Performs hardware reset, then executes the full "Enter Solomon Command"
 * initialisation sequence matching the datasheet OTP reference code.
 * After epd_init() the display is powered off.  Call epd_update_full()
 * to show content.
 *
 * @param  config  Pin / speed configuration.
 * @return ESP_OK on success.
 */
esp_err_t epd_init(const epd_config_t *config);

/**
 * @brief  Initialise EPD on an existing (already-initialised) SPI bus.
 *
 * Use this when sharing the SPI bus with other devices (e.g. W25Q64).
 * The caller must already have called spi_bus_initialize() for
 * @p config->spi_host.
 *
 * @param  config  Pin / speed configuration.
 * @return ESP_OK on success.
 */
esp_err_t epd_init_shared_bus(const epd_config_t *config);

/**
 * @brief  Free all SPI resources.
 */
esp_err_t epd_deinit(void);

/* ---- Power control ---------------------------------------------------- */

/**
 * @brief  Power ON (CMD 0x04).  Waits BUSY after.
 */
esp_err_t epd_power_on(void);

/**
 * @brief  Power OFF (CMD 0x02 + 0x00).  Waits BUSY.
 */
esp_err_t epd_power_off(void);

/**
 * @brief  Deep sleep (CMD 0x07 + 0xA5).  Call epd_init() to wake.
 */
esp_err_t epd_sleep(void);

/* ---- Update ----------------------------------------------------------- */

/**
 * @brief  Full-screen image update.
 *
 * Sequence: power-on → DTM → image data → DRF+0x00 → wait → power-off.
 *
 * @param  image_data  96000 bytes of 2-bit packed pixel data.
 * @return ESP_OK or ESP_ERR_TIMEOUT.
 */
esp_err_t epd_update_full(const uint8_t *image_data);

/**
 * @brief  Partial-area update (must follow an initial full update).
 *
 * @param  data    Full frame buffer (EPD_DATA_SIZE bytes); driver sends
 *                 only the window portion.
 * @param  x, y    Top-left corner.
 * @param  w, h    Size (must be multiples of 8).
 * @return ESP_OK on success.
 */
esp_err_t epd_update_partial(const uint8_t *data,
                             uint16_t x, uint16_t y,
                             uint16_t w, uint16_t h);

/* ---- Low-level helpers ------------------------------------------------- */

esp_err_t epd_wait_busy(uint32_t timeout_ms);
void     epd_hard_reset(void);
void     epd_write_cmd(uint8_t cmd);
void     epd_write_data_byte(uint8_t data);
void     epd_write_data(const uint8_t *data, size_t len);

/* ---- Image-buffer helpers -------------------------------------------- */

void epd_fill_buffer(uint8_t *buf, uint8_t colour);
void epd_set_pixel(uint8_t *buf, uint16_t x, uint16_t y, uint8_t colour);
void epd_draw_hline(uint8_t *buf, uint16_t x1, uint16_t x2,
                    uint16_t y, uint8_t colour);
void epd_draw_vline(uint8_t *buf, uint16_t x, uint16_t y1,
                    uint16_t y2, uint8_t colour);
void epd_fill_rect(uint8_t *buf, uint16_t x, uint16_t y,
                   uint16_t w, uint16_t h, uint8_t colour);

/* ---- Overlay window constants ---- */
#define OVERLAY_X       (EPD_WIDTH  - 272)   /* left edge, 272px from right */
#define OVERLAY_Y       (EPD_HEIGHT - 168)   /* top edge,  168px from bottom */
#define OVERLAY_W       264                  /* panel width (multiple of 8) */
#define OVERLAY_H       160                  /* panel height (multiple of 8) */
#define OVERLAY_MARGIN  4
#define OVERLAY_PAD_X   4
#define OVERLAY_PAD_Y   4

/* ---- Sensor overlay -------------------------------------------------- */

/**
 * @brief  Sensor sample data passed to epd_sensor_overlay().
 */
typedef struct {
    /* Accelerometer (g) */
    float acc_x, acc_y, acc_z;
    /* Gyroscope (dps) */
    float gyr_x, gyr_y, gyr_z;
    /* Magnetometer (µT) */
    float mag_x, mag_y, mag_z;
    /* GPS */
    bool  gps_fix;
    float lat, lon;
} epd_sensor_data_t;

/**
 * @brief  Render sensor data as text overlay in the bottom-right corner
 *         and perform a partial update.
 *
 * The overlay panel is 264×160 pixels anchored at the lower-right.
 * Background = white, text = black.
 *
 * @param  buf   EPD frame buffer (EPD_DATA_SIZE).  Existing content
 *               (e.g. a background image) is preserved; only the
 *               overlay panel is overwritten.
 * @param  data  Latest sensor values.
 * @return ESP_OK on success.
 */
esp_err_t epd_sensor_overlay(uint8_t *buf, const epd_sensor_data_t *data);

/**
 * @brief  Render sensor overlay into a compact window buffer
 *         (no full 96 KB frame buffer needed).
 *
 * The buffer must be (OVERLAY_W * OVERLAY_H) / 4 bytes (~10 KB).
 * Internally calls epd_update_partial_window() for the partial refresh.
 *
 * @param  window_buf  Compact overlay window buffer (~10 KB).
 * @param  data        Sensor readings.
 * @return ESP_OK on success.
 */
esp_err_t epd_sensor_overlay_window(uint8_t *window_buf,
                                    const epd_sensor_data_t *data);

/* ---- BLE image display on EPD --------------------------------------- */

/**
 * @brief  Read an image from SPI Flash (IMG_CACHE region) and full-refresh
 *         it onto the EPD.
 *
 * The image is expected to be pre-loaded into Flash at @p flash_offset
 * by the BLE receiver.  The data format must be EPD-compatible (2-bit
 * packed, 96000 bytes).
 *
 * @param  buf           EPD frame buffer (EPD_DATA_SIZE).
 * @param  flash_offset  Byte offset in Flash where the image starts.
 * @return ESP_OK on success.
 */
esp_err_t epd_display_from_flash(uint8_t *buf, uint32_t flash_offset);

/**
 * @brief  Full-refresh EPD with image stored in SPI Flash, reading in
 *         small chunks (no large RAM buffer needed).
 *
 * The image is read from Flash and sent to the EPD directly in 4 KB
 * chunks, avoiding the need for a 96000-byte contiguous RAM buffer.
 *
 * @param  flash_offset  Byte offset in Flash where the image starts.
 * @return ESP_OK on success.
 */
esp_err_t epd_update_full_from_flash(uint32_t flash_offset);

/**
 * @brief  Set the W25Q64 flash handle used by epd_display_from_flash().
 *         Must be called once before displaying BLE images.
 */
void epd_set_flash_handle(void *flash_handle);

#ifdef __cplusplus
}
#endif

#endif /* EPD_QYEG0397_H */