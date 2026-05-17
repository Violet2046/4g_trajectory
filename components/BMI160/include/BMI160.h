#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "hal/gpio_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------------- */
/*  Register Map                                                             */
/* ------------------------------------------------------------------------- */
#define BMI160_CHIP_ID          0x00
#define BMI160_ERR_REG          0x02
#define BMI160_PMU_STATUS       0x03
#define BMI160_DATA_0           0x04  /* MAG_X[7:0] */
#define BMI160_DATA_8           0x0C  /* GYR_X[7:0] */
#define BMI160_DATA_14          0x12  /* ACC_X[7:0] */
#define BMI160_SENSORTIME_0     0x18
#define BMI160_STATUS           0x1B
#define BMI160_INT_STATUS_0     0x1C
#define BMI160_INT_STATUS_1     0x1D
#define BMI160_INT_STATUS_2     0x1E
#define BMI160_INT_STATUS_3     0x1F
#define BMI160_TEMPERATURE_0    0x20
#define BMI160_FIFO_LENGTH_0    0x22
#define BMI160_FIFO_DATA        0x24
#define BMI160_ACC_CONF         0x40
#define BMI160_ACC_RANGE        0x41
#define BMI160_GYR_CONF         0x42
#define BMI160_GYR_RANGE        0x43
#define BMI160_INT_EN_0         0x50
#define BMI160_INT_EN_1         0x51
#define BMI160_INT_EN_2         0x52
#define BMI160_INT_OUT_CTRL     0x53
#define BMI160_INT_LATCH        0x54
#define BMI160_INT_MAP_0        0x55
#define BMI160_INT_MAP_1        0x56
#define BMI160_INT_MAP_2        0x57
#define BMI160_INT_DATA_0       0x58
#define BMI160_INT_DATA_1       0x59
#define BMI160_INT_LOWHIGH_0    0x5A
#define BMI160_INT_LOWHIGH_1    0x5B
#define BMI160_INT_LOWHIGH_2    0x5C
#define BMI160_INT_LOWHIGH_3    0x5D
#define BMI160_INT_LOWHIGH_4    0x5E
#define BMI160_INT_MOTION_0     0x5F
#define BMI160_INT_MOTION_1     0x60
#define BMI160_INT_MOTION_2     0x61
#define BMI160_INT_MOTION_3     0x62
#define BMI160_INT_TAP_0        0x6B
#define BMI160_INT_TAP_1        0x6C
#define BMI160_INT_TAP_2        0x6D
#define BMI160_INT_TAP_3        0x6E
#define BMI160_OFFSET_0         0x71
#define BMI160_OFFSET_6         0x77
#define BMI160_STEP_CNT_0       0x78
#define BMI160_STEP_CNT_1       0x79
#define BMI160_STEP_CONF_0      0x7A
#define BMI160_STEP_CONF_1      0x7B
#define BMI160_CMD              0x7E

/* ------------------------------------------------------------------------- */
/*  PMU Command Values (written to CMD register)                             */
/* ------------------------------------------------------------------------- */
#define BMI160_CMD_ACC_PMU_SUSPEND      0x10
#define BMI160_CMD_ACC_PMU_NORMAL       0x11
#define BMI160_CMD_ACC_PMU_LOW_POWER    0x12
#define BMI160_CMD_ACC_PMU_FAST_STARTUP 0x13
#define BMI160_CMD_GYR_PMU_SUSPEND      0x14
#define BMI160_CMD_GYR_PMU_NORMAL       0x15
#define BMI160_CMD_GYR_PMU_FAST_STARTUP 0x17
#define BMI160_CMD_SOFT_RESET           0xB6

/* ------------------------------------------------------------------------- */
/*  PMU Status Masks (PMU_STATUS register)                                   */
/* ------------------------------------------------------------------------- */
#define BMI160_ACC_PMU_STATUS_MASK  0x30
#define BMI160_GYR_PMU_STATUS_MASK  0x0C
#define BMI160_ACC_PMU_SUSPEND      0x00
#define BMI160_ACC_PMU_NORMAL       0x10
#define BMI160_ACC_PMU_LOW_POWER    0x20
#define BMI160_ACC_PMU_FAST_STARTUP 0x30
#define BMI160_GYR_PMU_SUSPEND      0x00
#define BMI160_GYR_PMU_NORMAL       0x04
#define BMI160_GYR_PMU_FAST_STARTUP 0x0C

