#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "driver/inc/tusb_serial.h"
#include "device/inc/user_lvgl.h"
#include "device/inc/st7789.h"
#include "device/inc/gt911.h"

#include "ui/inc/ui.h"