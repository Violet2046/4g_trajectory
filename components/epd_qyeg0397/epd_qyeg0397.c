/**
 * @file    epd_qyeg0397.c
 * @brief   Driver for QYEG0397RYS677F3 4-colour e-paper display
 *
 * Based on official datasheet v2.0 (2024-09-12).
 * Uses on-chip OTP waveform (no external LUT load required).
 *
 * Initialisation sequence follows the "OTP Operation Reference
 * Program Code" from section 10.2 of the specification.
 */

#include "epd_qyeg0397.h"
#include "epd_font.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "EPD_QYEG0397";

static spi_device_handle_t spi_handle = NULL;
static epd_config_t epd_cfg;
static bool s_bus_owned = false;   /* true if we called spi_bus_initialize */

/* ========================================================================= */
/*  Command codes (from spec section 7)                                     */
/* ========================================================================= */
#define CMD_PSR         0x00    /* Panel Setting                           */
#define CMD_PWR         0x01    /* Power Setting (11 params)               */
#define CMD_POF         0x02    /* Power OFF                               */
#define CMD_PON         0x04    /* Power ON                                */
#define CMD_BTST        0x06    /* Booster Soft Start                      */
#define CMD_DSLP        0x07    /* Deep Sleep                              */
#define CMD_DTM         0x10    /* Data Start Transmission                 */
#define CMD_DSP         0x11    /* Data Stop                               */
#define CMD_DRF         0x12    /* Display Refresh                         */
#define CMD_PLL         0x30    /* PLL Control                             */
#define CMD_CDI         0x50    /* VCOM & Data Interval                    */
#define CMD_TEMP        0x62    /* Temperature Sensor setting              */
#define CMD_GSST        0x65    /* Gate/Source Start                       */
#define CMD_TRES        0x61    /* Resolution Setting                      */
#define CMD_PWS         0xE3    /* Power Saving                            */
#define CMD_XON         0xE9    /* XON                                    */
/* Extended registers (Solomon-specific) */
#define CMD_CR_MISC_A   0xE0    /* Cascade / misc setting A               */
#define CMD_CR_MISC_B   0xE7    /* Cascade / misc setting B               */

/* ========================================================================= */
/*  Timeouts (ms)                                                            */
/* ========================================================================= */
#define BUSY_PON_TIMEOUT_MS     500     /* Power-on stabilisation          */
#define BUSY_REFRESH_TIMEOUT_MS 25000   /* Full refresh ~18 s, 25 s safe  */
#define BUSY_POF_TIMEOUT_MS     500     /* Power-off ~80 ms                */
#define BUSY_POLL_INTERVAL_MS   5

/* ========================================================================= */
/*  SPI transfer helper                                                      */
/* ========================================================================= */
static esp_err_t spi_transfer(const uint8_t *data, size_t len, uint8_t dc_level)
{
    if (!spi_handle || !data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_set_level(epd_cfg.pin_dc, dc_level);

    spi_transaction_t trans = {
        .length = len * 8,
        .tx_buffer = data,
    };
    return spi_device_transmit(spi_handle, &trans);
}

/* ========================================================================= */
/*  Busy wait                                                                */
/* ========================================================================= */

/**
 * @brief  Wait for BUSY pin to go HIGH (ready).
 *
 * Per spec Note 5-4: Low = busy (don't interrupt), High = ready.
 */
esp_err_t epd_wait_busy(uint32_t timeout_ms)
{
    TickType_t start = xTaskGetTickCount();
    while (gpio_get_level(epd_cfg.pin_busy) == 0) {  /* 0 = busy */
        if ((xTaskGetTickCount() - start) >= pdMS_TO_TICKS(timeout_ms)) {
            ESP_LOGE(TAG, "Busy timeout after %lu ms", (unsigned long)timeout_ms);
            return ESP_ERR_TIMEOUT;
        }
        vTaskDelay(pdMS_TO_TICKS(BUSY_POLL_INTERVAL_MS));
    }
    return ESP_OK;
}

/* ========================================================================= */
/*  Low-level SPI commands                                                   */
/* ========================================================================= */
void epd_write_cmd(uint8_t cmd)
{
    spi_transfer(&cmd, 1, 0);   /* DC low = command */
}

void epd_write_data_byte(uint8_t data)
{
    spi_transfer(&data, 1, 1);  /* DC high = data */
}

void epd_write_data(const uint8_t *data, size_t len)
{
    if (len == 0) return;
    spi_transfer(data, len, 1);
}

/* ========================================================================= */
/*  Hardware reset                                                           */
/* ========================================================================= */
void epd_hard_reset(void)
{
    gpio_set_level(epd_cfg.pin_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));      /* ≥ 10 µs per spec                 */
    gpio_set_level(epd_cfg.pin_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));      /* wait for IC ready                */
    /* After reset, the controller initialises itself; wait busy. */
    epd_wait_busy(BUSY_PON_TIMEOUT_MS);
}

