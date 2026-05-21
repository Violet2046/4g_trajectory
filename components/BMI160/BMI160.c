#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_log.h"

#include "driver/gpio.h"

#include "BMI160.h"

#define BMI160_TAG "BMI160"

#define BMI160_CHIP_ID_VAL  0xD1
#define BMI160_I2C_TIMEOUT_MS 100

/* All accelerometer data registers (ACC_X_LSB … ACC_Z_MSB) */
#define BMI160_ACC_DATA_LEN 6
/* All gyroscope data registers */
#define BMI160_GYR_DATA_LEN 6

struct bmi160_handle_t {
	bmi160_i2c_config_t     config;
	i2c_master_bus_handle_t  bus_handle;
	i2c_master_dev_handle_t  dev_handle;
	SemaphoreHandle_t       lock;
	bool                    bus_owned;   /* true if we created the bus ourselves */
};

/* ------------------------------------------------------------------------- */
/*  Register helpers                                                          */
/* ------------------------------------------------------------------------- */
static esp_err_t bmi160_reg_read(bmi160_handle_t *handle,
				 uint8_t reg,
				 uint8_t *data,
				 size_t len)
{
	return i2c_master_transmit_receive(handle->dev_handle,
					   &reg, 1,
					   data, len,
					   BMI160_I2C_TIMEOUT_MS);
}

static esp_err_t bmi160_reg_write(bmi160_handle_t *handle,
				  uint8_t reg,
				  uint8_t value)
{
	uint8_t buf[2] = { reg, value };
	return i2c_master_transmit(handle->dev_handle,
				   buf, sizeof(buf),
				   BMI160_I2C_TIMEOUT_MS);
}

static esp_err_t bmi160_cmd(bmi160_handle_t *handle, uint8_t cmd)
{
	return bmi160_reg_write(handle, BMI160_CMD, cmd);
}

