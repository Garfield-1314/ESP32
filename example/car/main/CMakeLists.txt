idf_component_register(

SRCS "main.c" 

"device/src/st7789.c"
"device/src/gt911.c"
"driver/src/tusb_serial.c"
"driver/src/ota_usb.c"
"driver/src/pwm.c"
"driver/src/gpio_out.c"
"driver/src/nvs.c"
"driver/src/espnow.c"

"device/src/user_lvgl.c"
"device/src/encoder.c"
"device/src/motor.c"
"device/src/icm42688.c"
"device/src/mpu6050.c"
"device/src/lsm6dsm.c"
"device/src/imu_hal.c"
"device/src/buzzer.c"

"ui/src/ui.c"
"ui/src/main_page.c"
"ui/src/app_page.c"
"ui/src/motor_page.c"
"ui/src/keyboard.c"
"ui/src/mecanum_page.c"
"ui/src/pid_page.c"
"ui/src/num_show_page.c"
"ui/src/motor_setting_page.c"
"ui/src/motor_debug.c"
"ui/src/car_control_page.c"
"ui/src/setting_page.c"
"ui/src/openmv_logo.c"
"ui/src/singtown_logo.c"
"font/gcxl_large_font_80.c"
"ui/src/imu_setting_page.c"
"ui/src/cmd_page.c"
"ui/src/ota_page.c"

"Algorithm/src/pid.c"
"Algorithm/src/Mecanum.c"
"Algorithm/src/imu_fusion.c"
"Algorithm/src/Balance_Attitude.c"
"Algorithm/src/Balance_Car.c"

"Command/src/command.c"

                    INCLUDE_DIRS "."
                    REQUIRES app_update esp_partition esp_driver_ledc esp_driver_pcnt esp_lcd esp_wifi esp_driver_i2c nvs_flash esp_timer
)