/* ========================================================================= */
/*  "Enter Solomon Command" – OTP initialisation sequence                   */
/*  (from spec section 10.2)                                                 */
/* ========================================================================= */
static void epd_otp_init_seq(void)
{
    /* ---- 1. Panel Setting (PSR, 0x00) ---- */
    {
        /* 0x2B = .RES_800x480, LUT_OTP, .SHL_R2L, .SHD_N, .RST */
        /* 0x29 = scan order, border waveform */
        uint8_t d[] = { 0x2B, 0x29 };
        epd_write_cmd(CMD_PSR);
        epd_write_data(d, sizeof(d));
    }

    /* ---- 2. Booster Soft Start (BTST, 0x06) ---- */
    {
        uint8_t d[] = { 0x0F, 0x8B, 0x93, 0xA4 };
        epd_write_cmd(CMD_BTST);
        epd_write_data(d, sizeof(d));
    }

    /* ---- 3. VCOM & Data Interval (CDI, 0x50) ---- */
    {
        /* 0x37: VCOM=0.5V? + data interval for 4-colour OTP */
        epd_write_cmd(CMD_CDI);
        epd_write_data_byte(0x37);
    }

    /* ---- 4. Resolution (TRES, 0x61) ---- */
    {
        /* HRES = 800 (0x0320), VRES = 480 (0x01E0) */
        uint8_t d[] = { 0x20, 0x03, 0xE0, 0x01, 0x00 };
        epd_write_cmd(CMD_TRES);
        epd_write_data(d, sizeof(d));
    }

    /* ---- 5. Gate/Source Start (GSST, 0x65) ---- */
    {
        uint8_t d[] = { 0x00, 0x00, 0x00, 0x00 };
        epd_write_cmd(CMD_GSST);
        epd_write_data(d, sizeof(d));
    }

    /* ---- 6. Misc cascade A (0xE0) ---- */
    epd_write_cmd(CMD_CR_MISC_A);
    epd_write_data_byte(0x10);

    /* ---- 7. Misc cascade B (0xE7) ---- */
    epd_write_cmd(CMD_CR_MISC_B);
    epd_write_data_byte(0xA4);

    /* ---- 8. XON (0xE9) ---- */
    epd_write_cmd(CMD_XON);
    epd_write_data_byte(0x01);

    /* ---- 9. PLL Control (0x30) ---- */
    /* 0x08: 2 MHz ? Actual frequency depends on oscillator */
    epd_write_cmd(CMD_PLL);
    epd_write_data_byte(0x08);

    /* ---- 10. Temperature Sensor (0x62) ---- */
    {
        uint8_t d[] = { 0x76, 0x76, 0x76, 0x5A,
                        0x9D, 0x8A, 0x76, 0x62 };
        epd_write_cmd(CMD_TEMP);
        epd_write_data(d, sizeof(d));
    }

    ESP_LOGD(TAG, "OTP init sequence done");
}

/* ========================================================================= */
/*  Power control                                                            */
/* ========================================================================= */
esp_err_t epd_power_on(void)
{
    epd_write_cmd(CMD_PON);
    esp_err_t err = epd_wait_busy(BUSY_PON_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "PON wait busy timeout");
    }
    return err;
}

