idf_component_register(SRCS "lsm6dsm.c" "tusb_serial.c" "lsm6dsm_9axis.c" "main_9axis.c"
                    INCLUDE_DIRS "."
                    REQUIRES driver esp_tinyusb)