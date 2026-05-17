#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "esp_check.h"
#include "esp_log.h"

#include "AK09911C.h"

#define AK09911_TAG "AK09911C"

#define AK09911_I2C_TIMEOUT_MS 100

/* burst-read length: HXL..HZH + TMPS + ST2 = 8 bytes */
#define AK09911_BURST_LEN    8

struct ak09911_handle_t {
	ak09911_i2c_config_t     config;
	i2c_master_bus_handle_t  bus_handle;
	i2c_master_dev_handle_t  dev_handle;
	SemaphoreHandle_t        lock;
	bool                     bus_owned;  /* true if we created the bus ourselves */
	uint8_t                  asax;   /* factory sensitivity coefficients */
	uint8_t                  asay;
	uint8_t                  asaz;
};

/* ------------------------------------------------------------------------- */
/*  Register helpers                                                          */
/* ------------------------------------------------------------------------- */
static esp_err_t ak09911_reg_read(ak09911_handle_t *handle,
				  uint8_t reg,
				  uint8_t *data,
				  size_t len)
{
	return i2c_master_transmit_receive(handle->dev_handle,
					   &reg, 1,
					   data, len,
					   AK09911_I2C_TIMEOUT_MS);
}

static esp_err_t ak09911_reg_write(ak09911_handle_t *handle,
				   uint8_t reg,
				   uint8_t value)
{
	uint8_t buf[2] = { reg, value };
	return i2c_master_transmit(handle->dev_handle,
				   buf, sizeof(buf),
				   AK09911_I2C_TIMEOUT_MS);
}

/* ------------------------------------------------------------------------- */
/*  Create / Destroy                                                          */
/* ------------------------------------------------------------------------- */
esp_err_t ak09911_create(ak09911_handle_t **out_handle,
			 const ak09911_i2c_config_t *config)
{
	ak09911_handle_t *handle;
	esp_err_t err;
	i2c_device_config_t dev_cfg;
	uint8_t wia1 = 0, wia2 = 0;

	if ((out_handle == NULL) || (config == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	handle = (ak09911_handle_t *)calloc(1, sizeof(ak09911_handle_t));
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
		ESP_LOGD(AK09911_TAG, "using external I2C bus handle");
	} else {
		err = i2c_new_master_bus(&handle->config.bus_cfg, &handle->bus_handle);
		if (err != ESP_OK) {
			ESP_LOGE(AK09911_TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(err));
			vSemaphoreDelete(handle->lock);
			free(handle);
			return err;
		}
		handle->bus_owned = true;
	}

	/* Add AK09911C device on the bus */
	dev_cfg = (i2c_device_config_t){
		.dev_addr_length = I2C_ADDR_BIT_LEN_7,
		.device_address   = handle->config.dev_addr,
		.scl_speed_hz     = handle->config.i2c_freq_hz,
	};

	err = i2c_master_bus_add_device(handle->bus_handle, &dev_cfg,
					&handle->dev_handle);
	if (err != ESP_OK) {
		ESP_LOGE(AK09911_TAG, "i2c_master_bus_add_device failed: %s",
			 esp_err_to_name(err));
		if (handle->bus_owned) i2c_del_master_bus(handle->bus_handle);
		vSemaphoreDelete(handle->lock);
		free(handle);
		return err;
	}

	/* Verify WIA registers */
	err  = ak09911_reg_read(handle, AK09911_WIA1, &wia1, 1);
	err |= ak09911_reg_read(handle, AK09911_WIA2, &wia2, 1);
	if ((err != ESP_OK) || (wia1 != AK09911_WIA1_VAL)
			  || (wia2 != AK09911_WIA2_VAL)) {
		ESP_LOGE(AK09911_TAG, "Bad chip ID: WIA1=0x%02X WIA2=0x%02X "
			 "(expected 0x%02X 0x%02X)",
			 wia1, wia2, AK09911_WIA1_VAL, AK09911_WIA2_VAL);
		i2c_master_bus_rm_device(handle->dev_handle);
		if (handle->bus_owned) i2c_del_master_bus(handle->bus_handle);
		vSemaphoreDelete(handle->lock);
		free(handle);
		return ESP_ERR_NOT_FOUND;
	}

	*out_handle = handle;
	return ESP_OK;
}

void ak09911_destroy(ak09911_handle_t *handle)
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
/*  Init (chip verify + sensitivity coefficients)                            */
/* ------------------------------------------------------------------------- */
esp_err_t ak09911_init(ak09911_handle_t **out_handle,
		       const ak09911_i2c_config_t *config)
{
	ak09911_handle_t *handle;
	esp_err_t err;
	uint8_t fuse[AK09911_FUSE_LEN];

	err = ak09911_create(&handle, config);
	if (err != ESP_OK) {
		return err;
	}

	/* Read factory sensitivity coefficients from Fuse ROM */
	/* Step 1: enter Fuse ROM access mode */
	err = ak09911_reg_write(handle, AK09911_CNTL2, AK09911_MODE_FUSE_ROM);
	if (err != ESP_OK) goto fail;
	vTaskDelay(pdMS_TO_TICKS(1));

	/* Step 2: read ASAX/ASAY/ASAZ (3 bytes starting at 0x60) */
	err = ak09911_reg_read(handle, AK09911_ASAX, fuse, AK09911_FUSE_LEN);
	if (err != ESP_OK) goto fail;
	handle->asax = fuse[0];
	handle->asay = fuse[1];
	handle->asaz = fuse[2];

	/* Step 3: back to Power-down */
	err = ak09911_reg_write(handle, AK09911_CNTL2, AK09911_MODE_POWERDOWN);
	if (err != ESP_OK) goto fail;
	vTaskDelay(pdMS_TO_TICKS(1));

	*out_handle = handle;
	ESP_LOGI(AK09911_TAG, "init ok — addr 0x%02X, ASA=(%d,%d,%d)",
		 config->dev_addr,
		 handle->asax, handle->asay, handle->asaz);
	return ESP_OK;

fail:
	ESP_LOGE(AK09911_TAG, "ak09911_init failed: %s", esp_err_to_name(err));
	ak09911_destroy(handle);
	return err;
}

/* ------------------------------------------------------------------------- */
/*  Mode / Reset                                                              */
/* ------------------------------------------------------------------------- */
esp_err_t ak09911_mode_set(ak09911_handle_t *handle, uint8_t mode)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t err;

	/* Per datasheet §6.3: transition to another mode must go through
	 * Power-down first.  Wait at least 100 µs (Twat) between writes. */
	err = ak09911_reg_write(handle, AK09911_CNTL2, AK09911_MODE_POWERDOWN);
	if (err != ESP_OK) return err;
	vTaskDelay(pdMS_TO_TICKS(1));  /* 1 ms ≫ 100 µs */

	return ak09911_reg_write(handle, AK09911_CNTL2, mode);
}