esp_err_t epd_power_off(void)
{
    epd_write_cmd(CMD_POF);
    epd_write_data_byte(0x00);
    return epd_wait_busy(BUSY_POF_TIMEOUT_MS);
}

esp_err_t epd_sleep(void)
{
    esp_err_t err = epd_power_off();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "POF before sleep failed (ignored)");
    }
    epd_write_cmd(CMD_DSLP);
    epd_write_data_byte(0xA5);
    vTaskDelay(pdMS_TO_TICKS(20));
    ESP_LOGI(TAG, "Entered deep sleep");
    return ESP_OK;
}

/* ========================================================================= */
/*  Full-screen update                                                       */
/* ========================================================================= */
esp_err_t epd_update_full(const uint8_t *image_data)
{
    if (!image_data) return ESP_ERR_INVALID_ARG;

    /* 1. Power ON */
    esp_err_t err = epd_power_on();
    if (err != ESP_OK) return err;

    /* 2. Data Start Transmission */
    epd_write_cmd(CMD_DTM);

    /* 3. Send complete image (96000 bytes of 2-bit packed data) */
    epd_write_data(image_data, EPD_DATA_SIZE);

    /* 4. Issue Display Refresh command */
    epd_write_cmd(CMD_DRF);
    epd_write_data_byte(0x00);

    /* 5. Wait for refresh to complete */
    err = epd_wait_busy(BUSY_REFRESH_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Refresh timeout");
        return err;
    }

    /* 6. Power OFF */
    err = epd_power_off();

    ESP_LOGI(TAG, "Full refresh done");
    return err;
}

/* ========================================================================= */
/*  Partial update                                                           */
/* ========================================================================= */
esp_err_t epd_update_partial(const uint8_t *data,
                             uint16_t x, uint16_t y,
                             uint16_t w, uint16_t h)
{
    if (!data) return ESP_ERR_INVALID_ARG;
    if (x + w > EPD_WIDTH || y + h > EPD_HEIGHT) return ESP_ERR_INVALID_SIZE;
    if ((w & 7) || (h & 7)) {
        ESP_LOGW(TAG, "Partial dims not multiple of 8 (%dx%d)", w, h);
    }

    /* Power ON */
    esp_err_t err = epd_power_on();
    if (err != ESP_OK) return err;

    /* Set window (CMD 0x11 = DSP, then CMD 0x15, 0x16) */
    /* Note: partial requires 0x11 first to stop previous transmission */
    epd_write_cmd(0x11);                /* Data Stop */

    /* Window: x-start, x-end, y-start, y-end (each as word, LSByte first) */
    {
        uint8_t win_x[] = { (uint8_t)(x & 0xFF),  (uint8_t)((x >> 8) & 0xFF),
                            (uint8_t)((x + w - 1) & 0xFF),
                            (uint8_t)(((x + w - 1) >> 8) & 0xFF) };
        epd_write_cmd(0x15);            /* Write Reg for x window */
        epd_write_data(win_x, sizeof(win_x));
    }
    {
        uint8_t win_y[] = { (uint8_t)(y & 0xFF),  (uint8_t)((y >> 8) & 0xFF),
                            (uint8_t)((y + h - 1) & 0xFF),
                            (uint8_t)(((y + h - 1) >> 8) & 0xFF) };
        epd_write_cmd(0x16);            /* Write Reg for y window */
        epd_write_data(win_y, sizeof(win_y));
    }

    /* Send only the window portion of the frame buffer */
    /* Since the controller expects full-width lines even for partial updates,
     * we need to send the entire lines for the window region. */
    epd_write_cmd(CMD_DTM);             /* Data Start Transmission */
    for (uint16_t row = y; row < y + h; row++) {
        uint32_t offset = (row * (EPD_WIDTH / 4UL)) + (x / 4UL);
        uint32_t len_bytes = (w + 3) / 4UL;  /* bytes for w pixels @ 4px/B */
        epd_write_data(data + offset, len_bytes);
    }

    /* Display Refresh */
    epd_write_cmd(CMD_DRF);
    epd_write_data_byte(0x00);

    err = epd_wait_busy(BUSY_REFRESH_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Partial refresh timeout");
        return err;
    }

    err = epd_power_off();

    ESP_LOGI(TAG, "Partial refresh (%d,%d %dx%d) done", x, y, w, h);
    return err;
}

