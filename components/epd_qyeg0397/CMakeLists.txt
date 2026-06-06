idf_component_register(SRCS "epd_qyeg0397.c" "epd_font.c"
                       INCLUDE_DIRS "include"
                       REQUIRES "esp_driver_spi" "esp_driver_gpio" "esp_timer")
