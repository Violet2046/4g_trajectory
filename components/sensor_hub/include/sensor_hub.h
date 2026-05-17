#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/i2c_master.h"
#include "esp_err.h"

#include "BMI160.h"
#include "AK09911C.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========================================================================= */
/*  sensor_hub — shared I2C bus manager for BMI160 + AK09911C                */
/* ========================================================================= */

/* ------------------------------------------------------------------------- */
/*  Sensor sample data (matches storage_record_t layout)                     */
/* ------------------------------------------------------------------------- */
typedef struct {
	uint32_t timestamp;

	/* GPS */
	bool     gps_fix;
	float    lat;
	float    lon;

	/* BMI160 */
	bmi160_axes_g_t   acc;
	bmi160_gyro_dps_t gyr;

	/* AK09911C */
	bool              mag_valid;
	ak09911_axes_ut_t mag;
} sensor_sample_t;

/* ------------------------------------------------------------------------- */
/*  Configuration                                                            */
/* ------------------------------------------------------------------------- */
typedef struct {
	/* I2C bus pins */
	int sda_io_num;
	int scl_io_num;

	/* BMI160 config */
	int         bmi160_int1_pin;
	gpio_int_type_t bmi160_int1_type;
	int         bmi160_int2_pin;   /* GPIO_NUM_NC to disable */
	gpio_int_type_t bmi160_int2_type;

	/* AK09911C addresses to try (typically {0x0C, 0x0D}) */
	const uint16_t *ak09911_addrs;
	int             ak09911_addr_count;
} sensor_hub_config_t;

/* ------------------------------------------------------------------------- */
/*  Handle (opaque)                                                          */
/* ------------------------------------------------------------------------- */
typedef struct sensor_hub_t sensor_hub_t;

/* ------------------------------------------------------------------------- */
/*  Public API                                                               */
/* ------------------------------------------------------------------------- */

/**
 * @brief  Create shared I2C bus, auto-detect and init all sensors.
 *
 * - Creates one I2C bus on @p sda/scl
 * - Tries BMI160 at addresses 0x68, 0x69
 * - Tries AK09911C at configured addresses (default 0x0C, 0x0D)
 * - Any sensor not found is gracefully skipped
 *
 * @param  out_hub  [out] hub handle
 * @param  config   pin and address configuration
 * @return ESP_OK on success (even if some sensors are missing)
 */
esp_err_t sensor_hub_init(sensor_hub_t **out_hub,
			  const sensor_hub_config_t *config);

/**
 * @brief  Destroy hub and all sensor handles.
 */
void sensor_hub_destroy(sensor_hub_t *hub);

/**
 * @brief  Read all available sensors into a sample structure.
 *
 * Fast, non-blocking per-sensor. GPS is not included (handled externally).
 */
esp_err_t sensor_hub_sample(sensor_hub_t *hub, sensor_sample_t *sample);

/**
 * @brief  Low-power: suspend gyro, set acc low-power, power-down mag.
 */
esp_err_t sensor_hub_sleep(sensor_hub_t *hub);

/**
 * @brief  Wake: restore acc/gyro normal, restart mag continuous mode.
 */
esp_err_t sensor_hub_wake(sensor_hub_t *hub);

/* ---- Handle accessors (for ISR registration, etc.) ---- */
bmi160_handle_t  *sensor_hub_get_bmi160(sensor_hub_t *hub);
ak09911_handle_t *sensor_hub_get_ak09911(sensor_hub_t *hub);
i2c_master_bus_handle_t sensor_hub_get_bus(sensor_hub_t *hub);

/* ---- Convenience: check if sensors are present ---- */
bool sensor_hub_has_bmi160(const sensor_hub_t *hub);
bool sensor_hub_has_ak09911(const sensor_hub_t *hub);

#ifdef __cplusplus
}
#endif
