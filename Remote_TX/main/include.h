#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"


#include "include/tusb_serial.h"
#include "include/user_lvgl.h"
#include "include/st7789.h"
#include "include/gt911.h"
#include "include/ui.h"