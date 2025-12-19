#include "include/user_lvgl.h"
#include "include/st7789.h"
#include "include/gt911.h"
#include "lvgl.h"
#include "esp_timer.h"

static const char *TAG = "user_lvgl";

static void lv_tick_task(void *arg) {
    (void) arg;
    lv_tick_inc(1);
}

void user_lvgl_init(void) {
    ESP_LOGI(TAG, "Initializing User LVGL");

    lv_init();

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "periodic_gui"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000));
}