esp_err_t ak09911_soft_reset(ak09911_handle_t *handle)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t err;
	uint8_t val;

	err = ak09911_reg_write(handle, AK09911_CNTL3, AK09911_CNTL3_SRST);
	if (err != ESP_OK) return err;
	vTaskDelay(pdMS_TO_TICKS(1));

	/* Verify SRST auto-cleared */
	err = ak09911_reg_read(handle, AK09911_CNTL3, &val, 1);
	if (err != ESP_OK) return err;
	if (val & AK09911_CNTL3_SRST) {
		ESP_LOGW(AK09911_TAG, "SRST bit did not auto-clear");
	}

	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Data Ready                                                                */
/* ------------------------------------------------------------------------- */
esp_err_t ak09911_data_ready(ak09911_handle_t *handle, bool *ready)
{
	uint8_t st1;

	if ((handle == NULL) || (ready == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	esp_err_t err = ak09911_reg_read(handle, AK09911_ST1, &st1, 1);
	if (err != ESP_OK) return err;

	*ready = (st1 & AK09911_ST1_DRDY) != 0;
	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Data Reading                                                              */
/* ------------------------------------------------------------------------- */
esp_err_t ak09911_data_read_raw(ak09911_handle_t *handle,
				ak09911_axes_raw_t *out,
				bool *overflow)
{
	uint8_t  buf[AK09911_BURST_LEN];
	uint32_t timeout = pdMS_TO_TICKS(1000);
	bool     ready;
	esp_err_t err;

	if ((handle == NULL) || (out == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	/* Block until DRDY with timeout */
	for (;;) {
		err = ak09911_data_ready(handle, &ready);
		if (err != ESP_OK) return err;
		if (ready) break;
		if (timeout == 0) return ESP_ERR_TIMEOUT;
		vTaskDelay(pdMS_TO_TICKS(1));
		if (timeout > 0) timeout--;
	}

	/* Burst-read 8 bytes: HXL..HZH + TMPS + ST2.
	 * AK09911 auto-increments 0x11→0x12→0x13→…→0x18 within this block. */
	err = ak09911_reg_read(handle, AK09911_HXL, buf, AK09911_BURST_LEN);
	if (err != ESP_OK) return err;

	/* Parse Little Endian two's complement — only lower 14 bits are valid */
	out->x = (int16_t)(((uint16_t)buf[0])  | ((uint16_t)buf[1] << 8));
	out->y = (int16_t)(((uint16_t)buf[2])  | ((uint16_t)buf[3] << 8));
	out->z = (int16_t)(((uint16_t)buf[4])  | ((uint16_t)buf[5] << 8));

	/* ST2 (buf[7]): overflow flag */
	if (overflow != NULL) {
		*overflow = (buf[7] & AK09911_ST2_HOFL) != 0;
	}

	return ESP_OK;
}

esp_err_t ak09911_data_read_ut(ak09911_handle_t *handle,
			       ak09911_axes_ut_t *out,
			       bool *overflow)
{
	ak09911_axes_raw_t raw;
	esp_err_t err;

	if ((handle == NULL) || (out == NULL)) {
		return ESP_ERR_INVALID_ARG;
	}

	err = ak09911_data_read_raw(handle, &raw, overflow);
	if (err != ESP_OK) return err;

	/* Apply sensitivity adjustment: Hadj = H × (((ASA - 128) * 0.5) / 128 + 1) */
	out->x = (float)raw.x * ((((float)handle->asax - 128.0f) * 0.5f) / AK09911_ASA_FACTOR_DEN + 1.0f)
		 * AK09911_SENSITIVITY_UT_PER_LSB;
	out->y = (float)raw.y * ((((float)handle->asay - 128.0f) * 0.5f) / AK09911_ASA_FACTOR_DEN + 1.0f)
		 * AK09911_SENSITIVITY_UT_PER_LSB;
	out->z = (float)raw.z * ((((float)handle->asaz - 128.0f) * 0.5f) / AK09911_ASA_FACTOR_DEN + 1.0f)
		 * AK09911_SENSITIVITY_UT_PER_LSB;

	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Sensitivity                                                               */
/* ------------------------------------------------------------------------- */
esp_err_t ak09911_sensitivity_get(ak09911_handle_t *handle,
				  uint8_t *asax,
				  uint8_t *asay,
				  uint8_t *asaz)
{
	if (handle == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	if (asax != NULL) *asax = handle->asax;
	if (asay != NULL) *asay = handle->asay;
	if (asaz != NULL) *asaz = handle->asaz;
	return ESP_OK;
}
