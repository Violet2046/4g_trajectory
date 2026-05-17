idf_component_register(SRCS "sensor_hub.c"
		       INCLUDE_DIRS "include"
		       REQUIRES "esp_timer" "esp_driver_i2c" "esp_driver_gpio" "BMI160" "AK09911C")