/* ------------------------------------------------------------------------- */
/*  Accelerometer Config                                                      */
/* ------------------------------------------------------------------------- */
#define BMI160_ACC_ODR_0_78HZ   0x01
#define BMI160_ACC_ODR_1_56HZ   0x02
#define BMI160_ACC_ODR_3_12HZ   0x03
#define BMI160_ACC_ODR_6_25HZ   0x04
#define BMI160_ACC_ODR_12_5HZ   0x05
#define BMI160_ACC_ODR_25HZ     0x06
#define BMI160_ACC_ODR_50HZ     0x07
#define BMI160_ACC_ODR_100HZ    0x08
#define BMI160_ACC_ODR_200HZ    0x09
#define BMI160_ACC_ODR_400HZ    0x0A
#define BMI160_ACC_ODR_800HZ    0x0B
#define BMI160_ACC_ODR_1600HZ   0x0C

#define BMI160_ACC_BWP_NORMAL   0x02
#define BMI160_ACC_BWP_OSR2     0x01
#define BMI160_ACC_BWP_OSR4     0x00

#define BMI160_ACC_US_OFF       0x00
#define BMI160_ACC_US_ON        0x80

#define BMI160_ACC_RANGE_2G     0x03
#define BMI160_ACC_RANGE_4G     0x05
#define BMI160_ACC_RANGE_8G     0x08
#define BMI160_ACC_RANGE_16G    0x0C

/* ------------------------------------------------------------------------- */
/*  Gyroscope Config                                                          */
/* ------------------------------------------------------------------------- */
#define BMI160_GYR_ODR_25HZ     0x06
#define BMI160_GYR_ODR_50HZ     0x07
#define BMI160_GYR_ODR_100HZ    0x08
#define BMI160_GYR_ODR_200HZ    0x09
#define BMI160_GYR_ODR_400HZ    0x0A
#define BMI160_GYR_ODR_800HZ    0x0B
#define BMI160_GYR_ODR_1600HZ   0x0C
#define BMI160_GYR_ODR_3200HZ   0x0D

#define BMI160_GYR_BWP_NORMAL   0x02
#define BMI160_GYR_BWP_OSR2     0x01
#define BMI160_GYR_BWP_OSR4     0x00

#define BMI160_GYR_RANGE_125    0x04
#define BMI160_GYR_RANGE_250    0x03
#define BMI160_GYR_RANGE_500    0x02
#define BMI160_GYR_RANGE_1000   0x01
#define BMI160_GYR_RANGE_2000   0x00

/* ------------------------------------------------------------------------- */
/*  Interrupt Enable / Map masks                                              */
/* ------------------------------------------------------------------------- */
#define BMI160_INT_ANY_MOTION_X_EN  0x01
#define BMI160_INT_ANY_MOTION_Y_EN  0x02
#define BMI160_INT_ANY_MOTION_Z_EN  0x04
#define BMI160_INT_SIG_MOTION_EN    0x07  /* sig-motion is enabled via any-motion x|y|z in INT_EN_0 */
#define BMI160_INT_STEP_DETECT_EN   0x10
#define BMI160_INT_TAP_EN           0x20
#define BMI160_INT_DOUBLE_TAP_EN    0x40
#define BMI160_INT_DRDY_EN          0x10

/* ------------------------------------------------------------------------- */
/*  Int Motion configuration (INT_MOTION_0..3)                                */
/* ------------------------------------------------------------------------- */
#define BMI160_ANYM_DUR_DEFAULT      0x02  /* 2 samples */
#define BMI160_ANYM_THR_DEFAULT      0x14  /* ~0.3g per LSB=15.6mg → 312mg */
#define BMI160_SIGM_DUR_DEFAULT      0x04
#define BMI160_SIGM_THR_DEFAULT      0x28  /* ~0.6g */

