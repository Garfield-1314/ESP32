idf_component_register(SRCS "lvgl_example_main.c"
                            "device/src/st7789.c"
                            "device/src/gt911.c"
                            "device/src/user_lvgl.c"
                            "driver/src/tusb_serial.c"
                            "ui/src/ui.c"
                            "ui/src/app_page.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_lcd esp_driver_i2c lvgl/lvgl espressif/esp_tinyusb
                       )