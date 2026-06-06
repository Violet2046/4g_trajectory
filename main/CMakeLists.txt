set(main_reqs esp_timer esp_driver_uart esp_driver_gptimer esp_driver_gpio esp_driver_spi nvs_flash CT511N sensor_hub W25Q64 wifi_cfg epd_qyeg0397 ble_img_rx)

# NimBLE host is part of 'bt' component; explicitly require it so the
# linker pulls in NimBLE when ble_img_rx is compiled with BLE support.
if(CONFIG_BT_ENABLED AND CONFIG_BT_NIMBLE_ENABLED)
    list(APPEND main_reqs bt)
endif()

idf_component_register(SRCS "main.c" "app_utils.c" "storage_mgr.c"
                    INCLUDE_DIRS "."
                    REQUIRES ${main_reqs})
