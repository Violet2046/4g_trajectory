#pragma once

#include "driver/spi_master.h"
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*  config.h  --  所有可配置参数单一定义点                                      */
/* ========================================================================= */

/* ======================================================================== */
/*  Hardware Pins                                                            */
/* ======================================================================== */

/* ---- CT511N (4G + GPS) UART ---- */
#define CFG_CT_UART_PORT        UART_NUM_1
#define CFG_CT_UART_TX_PIN      5
#define CFG_CT_UART_RX_PIN      6
#define CFG_CT_UART_BAUD        115200
#define CFG_CT_DTR_PIN          10

/* ---- I2C bus (shared: BMI160 + AK09911C) ---- */
#define CFG_I2C_SDA             7
#define CFG_I2C_SCL             8

/* ---- BMI160 IMU interrupt pins ---- */
#define CFG_BMI160_INT1_PIN     9       /* any-motion → light-sleep wakeup */
#define CFG_BMI160_INT2_PIN     0       /* double-tap → IMG_RECEIVE       */

/* ---- SPI bus (shared: W25Q64 + ST7789) ---- */
#define CFG_SPI_HOST            SPI2_HOST
#define CFG_SPI_SCK_GPIO        3
#define CFG_SPI_MOSI_GPIO       2
#define CFG_SPI_MISO_GPIO       4

/* ---- W25Q64 NOR Flash ---- */
#define CFG_W25Q64_CS_GPIO      1

/* ---- ST7789 LCD (240×240 RGB565) ---- */
#define CFG_ST7789_CS_GPIO      11
#define CFG_ST7789_DC_GPIO      12
#define CFG_ST7789_RST_GPIO     13
#define CFG_ST7789_BLK_GPIO     14

/* ---- SPI Bus Freq ---- */
#define CFG_W25Q64_SPI_FREQ_HZ   (10 * 1000 * 1000)  /* 10 MHz */
#define CFG_ST7789_SPI_FREQ_HZ   (40 * 1000 * 1000)  /* 40 MHz */

/* ======================================================================== */
/*  Timing                                                                   */
/* ======================================================================== */

#define CFG_GPTIMER_RESOLUTION_HZ   1000000
#define CFG_SAMPLE_INTERVAL_S       1       /* sensor sampling period, s  */
#define CFG_CT_WAKE_DELAY_MS        500     /* boot delay for CT511N       */
#define CFG_INACTIVITY_TIMEOUT_MS   5000    /* no-motion → LOW_POWER       */
#define CFG_IDLE_POLL_MS            50      /* ACTIVE loop poll interval   */
#define CFG_LOWPOWER_POLL_MS        1000    /* LOW_POWER semaphore timeout */

/* ---- W25Q64 SPI ---- */
#define CFG_W25Q64_SPI_FREQ_HZ      10000000  /* 10 MHz, breadboard-safe  */

/* ======================================================================== */
/*  BMI160 Motion                                                            */
/* ======================================================================== */

#define CFG_ANYMOTION_THRESHOLD     0x06    /* any-motion threshold        */
#define CFG_ANYMOTION_DURATION      0x01    /* any-motion duration (samples)*/
#define CFG_MOTION_DEBOUNCE_MS      2000    /* 2nd hit ≥ N ms after 1st   */

/* ======================================================================== */
/*  WiFi AP                                                                  */
/* ======================================================================== */

#define CFG_WIFI_AP_SSID            "4G-Tracker"
#define CFG_WIFI_AP_PASSWORD        "12345678"
#define CFG_WIFI_AP_MAX_CONN        4

/* ======================================================================== */
/*  Server / Network                                                         */
/* ======================================================================== */

#define CFG_SERVER_IP_DEFAULT       "tcp.doiot.cn"
#define CFG_SERVER_PORT_DEFAULT     22962

/* connectivity test target */
#define CFG_PING_TARGET             "baidu.com"
#define CFG_PING_PORT               80
#define CFG_WIFI_STA_CONNECT_TIMEOUT_MS  15000

/* string form for storage_config_t.server_port */
#define STR_HELPER(x) #x
#define STR(x)        STR_HELPER(x)
#define CFG_SERVER_PORT_STR  STR(CFG_SERVER_PORT_DEFAULT)


#ifdef __cplusplus
}
#endif
