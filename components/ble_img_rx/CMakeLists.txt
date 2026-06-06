set(component_reqs "nvs_flash" "esp_timer" "main")
set(extra_includes "")

# NimBLE is part of the 'bt' component in ESP-IDF v6.0.
# When BT+NimBLE is enabled, we must manually add NimBLE headers because
# user components do not receive bt's transitive INCLUDE_DIRS.
if(CONFIG_BT_ENABLED AND CONFIG_BT_NIMBLE_ENABLED)
    list(APPEND component_reqs "bt")
    # All paths relative to ${IDF_PATH}/components/bt/
    set(nimble_root "${IDF_PATH}/components/bt")
    set(extra_includes
        # BT controller API (esp_bt.h)
        "${nimble_root}/include/esp32c3/include"
        # NimBLE host core
        "${nimble_root}/host/nimble/nimble/nimble/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/util/include"
        "${nimble_root}/host/nimble/nimble/nimble/transport/include"
        # NimBLE porting layer
        "${nimble_root}/host/nimble/nimble/porting/nimble/include"
        "${nimble_root}/host/nimble/nimble/porting/npl/freertos/include"
        "${nimble_root}/host/nimble/port/include"
        "${nimble_root}/host/nimble/esp-hci/include"
        # NimBLE store
        "${nimble_root}/host/nimble/nimble/nimble/host/store/ram/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/store/config/include"
        # NimBLE GATT services
        "${nimble_root}/host/nimble/nimble/nimble/host/services/gap/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/gatt/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/ans/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/bas/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/dis/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/hr/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/htp/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/ias/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/ipss/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/lls/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/prox/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/cts/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/tps/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/hid/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/sps/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/cte/include"
        "${nimble_root}/host/nimble/nimble/nimble/host/services/ras/include"
    )
endif()

idf_component_register(SRCS "ble_img_rx.c"
                       INCLUDE_DIRS "include" ${extra_includes}
                       REQUIRES ${component_reqs})

# Force the 'bt' component to be linked with --whole-archive.
# This ensures NimBLE host objects are always included even though
# libbt.a may appear before ble_img_rx in the linker command line.
# Without this, GNU ld skips NimBLE members in the first occurrence
# of libbt.a and does not revisit them in later occurrences.
if(CONFIG_BT_ENABLED AND CONFIG_BT_NIMBLE_ENABLED)
    # Target name: ___idf_bt (component alias "idf::bt" → "idf_bt" → "___idf_bt")
    set_property(TARGET ___idf_bt PROPERTY WHOLE_ARCHIVE TRUE)
endif()
