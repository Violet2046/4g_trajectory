#pragma once

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*  Pin Configuration — single source of truth                               */
/*                                                                           */
/*  All hardware pin assignments, bus IDs and timer settings are defined     */
/*  here.  BMI160_INT1_PIN == LP_WAKEUP_GPIO — they share the same           */
/*  physical GPIO so that any-motion can wake the CPU from Light-sleep.      */
/* ========================================================================= */

/* ---- CT511N (4G + GPS) UART ---- */
#define CT_UART_PORT            UART_NUM_1
#define CT_UART_TX_PIN          5
#define CT_UART_RX_PIN          6
#define CT_UART_BAUD            115200
#define CT_DTR_PIN              10

/* ---- I2C bus (shared: BMI160 + AK09911C) ---- */
#define I2C_SDA                 7
#define I2C_SCL                 8

/* ---- BMI160 IMU interrupt pins ---- */
/* INT1: any-motion detection — also serves as Light-sleep wakeup source */
#define BMI160_INT1_PIN         9
/* INT2: double-tap detection — triggers IMG_RECEIVE mode */
#define BMI160_INT2_PIN         0

/* Light-sleep wakeup GPIO — same physical pin as BMI160 INT1 */
#define LP_WAKEUP_GPIO          BMI160_INT1_PIN

/* ---- SPI bus (shared: W25Q64 NOR Flash + ST7789 LCD) ---- */
#define SPI_HOST                SPI2_HOST
#define SPI_SCK_GPIO            3
#define SPI_MOSI_GPIO           2
#define SPI_MISO_GPIO           4

/* ---- W25Q64 NOR Flash ---- */
#define W25Q64_CS_GPIO          1

/* ---- ST7789 LCD display (240×240 RGB565) ---- */
#define ST7789_CS_GPIO          11
#define ST7789_DC_GPIO          12
#define ST7789_RST_GPIO         13
#define ST7789_BLK_GPIO         14

/* ---- Timer resolution ---- */
#define GPTIMER_RESOLUTION_HZ   1000000

#ifdef __cplusplus
}
#endif