/* ------------------------------------------------------------------------- */
/*  Tap configuration (INT_TAP_0..3)                                          */
/* ------------------------------------------------------------------------- */
#define BMI160_TAP_DUR_DEFAULT       0x04
#define BMI160_TAP_THR_DEFAULT       0x0A
#define BMI160_TAP_SHOCK_DEFAULT     0x00
#define BMI160_TAP_QUIET_DEFAULT     0x00

/* ------------------------------------------------------------------------- */
/*  Step counter                                                              */
/* ------------------------------------------------------------------------- */
#define BMI160_STEP_CNT_EN           0x08
#define BMI160_STEP_SENS_NORMAL      0x00
#define BMI160_STEP_SENS_SENSITIVE   0x01
#define BMI160_STEP_SENS_ROBUST      0x02

/* ------------------------------------------------------------------------- */
/*  Data structures                                                           */
/* ------------------------------------------------------------------------- */
typedef struct bmi160_handle_t bmi160_handle_t;

typedef struct {
	int16_t x;
	int16_t y;
	int16_t z;
} bmi160_axes_raw_t;

typedef struct {
	float x;
	float y;
	float z;
} bmi160_axes_g_t;

typedef struct {
	float x;
	float y;
	float z;
} bmi160_gyro_dps_t;

typedef struct {
	i2c_master_bus_handle_t  bus_handle;  /* Existing bus handle, or NULL to create new */
	i2c_master_bus_config_t  bus_cfg;   /* I2C bus config (used only if bus_handle==NULL) */
	uint16_t                 dev_addr;  /* 7-bit device address (default 0x68) */
	uint32_t                 i2c_freq_hz; /* SCL frequency, Hz */
	gpio_num_t               int1_pin;  /* INT1 GPIO, GPIO_NUM_NC to disable */
	gpio_int_type_t          int1_type; /* INT1 trigger type (e.g., GPIO_INTR_POSEDGE) */
	gpio_num_t               int2_pin;  /* INT2 GPIO, GPIO_NUM_NC to disable */
	gpio_int_type_t          int2_type; /* INT2 trigger type */
} bmi160_i2c_config_t;

typedef enum {
	BMI160_MODE_SUSPEND = 0,
	BMI160_MODE_NORMAL,
	BMI160_MODE_LOW_POWER,
	BMI160_MODE_FAST_STARTUP,
} bmi160_mode_t;

typedef enum {
	BMI160_ACC_RANGE_2G_VAL  = 2,
	BMI160_ACC_RANGE_4G_VAL  = 4,
	BMI160_ACC_RANGE_8G_VAL  = 8,
	BMI160_ACC_RANGE_16G_VAL = 16,
} bmi160_acc_range_val_t;

typedef enum {
	BMI160_GYR_RANGE_125_VAL  = 125,
	BMI160_GYR_RANGE_250_VAL  = 250,
	BMI160_GYR_RANGE_500_VAL  = 500,
	BMI160_GYR_RANGE_1000_VAL = 1000,
	BMI160_GYR_RANGE_2000_VAL = 2000,
} bmi160_gyr_range_val_t;

/* ------------------------------------------------------------------------- */
/*  Public API                                                                */
/* ------------------------------------------------------------------------- */

/**
 * @brief  Create a BMI160 handle and initialise the hardware.
 * @param  out_handle  [out] pointer to receive the handle.
 * @param  config      I2C bus / address configuration.
 * @return ESP_OK on success.
 */
esp_err_t bmi160_create(bmi160_handle_t **out_handle,
			const bmi160_i2c_config_t *config);

/**
 * @brief  Create and fully configure a BMI160 device for motion tracking.
 *
 * Convenience function that creates the device and applies the project's
 * standard sensor configuration in one call:
 *   - Accelerometer: ±8 g, 100 Hz ODR, normal bandwidth
 *   - Gyroscope:     ±500 dps, 100 Hz ODR, normal bandwidth
 *   - Significant-motion interrupt enabled, mapped to INT1 (INT_MAP[0] = 0x04)
 *   - INT1 output: active-high, push-pull
 *   - Both sensors powered to Normal mode
 *
 * After calling this function the caller only needs to register the ISR
 * callback with @ref bmi160_int1_isr_add.
 *
 * @param  out_handle  [out] pointer to receive the handle.
 * @param  config      I2C bus / address / interrupt pin configuration.
 * @return ESP_OK on success, or an error code (handle destroyed on failure).
 */