/* ----------------------------------------------------------------------- */
/*  Partial update — window-only buffer (no full frame buffer needed)       */
/* ----------------------------------------------------------------------- */
esp_err_t epd_update_partial_window(const uint8_t *window_data,
                                     uint16_t x, uint16_t y,
                                     uint16_t w, uint16_t h)
{
    if (!window_data) return ESP_ERR_INVALID_ARG;
    if (x + w > EPD_WIDTH || y + h > EPD_HEIGHT) return ESP_ERR_INVALID_SIZE;

    esp_err_t err = epd_power_on();
    if (err != ESP_OK) return err;

    epd_write_cmd(0x11);                /* Data Stop */

    {
        uint8_t win_x[] = { (uint8_t)(x & 0xFF),  (uint8_t)((x >> 8) & 0xFF),
                            (uint8_t)((x + w - 1) & 0xFF),
                            (uint8_t)(((x + w - 1) >> 8) & 0xFF) };
        epd_write_cmd(0x15);
        epd_write_data(win_x, sizeof(win_x));
    }
    {
        uint8_t win_y[] = { (uint8_t)(y & 0xFF),  (uint8_t)((y >> 8) & 0xFF),
                            (uint8_t)((y + h - 1) & 0xFF),
                            (uint8_t)(((y + h - 1) >> 8) & 0xFF) };
        epd_write_cmd(0x16);
        epd_write_data(win_y, sizeof(win_y));
    }

    /* Send window data — compacted buffer: one row after another */
    {
        uint32_t row_bytes = (w + 3) / 4UL;
        epd_write_cmd(CMD_DTM);
        for (uint16_t row = 0; row < h; row++) {
            epd_write_data(window_data + row * row_bytes, row_bytes);
        }
    }

    epd_write_cmd(CMD_DRF);
    epd_write_data_byte(0x00);

    err = epd_wait_busy(BUSY_REFRESH_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Partial window refresh timeout");
        return err;
    }

    err = epd_power_off();
    return err;
}

/* Forward declarations for Flash I/O used below */
extern esp_err_t w25q64_read(void *handle, uint32_t addr,
                             uint8_t *data, uint32_t len);
extern void *g_epd_flash_handle;

/* ========================================================================= */
/*  Full update — read from SPI Flash in chunks (no large RAM buffer)        */
/* ========================================================================= */
esp_err_t epd_update_full_from_flash(uint32_t flash_offset)
{
    if (!g_epd_flash_handle) return ESP_ERR_INVALID_STATE;

    /* 使用栈缓冲（512 字节）分段读取 Flash 并写入 EPD，不占用堆内存。
     * ESP32-C3 任务栈 8KB，512 字节绰绰有余。
     * SPI DMA 可以直接使用栈地址，无需额外分配。 */
    uint8_t chunk[512];

    esp_err_t err = epd_power_on();
    if (err != ESP_OK) return err;

    epd_write_cmd(CMD_DTM);

    uint32_t remaining = EPD_DATA_SIZE;
    uint32_t addr = flash_offset;
    while (remaining > 0) {
        uint32_t to_read = (remaining < sizeof(chunk)) ? remaining : sizeof(chunk);
        err = w25q64_read(g_epd_flash_handle, addr, chunk, to_read);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Flash read @ 0x%lx: %s",
                     (unsigned long)addr, esp_err_to_name(err));
            epd_power_off();
            return err;
        }
        epd_write_data(chunk, to_read);
        addr += to_read;
        remaining -= to_read;
    }

    epd_write_cmd(CMD_DRF);
    epd_write_data_byte(0x00);

    err = epd_wait_busy(BUSY_REFRESH_TIMEOUT_MS);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Full refresh timeout");
        return err;
    }

    err = epd_power_off();
    ESP_LOGI(TAG, "Full refresh from flash done");
    return err;
}


