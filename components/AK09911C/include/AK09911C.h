#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/*  Register Map                                                             */
/* ------------------------------------------------------------------------- */
#define AK09911_WIA1            0x00  /* Company ID (fixed 0x48)             */
#define AK09911_WIA2            0x01  /* Device ID  (fixed 0x05)             */
#define AK09911_INFO1           0x02
#define AK09911_INFO2           0x03
#define AK09911_ST1             0x10  /* Status 1: DOR(1) DRDY(0)           */
#define AK09911_HXL             0x11  /* X-axis measurement data LSB         */
#define AK09911_HXH             0x12
#define AK09911_HYL             0x13
#define AK09911_HYH             0x14
#define AK09911_HZL             0x15
#define AK09911_HZH             0x16
#define AK09911_TMPS            0x17  /* Dummy register (read to advance)    */
#define AK09911_ST2             0x18  /* Status 2: HOFL(3)                  */
#define AK09911_CNTL1           0x30  /* Dummy register                     */
#define AK09911_CNTL2           0x31  /* Operation mode                      */
#define AK09911_CNTL3           0x32  /* Soft reset                          */
#define AK09911_TS1             0x33  /* Test register — do not use          */
#define AK09911_ASAX            0x60  /* X-axis sensitivity adjustment       */
#define AK09911_ASAY            0x61
#define AK09911_ASAZ            0x62

/* ---- Fuse ROM data length ---- */
#define AK09911_FUSE_LEN        3

/* ------------------------------------------------------------------------- */
/*  Device IDs                                                                */
/* ------------------------------------------------------------------------- */
#define AK09911_WIA1_VAL        0x48
#define AK09911_WIA2_VAL        0x05

/* ------------------------------------------------------------------------- */
/*  ST1 bits                                                                  */
/* ------------------------------------------------------------------------- */
#define AK09911_ST1_DRDY        0x01  /* Data Ready                          */
#define AK09911_ST1_DOR         0x02  /* Data Overrun                        */

/* ------------------------------------------------------------------------- */
/*  ST2 bits                                                                  */
/* ------------------------------------------------------------------------- */
#define AK09911_ST2_HOFL        0x08  /* Magnetic sensor overflow            */

/* ------------------------------------------------------------------------- */
/*  Operation Modes (CNTL2 MODE[4:0])                                        */
/* ------------------------------------------------------------------------- */
#define AK09911_MODE_POWERDOWN  0x00
#define AK09911_MODE_SINGLE     0x01
#define AK09911_MODE_CONT_1     0x02  /* 10 Hz                               */
#define AK09911_MODE_CONT_2     0x04  /* 20 Hz                               */
#define AK09911_MODE_CONT_3     0x06  /* 50 Hz                               */
#define AK09911_MODE_CONT_4     0x08  /* 100 Hz                              */
#define AK09911_MODE_SELFTEST   0x10
#define AK09911_MODE_FUSE_ROM   0x1F

/* ------------------------------------------------------------------------- */
/*  CNTL3 bits                                                                */
/* ------------------------------------------------------------------------- */
#define AK09911_CNTL3_SRST      0x01  /* Soft reset                          */

/* ------------------------------------------------------------------------- */
/*  Sensitivity constants                                                     */
/* ------------------------------------------------------------------------- */
#define AK09911_SENSITIVITY_UT_PER_LSB  0.6f   /* 0.6 µT/LSB                 */
#define AK09911_ASA_FACTOR_DEN          128    /* ASA/128 + 1                 */

/* ------------------------------------------------------------------------- */
/*  Data structures                                                           */
/* ------------------------------------------------------------------------- */
typedef struct ak09911_handle_t ak09911_handle_t;

/** Raw 16-bit magnetometer axes (two's complement, Little Endian) */
typedef struct {
	int16_t x;
	int16_t y;
	int16_t z;
} ak09911_axes_raw_t;

/** Magnetometer axes in microtesla (µT) */
typedef struct {
	float x;
	float y;
	float z;
} ak09911_axes_ut_t;

/** I2C configuration passed to ak09911_create() */
typedef struct {
	i2c_master_bus_handle_t  bus_handle;  /* Existing bus handle, or NULL to create new */
	i2c_master_bus_config_t  bus_cfg;    /* I2C bus config (used only if bus_handle==NULL) */
	uint16_t                 dev_addr;   /* 7-bit address: 0x0C (CAD=0) or 0x0D (CAD=1) */
	uint32_t                 i2c_freq_hz; /* SCL frequency, Hz               */
} ak09911_i2c_config_t;

/* ------------------------------------------------------------------------- */
/*  Create / Destroy / Init                                                   */
/* ------------------------------------------------------------------------- */
esp_err_t ak09911_create(ak09911_handle_t **out_handle,
			 const ak09911_i2c_config_t *config);
void      ak09911_destroy(ak09911_handle_t *handle);

/**
 * @brief  Create handle, verify chip, and apply default setup.
 *
 *   - Verifies WIA1/WIA2 device ID
 *   - Reads and stores factory sensitivity adjustment coefficients (ASAX/Y/Z)
 *   - Leaves device in Power-down mode; caller must set an operation mode
 */
esp_err_t ak09911_init(ak09911_handle_t **out_handle,
		       const ak09911_i2c_config_t *config);

/* ------------------------------------------------------------------------- */
/*  Mode / Reset                                                              */
/* ------------------------------------------------------------------------- */
esp_err_t ak09911_mode_set(ak09911_handle_t *handle, uint8_t mode);
esp_err_t ak09911_soft_reset(ak09911_handle_t *handle);

/* ------------------------------------------------------------------------- */
/*  Data Reading                                                              */
/* ------------------------------------------------------------------------- */
/**
 * @brief  Read raw magnetic data (14-bit stored in 16-bit, two's complement).
 *
 * Blocks until DRDY is set, then reads HXL–HZH + TMPS + ST2.
 * Returns the raw axes and populates @p overflow (HOFL flag).
 */
esp_err_t ak09911_data_read_raw(ak09911_handle_t *handle,
				ak09911_axes_raw_t *out,
				bool *overflow);

/**
 * @brief  Read magnetic data converted to microtesla (µT).
 *
 * Applies factory sensitivity adjustment coefficients.
 */
esp_err_t ak09911_data_read_ut(ak09911_handle_t *handle,
			       ak09911_axes_ut_t *out,
			       bool *overflow);

/**
 * @brief  Poll DRDY bit — non-blocking check.
 * @param  ready  [out] true if new data is available.
 */
esp_err_t ak09911_data_ready(ak09911_handle_t *handle, bool *ready);

/* ------------------------------------------------------------------------- */
/*  Sensitivity                                                               */
/* ------------------------------------------------------------------------- */
/**
 * @brief  Copy the stored sensitivity coefficients.
 * @param  asax/y/z  [out] raw coefficients from Fuse ROM (0–255).
 */
esp_err_t ak09911_sensitivity_get(ak09911_handle_t *handle,
				  uint8_t *asax,
				  uint8_t *asay,
				  uint8_t *asaz);

#ifdef __cplusplus
}
#endif
