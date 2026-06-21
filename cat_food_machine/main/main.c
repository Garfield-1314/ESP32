#include "include.h"

static const char *TAG = "main";

/* WiFi 连接成功后的回调 */
static void on_wifi_connected(void)
{
    ESP_LOGI(TAG, "WiFi connected, starting SNTP time sync...");
    sntp_time_init();
}

void user_component_init(void)
{
    tusb_serial_init();
    vTaskDelay(pdMS_TO_TICKS(1000));
    user_lvgl_init();
    create_ui();

    /* 初始化 WiFi (自动尝试连接上一次保存的网络) */
    wifi_app_init();

    /* 注册 WiFi 连接成功回调（连接后自动同步时间） */
    wifi_app_register_connected_cb(on_wifi_connected);

    /* 如果 WiFi 已连接（从 NVS 恢复），直接初始化 SNTP */
    if (wifi_app_is_connected()) {
        sntp_time_init();
    }
}

void lvgl_event_task(void *arg)
{
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(20);  // 20ms => 50Hz

    for (;;) {
        lv_timer_handler();
        lv_tick_inc(20);

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void app_main(void)
{
    user_component_init();

    xTaskCreate(lvgl_event_task, "lvgl_event_task", 4096, NULL, 5, NULL);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}