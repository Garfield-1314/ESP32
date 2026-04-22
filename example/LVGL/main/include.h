#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"


#include "driver/inc/tusb_serial.h"
#include "driver/inc/user_lvgl.h"
#include "driver/inc/st7789.h"
#include "driver/inc/gt911.h"

#include "ui/inc/ui.h"
#include "ui/inc/main_page.h"
#include "ui/inc/app_page.h"