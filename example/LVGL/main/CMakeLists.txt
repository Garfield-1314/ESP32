idf_component_register(SRCS "main.c" 
"driver/src/st7789.c"
"driver/src/gt911.c"
"driver/src/tusb_serial.c"
"driver/src/user_lvgl.c"

"ui/src/ui.c"
"ui/src/main_page.c"
"ui/src/app_page.c"

                    INCLUDE_DIRS "."
                    REQUIRES 
)