/* ------------------------------------------------------------------------- */
/*  Create / Destroy                                                          */
/* ------------------------------------------------------------------------- */
esp_err_t bmi160_create(bmi160_handle_t **out_handle,
			const bmi160_i2c_config_t *config)
{
	bmi160_handle_t *handle;
	esp_err_t err;
	i2c_device_config_t dev_cfg;
	uint8_t chip_id = 0;

	if ((out_handle == NULL) || (config == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	handle = (bmi160_handle_t *)calloc(1, sizeof(bmi160_handle_t));
	if (handle == NULL) {
		return ESP_ERR_NO_MEM;
	}

	handle->config = *config;
	handle->lock = xSemaphoreCreateMutex();
	if (handle->lock == NULL) {
		free(handle);
		return ESP_ERR_NO_MEM;
	}

	/* Use external bus handle if provided, otherwise create new bus */
	if (config->bus_handle != NULL) {
		handle->bus_handle = config->bus_handle;
		handle->bus_owned  = false;
		ESP_LOGD(BMI160_TAG, "using external I2C bus handle");
	} else {
		err = i2c_new_master_bus(&handle->config.bus_cfg, &handle->bus_handle);
		if (err != ESP_OK) {
			ESP_LOGE(BMI160_TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
			vSemaphoreDelete(handle->lock);
			free(handle);
			return err;
		}
		handle->bus_owned = true;
	}

	/* Add BMI160 device on the bus */
	dev_cfg = (i2c_device_config_t){
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address   = handle->config.dev_addr,
		.scl_speed_hz     = handle->config.i2c_freq_hz,
	};

	err = i2c_master_bus_add_device(handle->bus_handle,
					&dev_cfg,
					&handle->dev_handle);
	if (err != ESP_OK) {
		ESP_LOGE(BMI160_TAG, "i2c_master_bus_add_device failed: %s", esp_err_to_name(err));
		if (handle->bus_owned) i2c_del_master_bus(handle->bus_handle);
		vSemaphoreDelete(handle->lock);
		free(handle);
		return err;
	}

	/* Verify chip ID */
	err = bmi160_reg_read(handle, BMI160_CHIP_ID, &chip_id, 1);
	if ((err != ESP_OK) || (chip_id != BMI160_CHIP_ID_VAL)) {
		ESP_LOGE(BMI160_TAG, "Bad chip_id: 0x%02X (expected 0x%02X)",
			 chip_id, BMI160_CHIP_ID_VAL);
		i2c_master_bus_rm_device(handle->dev_handle);
		if (handle->bus_owned) i2c_del_master_bus(handle->bus_handle);
		vSemaphoreDelete(handle->lock);
		free(handle);
		return ESP_ERR_NOT_FOUND;
	}

	/* Init INT1 / INT2 GPIOs as input with pull-up if configured */
	if (handle->config.int1_pin != GPIO_NUM_NC) {
		gpio_set_direction(handle->config.int1_pin, GPIO_MODE_INPUT);
		gpio_set_pull_mode(handle->config.int1_pin, GPIO_PULLUP_ONLY);
		gpio_set_intr_type(handle->config.int1_pin, handle->config.int1_type);
		ESP_LOGI(BMI160_TAG, "INT1 pin: GPIO_NUM_%d, intr_type: %d", (int)handle->config.int1_pin, (int)handle->config.int1_type);
	}
	if (handle->config.int2_pin != GPIO_NUM_NC) {
		gpio_set_direction(handle->config.int2_pin, GPIO_MODE_INPUT);
		gpio_set_pull_mode(handle->config.int2_pin, GPIO_PULLUP_ONLY);
		gpio_set_intr_type(handle->config.int2_pin, handle->config.int2_type);
		ESP_LOGI(BMI160_TAG, "INT2 pin: GPIO_NUM_%d, intr_type: %d", (int)handle->config.int2_pin, (int)handle->config.int2_type);
	}

	/* Put both sensors in suspend after power-up */
	(void)bmi160_cmd(handle, BMI160_CMD_ACC_PMU_SUSPEND);
	vTaskDelay(pdMS_TO_TICKS(5));
	(void)bmi160_cmd(handle, BMI160_CMD_GYR_PMU_SUSPEND);
	vTaskDelay(pdMS_TO_TICKS(5));

	*out_handle = handle;
	return ESP_OK;
}

void bmi160_destroy(bmi160_handle_t *handle)
{
	if (handle == NULL) {
		return;
	}
	if (handle->dev_handle != NULL) {
		i2c_master_bus_rm_device(handle->dev_handle);
	}
	/* Only delete the bus if we created it (not externally provided) */
	if (handle->bus_owned && handle->bus_handle != NULL) {
		i2c_del_master_bus(handle->bus_handle);
	}
	if (handle->lock != NULL) {
		vSemaphoreDelete(handle->lock);
	}
	free(handle);
}

/* ------------------------------------------------------------------------- */
/*  Soft Reset                                                               */
/* ------------------------------------------------------------------------- */
esp_err_t bmi160_soft_reset(bmi160_handle_t *handle)
{
	esp_err_t err;
	uint8_t pmu;
	int retry;

	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	err = bmi160_cmd(handle, BMI160_CMD_SOFT_RESET);
	if (err != ESP_OK) {
		return err;
	}
	vTaskDelay(pdMS_TO_TICKS(10));

	/* Wait until PMU_STATUS reports both sensors in suspend */
	for (retry = 0; retry < 50; retry++) {
		err = bmi160_reg_read(handle, BMI160_PMU_STATUS, &pmu, 1);
		if (err != ESP_OK) {
			return err;
		}
		if (pmu == 0x00) {
			return ESP_OK;
		}
		vTaskDelay(pdMS_TO_TICKS(2));
	}

	return ESP_ERR_TIMEOUT;
}

/* ------------------------------------------------------------------------- */
/*  Sensor Configuration                                                     */
/* ------------------------------------------------------------------------- */
esp_err_t bmi160_acc_set_conf(bmi160_handle_t *handle,
			      uint8_t odr,
			      uint8_t bwp,
			      uint8_t us)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	return bmi160_reg_write(handle, BMI160_ACC_CONF,
				(uint8_t)(odr | (bwp << 4) | us));
}

esp_err_t bmi160_acc_set_range(bmi160_handle_t *handle,
			       bmi160_acc_range_val_t range)
{
	uint8_t reg_val;

	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	switch (range) {
	case BMI160_ACC_RANGE_2G_VAL:  reg_val = BMI160_ACC_RANGE_2G;  break;
	case BMI160_ACC_RANGE_4G_VAL:  reg_val = BMI160_ACC_RANGE_4G;  break;
	case BMI160_ACC_RANGE_8G_VAL:  reg_val = BMI160_ACC_RANGE_8G;  break;
	case BMI160_ACC_RANGE_16G_VAL: reg_val = BMI160_ACC_RANGE_16G; break;
	default: return ESP_ERR_INVALID_ARG;
	}

	return bmi160_reg_write(handle, BMI160_ACC_RANGE, reg_val);
}

esp_err_t bmi160_gyr_set_conf(bmi160_handle_t *handle,
			      uint8_t odr,
			      uint8_t bwp)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	return bmi160_reg_write(handle, BMI160_GYR_CONF,
				(uint8_t)(odr | (bwp << 4)));
}

esp_err_t bmi160_gyr_set_range(bmi160_handle_t *handle,
			       bmi160_gyr_range_val_t range)
{
	uint8_t reg_val;

	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	switch (range) {
	case BMI160_GYR_RANGE_125_VAL:  reg_val = BMI160_GYR_RANGE_125;  break;
	case BMI160_GYR_RANGE_250_VAL:  reg_val = BMI160_GYR_RANGE_250;  break;
	case BMI160_GYR_RANGE_500_VAL:  reg_val = BMI160_GYR_RANGE_500;  break;
	case BMI160_GYR_RANGE_1000_VAL: reg_val = BMI160_GYR_RANGE_1000; break;
	case BMI160_GYR_RANGE_2000_VAL: reg_val = BMI160_GYR_RANGE_2000; break;
	default: return ESP_ERR_INVALID_ARG;
	}

	return bmi160_reg_write(handle, BMI160_GYR_RANGE, reg_val);
}

/* ------------------------------------------------------------------------- */
/*  Power-Mode Transitions                                                   */
/* ------------------------------------------------------------------------- */
static esp_err_t bmi160_wait_pmu_status(bmi160_handle_t *handle,
					uint8_t mask,
					uint8_t expected)
{
	uint8_t status;
	int retry;

	for (retry = 0; retry < 100; retry++) {
		esp_err_t err = bmi160_reg_read(handle, BMI160_PMU_STATUS,
						&status, 1);
		if (err != ESP_OK) {
			return err;
		}
		if ((status & mask) == expected) {
			return ESP_OK;
		}
		vTaskDelay(pdMS_TO_TICKS(1));
	}

	return ESP_ERR_TIMEOUT;
}

esp_err_t bmi160_acc_set_mode(bmi160_handle_t *handle, bmi160_mode_t mode)
{
	uint8_t cmd;
	uint8_t expected;

	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	switch (mode) {
	case BMI160_MODE_SUSPEND:
		cmd = BMI160_CMD_ACC_PMU_SUSPEND;
		expected = BMI160_ACC_PMU_SUSPEND;
		break;
	case BMI160_MODE_NORMAL:
		cmd = BMI160_CMD_ACC_PMU_NORMAL;
		expected = BMI160_ACC_PMU_NORMAL;
		break;
	case BMI160_MODE_LOW_POWER:
		cmd = BMI160_CMD_ACC_PMU_LOW_POWER;
		expected = BMI160_ACC_PMU_LOW_POWER;
		break;
	case BMI160_MODE_FAST_STARTUP:
		cmd = BMI160_CMD_ACC_PMU_FAST_STARTUP;
		expected = BMI160_ACC_PMU_FAST_STARTUP;
		break;
	default:
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t err = bmi160_cmd(handle, cmd);
	if (err != ESP_OK) {
		return err;
	}

	vTaskDelay(pdMS_TO_TICKS(5));

	return bmi160_wait_pmu_status(handle,
				      BMI160_ACC_PMU_STATUS_MASK,
				      expected);
}

esp_err_t bmi160_gyr_set_mode(bmi160_handle_t *handle, bmi160_mode_t mode)
{
	uint8_t cmd;
	uint8_t expected;

	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	switch (mode) {
	case BMI160_MODE_SUSPEND:
		cmd = BMI160_CMD_GYR_PMU_SUSPEND;
		expected = BMI160_GYR_PMU_SUSPEND;
		break;
	case BMI160_MODE_NORMAL:
		cmd = BMI160_CMD_GYR_PMU_NORMAL;
		expected = BMI160_GYR_PMU_NORMAL;
		break;
	case BMI160_MODE_FAST_STARTUP:
		cmd = BMI160_CMD_GYR_PMU_FAST_STARTUP;
		expected = BMI160_GYR_PMU_FAST_STARTUP;
		break;
	default:
		/* Gyro does not have low-power mode */
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t err = bmi160_cmd(handle, cmd);
	if (err != ESP_OK) {
		return err;
	}

	vTaskDelay(pdMS_TO_TICKS(30));

	return bmi160_wait_pmu_status(handle,
				      BMI160_GYR_PMU_STATUS_MASK,
				      expected);
}

/* ------------------------------------------------------------------------- */
/*  Data Reading                                                             */
/* ------------------------------------------------------------------------- */
esp_err_t bmi160_acc_read_raw(bmi160_handle_t *handle, bmi160_axes_raw_t *out)
{
	uint8_t data[6];

	if ((handle == NULL) || (out == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t err = bmi160_reg_read(handle, BMI160_DATA_14, data, 6);
	if (err != ESP_OK) {
		return err;
	}

	out->x = (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
	out->y = (int16_t)((uint16_t)data[2] | ((uint16_t)data[3] << 8));
	out->z = (int16_t)((uint16_t)data[4] | ((uint16_t)data[5] << 8));

	return ESP_OK;
}

esp_err_t bmi160_gyr_read_raw(bmi160_handle_t *handle, bmi160_axes_raw_t *out)
{
	uint8_t data[6];

	if ((handle == NULL) || (out == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t err = bmi160_reg_read(handle, BMI160_DATA_8, data, 6);
	if (err != ESP_OK) {
		return err;
	}

	out->x = (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
	out->y = (int16_t)((uint16_t)data[2] | ((uint16_t)data[3] << 8));
	out->z = (int16_t)((uint16_t)data[4] | ((uint16_t)data[5] << 8));

	return ESP_OK;
}

esp_err_t bmi160_acc_read_g(bmi160_handle_t *handle,
			    bmi160_acc_range_val_t range,
			    bmi160_axes_g_t *out)
{
	bmi160_axes_raw_t raw;
	esp_err_t err = bmi160_acc_read_raw(handle, &raw);
	float scale;

	if (err != ESP_OK) {
		return err;
	}

	/* Sensitivity for ±range (16-bit signed) */
	switch (range) {
	case BMI160_ACC_RANGE_2G_VAL:  scale = 2.0f  / 32768.0f; break;
	case BMI160_ACC_RANGE_4G_VAL:  scale = 4.0f  / 32768.0f; break;
	case BMI160_ACC_RANGE_8G_VAL:  scale = 8.0f  / 32768.0f; break;
	case BMI160_ACC_RANGE_16G_VAL: scale = 16.0f / 32768.0f; break;
	default: return ESP_ERR_INVALID_ARG;
	}

	out->x = raw.x * scale;
	out->y = raw.y * scale;
	out->z = raw.z * scale;

	return ESP_OK;
}

esp_err_t bmi160_gyr_read_dps(bmi160_handle_t *handle,
			      bmi160_gyr_range_val_t range,
			      bmi160_gyro_dps_t *out)
{
	bmi160_axes_raw_t raw;
	esp_err_t err = bmi160_gyr_read_raw(handle, &raw);
	float scale;

	if (err != ESP_OK) {
		return err;
	}

	switch (range) {
	case BMI160_GYR_RANGE_125_VAL:  scale = 125.0f  / 32768.0f; break;
	case BMI160_GYR_RANGE_250_VAL:  scale = 250.0f  / 32768.0f; break;
	case BMI160_GYR_RANGE_500_VAL:  scale = 500.0f  / 32768.0f; break;
	case BMI160_GYR_RANGE_1000_VAL: scale = 1000.0f / 32768.0f; break;
	case BMI160_GYR_RANGE_2000_VAL: scale = 2000.0f / 32768.0f; break;
	default: return ESP_ERR_INVALID_ARG;
	}

	out->x = raw.x * scale;
	out->y = raw.y * scale;
	out->z = raw.z * scale;

	return ESP_OK;
}

esp_err_t bmi160_int_status_read(bmi160_handle_t *handle, uint8_t status[4])
{
	if ((handle == NULL) || (status == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}
	return bmi160_reg_read(handle, BMI160_INT_STATUS_0, status, 4);
}

/* ------------------------------------------------------------------------- */
/*  Motion / Event Configuration                                             */
/* ------------------------------------------------------------------------- */
esp_err_t bmi160_any_motion_configure(bmi160_handle_t *handle,
				      uint8_t threshold,
				      uint8_t duration)
{
	esp_err_t err;

	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	/* INT_MOTION_0 : any-motion threshold */
	err = bmi160_reg_write(handle, BMI160_INT_MOTION_0, threshold);
	if (err != ESP_OK) return err;

	/* INT_MOTION_1 : any-motion duration */
	err = bmi160_reg_write(handle, BMI160_INT_MOTION_1, duration);
	if (err != ESP_OK) return err;

	/* Enable any-motion on X/Y/Z in INT_EN_0 */
	return bmi160_reg_write(handle, BMI160_INT_EN_0,
				BMI160_INT_ANY_MOTION_X_EN |
				BMI160_INT_ANY_MOTION_Y_EN |
				BMI160_INT_ANY_MOTION_Z_EN);
}

esp_err_t bmi160_sig_motion_configure(bmi160_handle_t *handle,
				      uint8_t threshold,
				      uint8_t duration)
{
	esp_err_t err;

	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	/* INT_MOTION_2 : significant-motion threshold */
	err = bmi160_reg_write(handle, BMI160_INT_MOTION_2, threshold);
	if (err != ESP_OK) return err;

	/* INT_MOTION_3 : significant-motion duration */
	err = bmi160_reg_write(handle, BMI160_INT_MOTION_3, duration);
	if (err != ESP_OK) return err;

	/* Enable significant-motion on X/Y/Z in INT_EN_0.
	 * According to datasheet, sig-motion shares any-motion logic implicitly
	 * and uses its own mask in INT_EN_0 (bit 0-2) or INT_EN_1 (bit 0) depending on version. 
	 * BMI160 sets sig-motion enable at INT_EN_0 along with any-motion, or implicitly works 
	 * when any_motion is set but read from sig_motion status. Actually, INT_EN_0 bit 0/1/2 
	 * are any_motion. Sig motion is INT_EN_0 bit 0/1/2 AND INT_MOTION_3 config, but simply we 
	 * need to enable any_motion on X/Y/Z (INT_EN_0 = 0x07) and map sig_motion (INT_MAP_x).
	 * Wait, proper sig_motion enable is INT_EN_0 bit 0, 1, 2 for X,Y,Z any_motion, 
	 * and sig-motion is generated internally. Let's just write INT_EN_0.
	 */
	return bmi160_reg_write(handle, BMI160_INT_EN_0, BMI160_INT_SIG_MOTION_EN);
}

esp_err_t bmi160_tap_configure(bmi160_handle_t *handle,
			       uint8_t duration,
			       uint8_t threshold,
			       uint8_t shock,
			       uint8_t quiet)
{
	esp_err_t err;

	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	/* INT_TAP_0 : tap duration, shock */
	err = bmi160_reg_write(handle, BMI160_INT_TAP_0,
			       (uint8_t)((quiet << 7) | (shock << 6) | duration));
	if (err != ESP_OK) return err;

	/* INT_TAP_1 : tap threshold */
	err = bmi160_reg_write(handle, BMI160_INT_TAP_1, threshold);
	if (err != ESP_OK) return err;

	/* Enable single / double tap in INT_EN_2 */
	return bmi160_reg_write(handle, BMI160_INT_EN_2,
				BMI160_INT_TAP_EN | BMI160_INT_DOUBLE_TAP_EN);
}

esp_err_t bmi160_double_tap_configure(bmi160_handle_t *handle)
{
	esp_err_t err;

	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	/* Tap parameters with defaults */
	err = bmi160_tap_configure(handle,
				   BMI160_TAP_DUR_DEFAULT,
				   BMI160_TAP_THR_DEFAULT,
				   BMI160_TAP_SHOCK_DEFAULT,
				   BMI160_TAP_QUIET_DEFAULT);
	if (err != ESP_OK) return err;

	/* Read-modify-write INT_MAP_1: add double-tap bit for INT2.
	 * Preserve any existing INT1 mapping (INT_MAP_0) and other bits.
	 * We only touch INT_MAP_1, leaving INT_MAP_0/2 unchanged. */
	uint8_t map1;
	err = bmi160_reg_read(handle, BMI160_INT_MAP_1, &map1, 1);
	if (err != ESP_OK) return err;

	map1 |= BMI160_INT_MAP_DOUBLE_TAP;   /* bit 6 = double-tap → INT2 */

	err = bmi160_reg_write(handle, BMI160_INT_MAP_1, map1);
	if (err != ESP_OK) return err;

	ESP_LOGI(BMI160_TAG, "double-tap configured, mapped to INT2");
	return ESP_OK;
}

esp_err_t bmi160_step_counter_configure(bmi160_handle_t *handle,
					bool step_counter_en,
					uint8_t sensitivity)
{
	uint8_t step_conf;

	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	step_conf = sensitivity & 0x03;

	if (step_counter_en) {
		step_conf |= BMI160_STEP_CNT_EN;

		/* enable step-detector interrupt in INT_EN_2 */
		esp_err_t err = bmi160_reg_write(handle, BMI160_INT_EN_2,
						  BMI160_INT_STEP_DETECT_EN);
		if (err != ESP_OK) return err;
	}

	return bmi160_reg_write(handle, BMI160_STEP_CONF_1, step_conf);
}

esp_err_t bmi160_step_counter_read(bmi160_handle_t *handle, uint16_t *steps)
{
	uint8_t data[2];
	esp_err_t err;

	if ((handle == NULL) || (steps == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	err = bmi160_reg_read(handle, BMI160_STEP_CNT_0, data, 2);
	if (err != ESP_OK) {
		return err;
	}

	*steps = (uint16_t)(((uint16_t)data[0]) | ((uint16_t)data[1] << 8));
	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Interrupt Mapping / Output                                               */
/* ------------------------------------------------------------------------- */
esp_err_t bmi160_int_map_set(bmi160_handle_t *handle,
			     uint8_t int_map_0,
			     uint8_t int_map_1,
			     uint8_t int_map_2)
{
	esp_err_t err;

	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	err = bmi160_reg_write(handle, BMI160_INT_MAP_0, int_map_0);
	if (err != ESP_OK) return err;

	err = bmi160_reg_write(handle, BMI160_INT_MAP_1, int_map_1);
	if (err != ESP_OK) return err;

	return bmi160_reg_write(handle, BMI160_INT_MAP_2, int_map_2);
}

esp_err_t bmi160_int_out_ctrl_set(bmi160_handle_t *handle, uint8_t value)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	return bmi160_reg_write(handle, BMI160_INT_OUT_CTRL, value);
}

esp_err_t bmi160_int_latch_set(bmi160_handle_t *handle, uint8_t value)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	return bmi160_reg_write(handle, BMI160_INT_LATCH, value);
}

esp_err_t bmi160_int_enable(bmi160_handle_t *handle,
			    uint8_t en_0,
			    uint8_t en_1,
			    uint8_t en_2)
{
	esp_err_t err;

	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	err = bmi160_reg_write(handle, BMI160_INT_EN_0, en_0);
	if (err != ESP_OK) return err;

	err = bmi160_reg_write(handle, BMI160_INT_EN_1, en_1);
	if (err != ESP_OK) return err;

	return bmi160_reg_write(handle, BMI160_INT_EN_2, en_2);
}

/* ------------------------------------------------------------------------- */
/*  Interrupt pin helpers                                                     */
/* ------------------------------------------------------------------------- */
esp_err_t bmi160_int1_pin_read(bmi160_handle_t *handle, int *level)
{
	if ((handle == NULL) || (level == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}
	if (handle->config.int1_pin == GPIO_NUM_NC) {
		return ESP_ERR_NOT_SUPPORTED;
	}
	*level = gpio_get_level(handle->config.int1_pin);
	return ESP_OK;
}

esp_err_t bmi160_int2_pin_read(bmi160_handle_t *handle, int *level)
{
	if ((handle == NULL) || (level == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}
	if (handle->config.int2_pin == GPIO_NUM_NC) {
		return ESP_ERR_NOT_SUPPORTED;
	}
	*level = gpio_get_level(handle->config.int2_pin);
	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  ISR registration helpers                                                  */
/* ------------------------------------------------------------------------- */
esp_err_t bmi160_int1_isr_add(bmi160_handle_t *handle,
			      gpio_isr_t isr_handler,
			      void *args)
{
	if ((handle == NULL) || (isr_handler == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}
	if (handle->config.int1_pin == GPIO_NUM_NC) {
		return ESP_ERR_NOT_SUPPORTED;
	}

	gpio_install_isr_service(0);
	gpio_isr_handler_add(handle->config.int1_pin, isr_handler, args);
	ESP_LOGI(BMI160_TAG, "ISR registered on INT1 pin GPIO_NUM_%d",
		 (int)handle->config.int1_pin);
	return ESP_OK;
}

esp_err_t bmi160_int2_isr_add(bmi160_handle_t *handle,
			      gpio_isr_t isr_handler,
			      void *args)
{
	if ((handle == NULL) || (isr_handler == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}
	if (handle->config.int2_pin == GPIO_NUM_NC) {
		return ESP_ERR_NOT_SUPPORTED;
	}

	gpio_install_isr_service(0);
	gpio_isr_handler_add(handle->config.int2_pin, isr_handler, args);
	ESP_LOGI(BMI160_TAG, "ISR registered on INT2 pin GPIO_NUM_%d",
		 (int)handle->config.int2_pin);
	return ESP_OK;
}

esp_err_t bmi160_int1_isr_remove(bmi160_handle_t *handle)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	if (handle->config.int1_pin == GPIO_NUM_NC) {
		return ESP_ERR_NOT_SUPPORTED;
	}

	gpio_isr_handler_remove(handle->config.int1_pin);
	ESP_LOGI(BMI160_TAG, "ISR removed from INT1 pin GPIO_NUM_%d",
		 (int)handle->config.int1_pin);
	return ESP_OK;
}

esp_err_t bmi160_int2_isr_remove(bmi160_handle_t *handle)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}
	if (handle->config.int2_pin == GPIO_NUM_NC) {
		return ESP_ERR_NOT_SUPPORTED;
	}

	gpio_isr_handler_remove(handle->config.int2_pin);
	ESP_LOGI(BMI160_TAG, "ISR removed from INT2 pin GPIO_NUM_%d",
		 (int)handle->config.int2_pin);
	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Unified init                                                              */
/* ------------------------------------------------------------------------- */
esp_err_t bmi160_init(bmi160_handle_t **out_handle,
		      const bmi160_i2c_config_t *config)
{
	bmi160_handle_t *handle;
	esp_err_t err;

	if ((out_handle == NULL) || (config == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	/* 1.  create & verify chip */
	err = bmi160_create(&handle, config);
	if (err != ESP_OK) {
		return err;
	}

	/* 2.  accelerometer:  ±8 g, 100 Hz, normal bandwidth */
	err = bmi160_acc_set_range(handle, BMI160_ACC_RANGE_8G_VAL);
	if (err != ESP_OK) goto fail;
	err = bmi160_acc_set_conf(handle,
				  BMI160_ACC_ODR_100HZ,
				  BMI160_ACC_BWP_NORMAL,
				  BMI160_ACC_US_OFF);
	if (err != ESP_OK) goto fail;

	/* 3.  gyroscope:  ±500 dps, 100 Hz, normal bandwidth */
	err = bmi160_gyr_set_range(handle, BMI160_GYR_RANGE_500_VAL);
	if (err != ESP_OK) goto fail;
	err = bmi160_gyr_set_conf(handle,
				  BMI160_GYR_ODR_100HZ,
				  BMI160_GYR_BWP_NORMAL);
	if (err != ESP_OK) goto fail;

	/* 4.  significant-motion interrupt */
	err = bmi160_sig_motion_configure(handle,
					  BMI160_SIGM_THR_DEFAULT,
					  BMI160_SIGM_DUR_DEFAULT);
	if (err != ESP_OK) goto fail;

	/* 5.  map significant-motion → INT1  (INT_MAP[0] bit 2 = 0x04) */
	err = bmi160_int_map_set(handle, 0x04, 0x00, 0x00);
	if (err != ESP_OK) goto fail;

	/* 6.  INT1 output: active-high, push-pull  (int1_out_en=1, int1_lvl=1, int1_od=0) */
	err = bmi160_int_out_ctrl_set(handle, 0x0A);
	if (err != ESP_OK) goto fail;

	/* 7.  power on both sensors */
	err = bmi160_acc_set_mode(handle, BMI160_MODE_NORMAL);
	if (err != ESP_OK) goto fail;
	err = bmi160_gyr_set_mode(handle, BMI160_MODE_NORMAL);
	if (err != ESP_OK) goto fail;

	*out_handle = handle;
	ESP_LOGI(BMI160_TAG, "init complete — acc 8g/100Hz, gyr 500dps/100Hz, sig-motion on INT1");
	return ESP_OK;

fail:
	ESP_LOGE(BMI160_TAG, "bmi160_init step failed: %s", esp_err_to_name(err));
	bmi160_destroy(handle);
	return err;
}
