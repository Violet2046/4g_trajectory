idf_component_register(SRCS "wifi_cfg.c"
                    INCLUDE_DIRS "include" "../../main"
                    REQUIRES esp_netif esp_wifi esp_http_server esp_event esp_driver_gpio esp_driver_spi esp_driver_uart W25Q64)
