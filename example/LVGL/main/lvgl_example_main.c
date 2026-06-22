/*
 * SPDX-FileCopyrightText: 2026 Garfield-1314
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"

#include "device/inc/user_lvgl.h"
#include "driver/inc/tusb_serial.h"
#include "ui/inc/ui.h"

static const char *TAG = "lvgl_example";

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
    ESP_LOGI(TAG, "LVGL Example starting...");

    /* Initialize TinyUSB CDC (debug output via USB serial) */
    tusb_serial_init();

    /* Allow USB to enumerate */
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* Initialize LCD, touch, and LVGL */
    user_lvgl_init();

    /* Create and load the UI */
    create_ui();

    /* Create LVGL event task */
    xTaskCreate(lvgl_event_task, "lvgl_event_task", 4096, NULL, 5, NULL);

    ESP_LOGI(TAG, "LVGL Example initialized successfully");

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}