/* ========================================================================= */
/*  Initialisation / de-initialisation                                       */
/* ========================================================================= */
esp_err_t epd_init(const epd_config_t *config)
{
    if (!config) return ESP_ERR_INVALID_ARG;
    epd_cfg = *config;

    /* ---- 1. Configure GPIOs ---- */
    /* Outputs: CS, DC, RST, MOSI, SCLK */
    {
        uint64_t out_mask = (1ULL << epd_cfg.pin_cs)   |
                            (1ULL << epd_cfg.pin_dc)   |
                            (1ULL << epd_cfg.pin_rst)  |
                            (1ULL << epd_cfg.pin_mosi) |
                            (1ULL << epd_cfg.pin_sclk);
        gpio_config_t io = {
            .pin_bit_mask = out_mask,
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        gpio_config(&io);
    }
    /* Input: BUSY */
    {
        gpio_config_t io = {
            .pin_bit_mask = (1ULL << epd_cfg.pin_busy),
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,
        };
        gpio_config(&io);
    }

    /* CS idle HIGH */
    gpio_set_level(epd_cfg.pin_cs, 1);

    /* ---- 2. Initialise SPI bus ---- */
    spi_bus_config_t bus_cfg = {
        .mosi_io_num     = epd_cfg.pin_mosi,
        .miso_io_num     = -1,
        .sclk_io_num     = epd_cfg.pin_sclk,
        .quadwp_io_num   = -1,
        .quadhd_io_num   = -1,
        .max_transfer_sz = EPD_DATA_SIZE + 64,
    };
    esp_err_t ret = spi_bus_initialize(epd_cfg.spi_host, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize: %s", esp_err_to_name(ret));
        return ret;
    }
    s_bus_owned = true;

    /* ---- 3. Add SPI device ---- */
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz  = epd_cfg.clk_speed_hz,
        .mode            = 0,                    /* CPOL=0, CPHA=0        */
        .spics_io_num    = epd_cfg.pin_cs,
        .queue_size      = 2,
        .flags           = SPI_DEVICE_HALFDUPLEX,
    };
    ret = spi_bus_add_device(epd_cfg.spi_host, &dev_cfg, &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_add_device: %s", esp_err_to_name(ret));
        spi_bus_free(epd_cfg.spi_host);
        return ret;
    }

    /* ---- 4. Hardware reset ---- */
    epd_hard_reset();

    /* ---- 5. OTP initialisation sequence ---- */
    epd_otp_init_seq();

    ESP_LOGI(TAG, "EPD init OK  (800×480, OTP waveform)");
    return ESP_OK;
}

esp_err_t epd_init_shared_bus(const epd_config_t *config)
{
    if (!config) return ESP_ERR_INVALID_ARG;
    epd_cfg = *config;
    s_bus_owned = false;  /* bus already initialised by caller */

    /* ---- 1. Configure GPIOs (skip MOSI/SCLK — managed by bus owner) ---- */
    {
        uint64_t out_mask = (1ULL << epd_cfg.pin_cs)   |
                            (1ULL << epd_cfg.pin_dc)   |
                            (1ULL << epd_cfg.pin_rst);
        gpio_config_t io = {
            .pin_bit_mask = out_mask,
            .mode         = GPIO_MODE_OUTPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,
            .intr_type    = GPIO_INTR_DISABLE,
        };
        gpio_config(&io);
    }
    {
        gpio_config_t io = {
            .pin_bit_mask = (1ULL << epd_cfg.pin_busy),
            .mode         = GPIO_MODE_INPUT,
            .pull_up_en   = GPIO_PULLUP_ENABLE,
        };
        gpio_config(&io);
    }
    gpio_set_level(epd_cfg.pin_cs, 1);

    /* ---- 2. Add SPI device to existing bus ---- */
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz  = epd_cfg.clk_speed_hz,
        .mode            = 0,
        .spics_io_num    = epd_cfg.pin_cs,
        .queue_size      = 2,
        .flags           = SPI_DEVICE_HALFDUPLEX,
    };
    esp_err_t ret = spi_bus_add_device(epd_cfg.spi_host, &dev_cfg,
                                       &spi_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "shared spi_bus_add_device: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ---- 3. Hardware reset + OTP init ---- */
    epd_hard_reset();
    epd_otp_init_seq();

    ESP_LOGI(TAG, "EPD init OK on shared SPI bus (800×480)");
    return ESP_OK;
}

