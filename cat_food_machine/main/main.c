#include "include.h"

void user_component_init(void)
{
  tusb_serial_init();
  vTaskDelay(pdMS_TO_TICKS(1000));
  user_lvgl_init();
  create_ui();
}

void lvgl_event_task(void *arg)
{
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency = pdMS_TO_TICKS(20);  // 20ms => 50Hz

  for (;;) {
    lv_timer_handler();
    lv_tick_inc(20);

    vTaskDelay(pdMS_TO_TICKS(20));
  }
  vTaskDelayUntil(&xLastWakeTime, xFrequency);
}

void app_main(void)
{
  user_component_init();

  xTaskCreate(lvgl_event_task, "lvgl_event_task", 4096, NULL, 5, NULL);

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}