esp_err_t bmi160_init(bmi160_handle_t **out_handle,
		      const bmi160_i2c_config_t *config);

/**
 * @brief  Destroy handle and release resources.
 */
void bmi160_destroy(bmi160_handle_t *handle);

/**
 * @brief  Soft-reset the device and wait for it to come back.
 */
esp_err_t bmi160_soft_reset(bmi160_handle_t *handle);

/**
 * @brief  Set accelerometer ODR, bandwidth and undersampling.
 * @note   Must be called while accelerometer is in suspend mode.
 */
esp_err_t bmi160_acc_set_conf(bmi160_handle_t *handle,
			      uint8_t odr,
			      uint8_t bwp,
			      uint8_t us);

/**
 * @brief  Set accelerometer full-scale range.
 */
esp_err_t bmi160_acc_set_range(bmi160_handle_t *handle,
			       bmi160_acc_range_val_t range);

/**
 * @brief  Set gyroscope ODR and bandwidth.
 */
esp_err_t bmi160_gyr_set_conf(bmi160_handle_t *handle,
			      uint8_t odr,
			      uint8_t bwp);

/**
 * @brief  Set gyroscope full-scale range.
 */
esp_err_t bmi160_gyr_set_range(bmi160_handle_t *handle,
			       bmi160_gyr_range_val_t range);

/**
 * @brief  Transition the accelerometer PMU to a target mode.
 */
esp_err_t bmi160_acc_set_mode(bmi160_handle_t *handle, bmi160_mode_t mode);

/**
 * @brief  Transition the gyroscope PMU to a target mode.
 */
esp_err_t bmi160_gyr_set_mode(bmi160_handle_t *handle, bmi160_mode_t mode);

/**
 * @brief  Read raw accelerometer data.
 * @param  out  [out] raw 16-bit signed axes.
 */
esp_err_t bmi160_acc_read_raw(bmi160_handle_t *handle, bmi160_axes_raw_t *out);

/**
 * @brief  Read raw gyroscope data.
 * @param  out  [out] raw 16-bit signed axes.
 */
esp_err_t bmi160_gyr_read_raw(bmi160_handle_t *handle, bmi160_axes_raw_t *out);

/**
 * @brief  Read accelerometer converted to g.
 * @param  range  the configured accelerometer range.
 * @param  out    [out] acceleration in g.
 */
esp_err_t bmi160_acc_read_g(bmi160_handle_t *handle,
			    bmi160_acc_range_val_t range,
			    bmi160_axes_g_t *out);

/**
 * @brief  Read gyroscope converted to degrees/second.
 * @param  range  the configured gyroscope range.
 * @param  out    [out] angular rate in dps.
 */
esp_err_t bmi160_gyr_read_dps(bmi160_handle_t *handle,
			      bmi160_gyr_range_val_t range,
			      bmi160_gyro_dps_t *out);

/**
 * @brief  Read the current interrupt status registers.
 * @param  status  [out] 4-byte interrupt status (INT_STATUS_0..3).
 */
esp_err_t bmi160_int_status_read(bmi160_handle_t *handle, uint8_t status[4]);

/* -------- Motion / Event Configuration (call while PMU suspended) -------- */

/**
 * @brief  Configure any-motion detection.
 * @param  threshold  LSB; 1 LSB ≈ 15.6 mg (0–3.9 g).
 * @param  duration   number of consecutive samples threshold must be exceeded.
 */
esp_err_t bmi160_any_motion_configure(bmi160_handle_t *handle,
				      uint8_t threshold,
				      uint8_t duration);

/**
 * @brief  Configure significant-motion detection.
 * @param  threshold  LSB; 1 LSB ≈ 15.6 mg.
 * @param  duration   number of consecutive samples.
 */
esp_err_t bmi160_sig_motion_configure(bmi160_handle_t *handle,
				      uint8_t threshold,
				      uint8_t duration);

/**
 * @brief  Enable single-tap detection.
 */
esp_err_t bmi160_tap_configure(bmi160_handle_t *handle,
			       uint8_t duration,
			       uint8_t threshold,
			       uint8_t shock,
			       uint8_t quiet);

