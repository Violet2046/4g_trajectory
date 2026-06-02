idf_component_register(SRCS "main.c" "app_utils.c" "storage_mgr.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_timer esp_driver_uart esp_driver_gptimer esp_driver_gpio esp_driver_spi CT511N sensor_hub W25Q64 ST7789 ble_img_rx wifi_cfg)
