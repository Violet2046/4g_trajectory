idf_component_register(
    SRCS "CT511N.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_driver_uart esp_driver_gpio
)
