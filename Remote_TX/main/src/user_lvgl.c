#include "include/user_lvgl.h"
#include "include/st7789.h"
#include "include/gt911.h"
#include "lvgl.h"
#include "esp_timer.h"

static const char *TAG = "lvgl";

void user_lvgl_init(void) {

    lv_init();
}
