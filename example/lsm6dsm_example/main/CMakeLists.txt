idf_component_register(SRCS "lsm6dsm.c" "tusb_serial.c" "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_tinyusb)