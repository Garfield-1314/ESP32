#include "include/user_lvgl.h"
#include "include/st7789.h"
#include "include/gt911.h"
#include "lvgl.h"

static const char *TAG = "user_lvgl";

void user_lvgl_init(void) {
    ESP_LOGI(TAG, "Initializing User LVGL");

    lv_init();
}
