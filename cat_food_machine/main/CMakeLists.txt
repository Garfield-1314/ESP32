idf_component_register(
SRCS "main.c"

"driver/src/tusb_serial.c"

"device/src/st7789.c"
"device/src/gt911.c"
"device/src/user_lvgl.c"

"ui/src/ui.c"
"ui/src/app_page.c"
"ui/src/main_page.c"
"ui/src/setting_page.c"

                    INCLUDE_DIRS "."
                    REQUIRES esp_lcd esp_driver_i2c nvs_flash esp_timer
)