esp_err_t epd_deinit(void)
{
    if (spi_handle) {
        spi_bus_remove_device(spi_handle);
        spi_handle = NULL;
    }
    if (s_bus_owned && epd_cfg.spi_host != 0) {
        spi_bus_free(epd_cfg.spi_host);
    }
    s_bus_owned = false;
    return ESP_OK;
}

/* ========================================================================= */
/*  Image-buffer helper functions                                            */
/* ========================================================================= */

void epd_fill_buffer(uint8_t *buf, uint8_t colour)
{
    if (!buf) return;
    uint8_t byte_val = (colour & 3) * 0x55;   /* expand 2b → 8b */
    byte_val = (byte_val << 6) | (byte_val << 4) |
               (byte_val << 2) | byte_val;
    memset(buf, byte_val, EPD_DATA_SIZE);
}

void epd_set_pixel(uint8_t *buf, uint16_t x, uint16_t y, uint8_t colour)
{
    if (!buf || x >= EPD_WIDTH || y >= EPD_HEIGHT) return;
    uint32_t idx = (uint32_t)y * (EPD_WIDTH / 4UL) + (x / 4UL);
    uint8_t  shift = (3 - (x & 3)) * 2;   /* bits: px3..px0 = 6,4,2,0 */
    buf[idx] = (buf[idx] & ~(3 << shift)) | ((colour & 3) << shift);
}

void epd_draw_hline(uint8_t *buf, uint16_t x1, uint16_t x2,
                    uint16_t y, uint8_t colour)
{
    if (!buf || y >= EPD_HEIGHT) return;
    if (x1 > x2) { uint16_t t = x1; x1 = x2; x2 = t; }
    if (x2 >= EPD_WIDTH) x2 = EPD_WIDTH - 1;
    for (uint16_t x = x1; x <= x2; x++) {
        epd_set_pixel(buf, x, y, colour);
    }
}

void epd_draw_vline(uint8_t *buf, uint16_t x, uint16_t y1,
                    uint16_t y2, uint8_t colour)
{
    if (!buf || x >= EPD_WIDTH) return;
    if (y1 > y2) { uint16_t t = y1; y1 = y2; y2 = t; }
    if (y2 >= EPD_HEIGHT) y2 = EPD_HEIGHT - 1;
    for (uint16_t y = y1; y <= y2; y++) {
        epd_set_pixel(buf, x, y, colour);
    }
}

void epd_fill_rect(uint8_t *buf, uint16_t x, uint16_t y,
                   uint16_t w, uint16_t h, uint8_t colour)
{
    if (!buf) return;
    for (uint16_t row = y; row < y + h && row < EPD_HEIGHT; row++) {
        epd_draw_hline(buf, x, x + w - 1, row, colour);
    }
}

/* ========================================================================= */
/*  Sensor overlay panel (bottom-right corner)                                */
/* ========================================================================= */

/* Panel geometry */
#define OVERLAY_X       (EPD_WIDTH  - 272)   /* left edge, 272px from right */
#define OVERLAY_Y       (EPD_HEIGHT - 168)   /* top edge,  168px from bottom */
#define OVERLAY_W       264                   /* panel width (multiple of 8) */
#define OVERLAY_H       160                   /* panel height (multiple of 8) */
#define OVERLAY_MARGIN  4
#define OVERLAY_PAD_X   4
#define OVERLAY_PAD_Y   4

