#pragma once

#include "driver/spi_master.h"
#include "driver/uart.h"
#include "bmi160.h"

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
#define CFG_CT_DTR_PIN          11   /* cannot share with CFG_EPD_CS_GPIO (GPIO 19) */

/* ---- I2C bus (shared: BMI160 + AK09911C) ---- */
#define CFG_I2C_SDA             7
#define CFG_I2C_SCL             8   /* cannot use GPIO 8 — that's ESP32-C3 strapping pin */

/* ---- BMI160 IMU interrupt pins ---- */
#define CFG_BMI160_INT1_PIN     9      /* any-motion → light-sleep wakeup */
#define CFG_BMI160_INT2_PIN     22     /* double-tap → IMG_RECEIVE       */

/* ---- SPI bus (shared: W25Q64 + EPD) ---- */
#define CFG_SPI_HOST            SPI2_HOST
#define CFG_SPI_SCK_GPIO        3
#define CFG_SPI_MOSI_GPIO       2
#define CFG_SPI_MISO_GPIO       4

/* ---- W25Q64 NOR Flash ---- */
#define CFG_W25Q64_CS_GPIO      1

/* ---- QYEG0397 EPD (800×480 4-color) ---- */
#define CFG_EPD_CS_GPIO         10
#define CFG_EPD_DC_GPIO         20
#define CFG_EPD_RST_GPIO        21
#define CFG_EPD_BUSY_GPIO       0
#define CFG_EPD_SPI_FREQ_HZ     (4 * 1000 * 1000)   /* 4 MHz */

/* ---- SPI Bus Freq ---- */
#define CFG_W25Q64_SPI_FREQ_HZ   (1 * 1000 * 1000)  /* 2 MHz, breadboard-safe */

/* ======================================================================== */
/*  Timing                                                                   */
/* ======================================================================== */

#define CFG_GPTIMER_RESOLUTION_HZ   1000000
#define CFG_SAMPLE_INTERVAL_S       3       /* sensor sampling period, s  */
#define CFG_CT_WAKE_DELAY_MS        500     /* boot delay for CT511N       */
#define CFG_INACTIVITY_TIMEOUT_MS   100000    /* no-motion → LOW_POWER       */
#define CFG_IDLE_POLL_MS            50      /* ACTIVE loop poll interval   */
#define CFG_LOWPOWER_POLL_MS        1000    /* LOW_POWER semaphore timeout */

/* ======================================================================== */
/*  BMI160 Motion                                                            */
/* ======================================================================== */

#define CFG_ANYMOTION_THRESHOLD     BMI160_ACC_ODR_25HZ    /* any-motion threshold        */
#define CFG_ANYMOTION_DURATION      BMI160_INT_ANY_MOTION_X_EN    /* any-motion duration (samples)*/
#define CFG_MOTION_DEBOUNCE_MS      2000    /* 2nd hit ≥ N ms after 1st   */

/* ======================================================================== */
/*  WiFi AP                                                                  */
/* ======================================================================== */

#define CFG_WIFI_AP_SSID            "4G-Tracker"
#define CFG_WIFI_AP_PASSWORD        "12345678"
#define CFG_WIFI_AP_MAX_CONN        4

/* ---- Boot STA / AP / periodic scan timing (all in microseconds) ---- */
#define CFG_AP_NO_STA_TIMEOUT_US        20000000LL    /* 20s — AP close if no sta   */
#define CFG_AP_TOTAL_TIMEOUT_US         60000000LL    /* 60s — AP max total life    */
#define CFG_PERIODIC_SCAN_INTERVAL_US   300000000LL   /* 5min — WiFi STA scan cycle */
#define CFG_WIFI_UPLOAD_INTERVAL_US     300000000LL   /* 5min — WiFi batch upload   */

/* ---- BLE advertising interval (units: 0.625 ms) ---- */
#define CFG_BLE_ADV_ITVL_MIN            150          /*  5 × 0.625 ms =    3.125 ms */
#define CFG_BLE_ADV_ITVL_MAX            200         /* 10 × 0.625 ms =    6.25  ms */

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
