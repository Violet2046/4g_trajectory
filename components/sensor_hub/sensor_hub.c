#include "sensor_hub.h"

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define HUB_TAG "sensor_hub"

/* Default AK09911C addresses if config doesn't specify */
static const uint16_t default_ak09911_addrs[] = {0x0C, 0x0D};
#define DEFAULT_AK09911_ADDR_COUNT 2

struct sensor_hub_t {
	sensor_hub_config_t  config;
	i2c_master_bus_handle_t  bus;

	bmi160_handle_t  *bmi160;
	ak09911_handle_t *ak09911;

	bool double_tap_enabled;  /* INT2 double-tap configured? */
};

/* ------------------------------------------------------------------------- */
/*  Init                                                                     */
/* ------------------------------------------------------------------------- */
esp_err_t sensor_hub_init(sensor_hub_t **out_hub,
			  const sensor_hub_config_t *config)
{
	if (out_hub == NULL || config == NULL) {
		return ESP_ERR_INVALID_ARG;
	}

	sensor_hub_t *hub = (sensor_hub_t *)calloc(1, sizeof(*hub));
	if (hub == NULL) return ESP_ERR_NO_MEM;

	hub->config = *config;

	/* Use default AK09911C addresses if none provided */
	const uint16_t *mag_addrs = config->ak09911_addrs;
	int mag_count = config->ak09911_addr_count;
	if (mag_addrs == NULL || mag_count == 0) {
		mag_addrs = default_ak09911_addrs;
		mag_count = DEFAULT_AK09911_ADDR_COUNT;
	}

	/* ---- I2C bus recovery (warm-reset) ---- */
	{
		/* On warm reset, a slave (BMI160/AK09911C) may hold SDA low from an
		 * interrupted transaction.  Manually clock SCL to
		 * release the bus before the I2C driver takes over. */
		gpio_config_t io_conf = {
			.pin_bit_mask = (1ULL << config->sda_io_num) | (1ULL << config->scl_io_num),
			.mode         = GPIO_MODE_INPUT_OUTPUT_OD,
			.pull_up_en   = GPIO_PULLUP_ENABLE,
			.pull_down_en = GPIO_PULLDOWN_DISABLE,
			.intr_type    = GPIO_INTR_DISABLE,
		};
		gpio_config(&io_conf);
		
		/* Let SDA float high, we will clock SCL */
		gpio_set_level(config->sda_io_num, 1);
		gpio_set_level(config->scl_io_num, 1);
		esp_rom_delay_us(10);

		for (int i = 0; i < 9; i++) {
			/* If SDA is high, the bus is free, we can stop clocking */
			if (gpio_get_level(config->sda_io_num)) {
				break;
			}
			gpio_set_level(config->scl_io_num, 0);
			esp_rom_delay_us(10);
			gpio_set_level(config->scl_io_num, 1);
			esp_rom_delay_us(10);
		}
		
		/* Generate a STOP condition: SDA low→high while SCL is high */
		gpio_set_level(config->scl_io_num, 0);
		esp_rom_delay_us(10);
		gpio_set_level(config->sda_io_num, 0);
		esp_rom_delay_us(10);
		gpio_set_level(config->scl_io_num, 1);
		esp_rom_delay_us(10);
		gpio_set_level(config->sda_io_num, 1);
		esp_rom_delay_us(10);
		
		/* Reset pins to default state so i2c_new_master_bus can take over */
		gpio_reset_pin(config->sda_io_num);
		gpio_reset_pin(config->scl_io_num);
	}

	/* ---- Create shared I2C bus ---- */
	i2c_master_bus_config_t bus_cfg = {
		.i2c_port = -1,
		.sda_io_num = config->sda_io_num,
		.scl_io_num = config->scl_io_num,
		.clk_source = I2C_CLK_SRC_DEFAULT,
		.glitch_ignore_cnt = 7,
		.flags = { .enable_internal_pullup = true },
	};

	esp_err_t err = i2c_new_master_bus(&bus_cfg, &hub->bus);
	if (err != ESP_OK) {
		ESP_LOGE(HUB_TAG, "i2c_new_master_bus failed: %s",
			 esp_err_to_name(err));
		free(hub);
		return err;
	}

	ESP_LOGI(HUB_TAG, "I2C bus created SDA=%d SCL=%d",
		 config->sda_io_num, config->scl_io_num);

	/* ---- BMI160 auto-detect with warm-reset retry (try 0x68, 0x69) ---- */
	const uint16_t bmi_addrs[] = {0x68, 0x69};
	int bmi_found = -1;

	for (int attempt = 0; attempt < 3 && bmi_found < 0; attempt++) {
		if (attempt > 0) {
			ESP_LOGW(HUB_TAG, "BMI160 probe retry %d/3", attempt + 1);
			vTaskDelay(pdMS_TO_TICKS(200));
		}

		for (int i = 0; i < 2; i++) {
			bmi160_i2c_config_t bmi_cfg = {
				.bus_handle  = hub->bus,
				.dev_addr    = bmi_addrs[i],
				.i2c_freq_hz = 100000,
				.int1_pin    = config->bmi160_int1_pin,
				.int1_type   = config->bmi160_int1_type,
				.int2_pin    = config->bmi160_int2_pin,
				.int2_type   = config->bmi160_int2_type,
			};
			err = bmi160_init(&hub->bmi160, &bmi_cfg);
			if (err == ESP_OK) {
				bmi_found = i;
				break;
			}
			ESP_LOGW(HUB_TAG, "BMI160 not at 0x%02X: %s",
				 bmi_addrs[i], esp_err_to_name(err));
		}
	}

	if (bmi_found < 0) {
		ESP_LOGE(HUB_TAG, "BMI160 not found — check I2C wiring");
		i2c_del_master_bus(hub->bus);
		free(hub);
		return ESP_ERR_NOT_FOUND;
	}

	ESP_LOGI(HUB_TAG, "BMI160 ready at 0x%02X", bmi_addrs[bmi_found]);

	/* Configure any-motion interrupt (→ INT1) */
	bmi160_any_motion_configure(hub->bmi160, 0x06, 0x01);

	/* Configure double-tap interrupt (→ INT2) if pin is assigned */
	hub->double_tap_enabled = false;
	if (config->bmi160_int2_pin != GPIO_NUM_NC &&
	    config->bmi160_int2_type != GPIO_INTR_DISABLE) {
		esp_err_t dt_err = bmi160_double_tap_configure(hub->bmi160);
		if (dt_err == ESP_OK) {
			hub->double_tap_enabled = true;
			ESP_LOGI(HUB_TAG, "double-tap on INT2 (GPIO %d)",
				 config->bmi160_int2_pin);
		} else {
			ESP_LOGW(HUB_TAG, "double-tap config: %s",
				 esp_err_to_name(dt_err));
		}
	}

	/* ---- AK09911C auto-detect ---- */
	int mag_found = -1;
	for (int i = 0; i < mag_count; i++) {
		ak09911_i2c_config_t mag_cfg = {
			.bus_handle  = hub->bus,
			.dev_addr    = mag_addrs[i],
			.i2c_freq_hz = 100000,
		};
		err = ak09911_init(&hub->ak09911, &mag_cfg);
		if (err == ESP_OK) {
			mag_found = i;
			break;
		}
		ESP_LOGW(HUB_TAG, "AK09911C not at 0x%02X: %s",
			 mag_addrs[i], esp_err_to_name(err));
	}

	if (mag_found < 0) {
		ESP_LOGW(HUB_TAG, "AK09911C not found — continuing without magnetometer");
		hub->ak09911 = NULL;
	} else {
		ak09911_mode_set(hub->ak09911, AK09911_MODE_CONT_1);
		ESP_LOGI(HUB_TAG, "AK09911C ready at 0x%02X — 10Hz continuous",
			 mag_addrs[mag_found]);
	}

	*out_hub = hub;
	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Destroy                                                                  */
/* ------------------------------------------------------------------------- */
void sensor_hub_destroy(sensor_hub_t *hub)
{
	if (hub == NULL) return;

	if (hub->ak09911 != NULL) {
		ak09911_destroy(hub->ak09911);
	}
	if (hub->bmi160 != NULL) {
		bmi160_destroy(hub->bmi160);
	}
	if (hub->bus != NULL) {
		i2c_del_master_bus(hub->bus);
	}
	free(hub);
}

/* ------------------------------------------------------------------------- */
/*  Sample                                                                   */
/* ------------------------------------------------------------------------- */
esp_err_t sensor_hub_sample(sensor_hub_t *hub, sensor_sample_t *sample)
{
	if (hub == NULL || sample == NULL) return ESP_ERR_INVALID_ARG;

	memset(sample, 0, sizeof(*sample));
	sample->timestamp = (uint32_t)(esp_timer_get_time() / 1000000ULL);

	/* BMI160 */
	if (hub->bmi160 != NULL) {
		bmi160_acc_read_g(hub->bmi160, BMI160_ACC_RANGE_8G_VAL,
				  &sample->acc);
		bmi160_gyr_read_dps(hub->bmi160, BMI160_GYR_RANGE_500_VAL,
				    &sample->gyr);
	}

	/* AK09911C (non-blocking, skip if not ready) */
	if (hub->ak09911 != NULL) {
		bool ready = false;
		ak09911_data_ready(hub->ak09911, &ready);
		if (ready) {
			bool overflow = false;
			ak09911_data_read_ut(hub->ak09911, &sample->mag,
					     &overflow);
			if (!overflow) {
				sample->mag_valid = true;
			}
		}
	}

	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Power management                                                         */
/* ------------------------------------------------------------------------- */
esp_err_t sensor_hub_sleep(sensor_hub_t *hub)
{
	if (hub == NULL) return ESP_ERR_INVALID_ARG;

	if (hub->bmi160 != NULL) {
		esp_err_t err;
		err = bmi160_gyr_set_mode(hub->bmi160, BMI160_MODE_SUSPEND);
		if (err != ESP_OK) {
			ESP_LOGE(HUB_TAG, "gyr suspend failed: %s", esp_err_to_name(err));
			return err;
		}
		err = bmi160_acc_set_mode(hub->bmi160, BMI160_MODE_LOW_POWER);
		if (err != ESP_OK) {
			ESP_LOGE(HUB_TAG, "acc low-power failed: %s", esp_err_to_name(err));
			return err;
		}
		/* Re-apply any-motion + double-tap configs */
		err = bmi160_any_motion_configure(hub->bmi160, 0x06, 0x01);
		if (err != ESP_OK) {
			ESP_LOGE(HUB_TAG, "any-motion re-arm failed: %s", esp_err_to_name(err));
			return err;
		}
		uint8_t int_map_1 = hub->double_tap_enabled
				    ? BMI160_INT_MAP_DOUBLE_TAP
				    : 0x00;
		err = bmi160_int_map_set(hub->bmi160, 0x04, int_map_1, 0x00);
		if (err != ESP_OK) {
			ESP_LOGE(HUB_TAG, "int-map set failed: %s", esp_err_to_name(err));
			return err;
		}
		ESP_LOGI(HUB_TAG, "BMI160 low-power + any-motion re-armed");
	}

	if (hub->ak09911 != NULL) {
		ak09911_mode_set(hub->ak09911, AK09911_MODE_POWERDOWN);
	}

	return ESP_OK;
}

esp_err_t sensor_hub_wake(sensor_hub_t *hub)
{
	if (hub == NULL) return ESP_ERR_INVALID_ARG;

	if (hub->bmi160 != NULL) {
		bmi160_acc_set_mode(hub->bmi160, BMI160_MODE_NORMAL);
		bmi160_gyr_set_mode(hub->bmi160, BMI160_MODE_NORMAL);
	}

	if (hub->ak09911 != NULL) {
		ak09911_mode_set(hub->ak09911, AK09911_MODE_CONT_1);
	}

	return ESP_OK;
}

/* ------------------------------------------------------------------------- */
/*  Accessors                                                                */
/* ------------------------------------------------------------------------- */
bmi160_handle_t *sensor_hub_get_bmi160(sensor_hub_t *hub)
{
	return hub ? hub->bmi160 : NULL;
}

ak09911_handle_t *sensor_hub_get_ak09911(sensor_hub_t *hub)
{
	return hub ? hub->ak09911 : NULL;
}

i2c_master_bus_handle_t sensor_hub_get_bus(sensor_hub_t *hub)
{
	return hub ? hub->bus : NULL;
}

bool sensor_hub_has_bmi160(const sensor_hub_t *hub)
{
	return hub && hub->bmi160 != NULL;
}

bool sensor_hub_has_ak09911(const sensor_hub_t *hub)
{
	return hub && hub->ak09911 != NULL;
}