/* Forward declarations for compact-buffer helpers used below */

/**
 * Set pixel in a compact window buffer where row stride = win_w_px / 4.
 */
static void epd_set_pixel_win(uint8_t *buf, uint16_t x, uint16_t y,
                              uint8_t colour, uint16_t win_w)
{
    uint32_t idx = (uint32_t)y * (win_w / 4UL) + (x / 4UL);
    uint8_t shift = (3 - (x & 3)) * 2;
    buf[idx] = (uint8_t)((buf[idx] & ~(3 << shift)) | (colour << shift));
}

/**
 * Fill a rectangle in a compact window buffer.
 */
static void epd_fill_rect_win(uint8_t *buf, uint16_t x, uint16_t y,
                              uint16_t w, uint16_t h,
                              uint8_t colour, uint16_t win_w)
{
    uint32_t row_bytes = win_w / 4UL;
    for (uint16_t row = y; row < y + h; row++) {
        uint8_t *row_buf = buf + (uint32_t)row * row_bytes;
        for (uint16_t col = x; col < x + w; col++) {
            uint8_t shift = (3 - (col & 3)) * 2;
            uint32_t bi = col / 4UL;
            row_buf[bi] = (uint8_t)((row_buf[bi] & ~(3 << shift)) | (colour << shift));
        }
    }
}

/**
 * Draw a horizontal line in a compact window buffer.
 */
static void epd_draw_hline_win(uint8_t *buf, uint16_t x1, uint16_t x2,
                               uint16_t y, uint8_t colour, uint16_t win_w)
{
    for (uint16_t x = x1; x <= x2; x++) {
        epd_set_pixel_win(buf, x, y, colour, win_w);
    }
}

/**
 * Draw a vertical line in a compact window buffer.
 */
static void epd_draw_vline_win(uint8_t *buf, uint16_t x, uint16_t y1,
                               uint16_t y2, uint8_t colour, uint16_t win_w)
{
    for (uint16_t y = y1; y <= y2; y++) {
        epd_set_pixel_win(buf, x, y, colour, win_w);
    }
}

/**
 * Draw an 8×16 ASCII character in a compact window buffer.
 */
static void epd_draw_char_win(uint8_t *buf, uint16_t x, uint16_t y,
                              unsigned char ch, uint8_t colour, uint16_t win_w)
{
    if (ch < 32 || ch > 126) return;
    const uint8_t *glyph = epd_font_data + (ch - 32) * EPD_FONT_H;
    for (uint8_t row = 0; row < EPD_FONT_H; row++) {
        uint8_t bits = glyph[row];
        for (uint8_t col = 0; col < 8; col++) {
            if (bits & (0x80 >> col)) {
                epd_set_pixel_win(buf, x + col, y + row, colour, win_w);
            }
        }
    }
}

/**
 * Draw a string in a compact window buffer.
 */
static void epd_draw_string_win(uint8_t *buf, uint16_t x, uint16_t y,
                                const char *str, uint8_t colour, uint16_t win_w)
{
    while (*str) {
        epd_draw_char_win(buf, x, y, (unsigned char)*str, colour, win_w);
        x += 8;
        str++;
    }
}

/**
 * @brief  Render sensor overlay into a compact window buffer
 *         and perform a partial update.
 *
 * The buffer must be sized for a (OVERLAY_W × OVERLAY_H) window
 * at 2-bit packed format: (OVERLAY_W * OVERLAY_H) / 4 bytes.
 *
 * @param  window_buf  Compact overlay window buffer (~10 KB).
 * @param  data        Sensor readings.
 * @return ESP_OK on success.
 */