/**
 * @brief  Enable step-counter interrupt.
 * @note   step_counter_en = true enables the built-in step detector.
 */
esp_err_t bmi160_step_counter_configure(bmi160_handle_t *handle,
					bool step_counter_en,
					uint8_t sensitivity);

/**
 * @brief  Read step counter value (0–65535).
 */
esp_err_t bmi160_step_counter_read(bmi160_handle_t *handle, uint16_t *steps);

/**
 * @brief  Map interrupt sources to INT1 / INT2 pins.
 *
 * Writes raw values to INT_MAP_0 (0x55), INT_MAP_1 (0x56), INT_MAP_2 (0x57).
 *
 * Register bit semantics (set bit = route to the target pin):
 *   INT_MAP_0 → INT1:  bit 7=Flat, 6=Orient, 5=SingleTap, 4=DoubleTap,
 *                       3=NoMotion, 2=AnyMotion/SigMotion, 1=HighG, 0=LowG/Step
 *   INT_MAP_1 → INT1   (bits 7:4) / INT2 (bits 3:0):
 *                       7/3=DRDY, 6/2=FIFOWM, 5/1=FIFOFull, 4/0=PMUTrigger
 *   INT_MAP_2 → INT2:  same bit layout as INT_MAP_0
 *
 * @param  int_map_0/1/2  raw values for INT_MAP_0/1/2 registers.
 */
esp_err_t bmi160_int_map_set(bmi160_handle_t *handle,
			     uint8_t int_map_0,
			     uint8_t int_map_1,
			     uint8_t int_map_2);

/**
 * @brief  Set interrupt output mode (active level, push-pull etc.).
 */
esp_err_t bmi160_int_out_ctrl_set(bmi160_handle_t *handle, uint8_t value);

/**
 * @brief  Set interrupt latch mode.
 */
esp_err_t bmi160_int_latch_set(bmi160_handle_t *handle, uint8_t value);

/**
 * @brief  Enable interrupt sources (writes INT_EN_0/1/2).
 */
esp_err_t bmi160_int_enable(bmi160_handle_t *handle,
			    uint8_t en_0,
			    uint8_t en_1,
			    uint8_t en_2);

/**
 * @brief  Read the logic level of INT1 pin.
 * @param  level  [out] 0 or 1.
 * @return ESP_OK, or ESP_ERR_NOT_SUPPORTED if INT1 pin not configured.
 */
esp_err_t bmi160_int1_pin_read(bmi160_handle_t *handle, int *level);

/**
 * @brief  Read the logic level of INT2 pin.
 * @param  level  [out] 0 or 1.
 * @return ESP_OK, or ESP_ERR_NOT_SUPPORTED if INT2 pin not configured.
 */
esp_err_t bmi160_int2_pin_read(bmi160_handle_t *handle, int *level);

/**
 * @brief  Register an ISR handler for the INT1 pin.
 *
 * Installs the GPIO ISR service (if not already installed) and attaches
 * @p isr_handler to the INT1 pin configured in bmi160_i2c_config_t.
 * The ISR runs when the pin's interrupt condition (set via int1_type) is met.
 *
 * @param  handle       BMI160 device handle.
 * @param  isr_handler  ISR callback function (must have IRAM_ATTR if used
 *                      in no-cache contexts).
 * @param  args         Optional user argument passed to the callback.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t bmi160_int1_isr_add(bmi160_handle_t *handle,
			      gpio_isr_t isr_handler,
			      void *args);

/**
 * @brief  Register an ISR handler for the INT2 pin.
 *
 * @copydetails bmi160_int1_isr_add
 */
esp_err_t bmi160_int2_isr_add(bmi160_handle_t *handle,
			      gpio_isr_t isr_handler,
			      void *args);

/**
 * @brief  Remove (unregister) the ISR handler for the INT1 pin.
 *
 * @param  handle  BMI160 device handle.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t bmi160_int1_isr_remove(bmi160_handle_t *handle);

/**
 * @brief  Remove (unregister) the ISR handler for the INT2 pin.
 *
 * @param  handle  BMI160 device handle.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t bmi160_int2_isr_remove(bmi160_handle_t *handle);

#ifdef __cplusplus
}
#endif
