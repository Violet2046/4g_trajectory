set(component_reqs "nvs_flash" "esp_timer")

# NimBLE headers are only available when BT+NimBLE is enabled in menuconfig.
# When disabled, ble_img_rx.c compiles stub versions of its public API.
if(CONFIG_BT_ENABLED AND CONFIG_BT_NIMBLE_ENABLED)
    list(APPEND component_reqs "nimble")
endif()

idf_component_register(SRCS "ble_img_rx.c"
		       INCLUDE_DIRS "include"
		       REQUIRES ${component_reqs})