esp_err_t epd_sensor_overlay_window(uint8_t *window_buf,
                                    const epd_sensor_data_t *data)
{
    if (!window_buf || !data) return ESP_ERR_INVALID_ARG;

    /* Fill panel white */
    epd_fill_rect_win(window_buf, 0, 0, OVERLAY_W, OVERLAY_H,
                      EPD_COLOR_WHITE, OVERLAY_W);

    /* Thin black border */
    epd_draw_hline_win(window_buf, 0, OVERLAY_W - 1, 0,
                       EPD_COLOR_BLACK, OVERLAY_W);
    epd_draw_hline_win(window_buf, 0, OVERLAY_W - 1, OVERLAY_H - 1,
                       EPD_COLOR_BLACK, OVERLAY_W);
    epd_draw_vline_win(window_buf, 0, 0, OVERLAY_H - 1,
                       EPD_COLOR_BLACK, OVERLAY_W);
    epd_draw_vline_win(window_buf, OVERLAY_W - 1, 0, OVERLAY_H - 1,
                       EPD_COLOR_BLACK, OVERLAY_W);

    /* Render text (relative to window origin) */
    int cx = OVERLAY_PAD_X;
    int cy = OVERLAY_PAD_Y;
    char line[64];

    snprintf(line, sizeof(line), "ACC: %5.2f %5.2f %5.2f g",
             (double)data->acc_x, (double)data->acc_y, (double)data->acc_z);
    epd_draw_string_win(window_buf, cx, cy, line, EPD_COLOR_BLACK, OVERLAY_W);
    cy += EPD_FONT_H + 2;

    snprintf(line, sizeof(line), "GYR: %5.1f %5.1f %5.1f %s",
             (double)data->gyr_x, (double)data->gyr_y, (double)data->gyr_z, "dps");
    epd_draw_string_win(window_buf, cx, cy, line, EPD_COLOR_BLACK, OVERLAY_W);
    cy += EPD_FONT_H + 2;

    snprintf(line, sizeof(line), "MAG: %5.1f %5.1f %5.1f uT",
             (double)data->mag_x, (double)data->mag_y, (double)data->mag_z);
    epd_draw_string_win(window_buf, cx, cy, line, EPD_COLOR_BLACK, OVERLAY_W);
    cy += EPD_FONT_H + 2;

    if (data->gps_fix) {
        snprintf(line, sizeof(line), "GPS: %.6f,%.6f",
                 (double)data->lat, (double)data->lon);
    } else {
        snprintf(line, sizeof(line), "GPS: no fix");
    }
    epd_draw_string_win(window_buf, cx, cy, line, EPD_COLOR_BLACK, OVERLAY_W);

    /* Partial update */
    esp_err_t err = epd_update_partial_window(window_buf,
                                              OVERLAY_X, OVERLAY_Y,
                                              OVERLAY_W, OVERLAY_H);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "sensor_overlay_window partial update failed: %s",
                 esp_err_to_name(err));
    }
    return err;
}

/*
 * Keep the original epd_sensor_overlay() for backward compatibility
 * (used when a full 96000-byte frame buffer is available).
 */
esp_err_t epd_sensor_overlay(uint8_t *buf, const epd_sensor_data_t *data)
{
    return epd_sensor_overlay_window(buf, data);
}

/* ========================================================================= */
/*  BLE image → EPD display                                                 */
/* ========================================================================= */

/* Forward-declare W25Q64 read (from W25Q64.h) */
extern esp_err_t w25q64_read(void *handle, uint32_t addr,
                             uint8_t *data, uint32_t len);
/* The W25Q64 handle — must be set before calling epd_display_from_flash() */
void *g_epd_flash_handle = NULL;

void epd_set_flash_handle(void *flash_handle)
{
    g_epd_flash_handle = flash_handle;
}

esp_err_t epd_display_from_flash(uint8_t *buf, uint32_t flash_offset)
{
    if (!buf || !g_epd_flash_handle) return ESP_ERR_INVALID_STATE;

    /* Read image from Flash (IMG_CACHE region) into the frame buffer */
    esp_err_t err = w25q64_read(g_epd_flash_handle, flash_offset,
                                buf, EPD_DATA_SIZE);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Flash read: %s", esp_err_to_name(err));
        return err;
    }

    /* Full-screen refresh */
    return epd_update_full(buf);
}
