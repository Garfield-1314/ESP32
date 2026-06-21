#include "include.h"
#include "driver/uart.h"
/* OTA pending flag — checked in lvgl_event_task */
static bool s_ota_pending = false;

static void nvs_component_init(void) {
  model_nvs_init();
  motor_setting_NVS_init();
  motor_PID_NVS_init();
  Mecanum_PID_NVS_init();
}

static void lvgl_component_init(void) {
  user_lvgl_init();
  create_ui();
}

static void rtos_event_init(void) {
  xTaskCreatePinnedToCore(lvgl_event_task, "lvgl_event_task", 8192, NULL, 5,
                          NULL, 1);  // LVGL任务，运行在核心1
  xTaskCreatePinnedToCore(motor_control_task, "motor_task", 4096, NULL, 10,
                          NULL, 0);  // 电机控制任务，运行在核心0
  xTaskCreatePinnedToCore(Mecanum_control_task, "mecanum_task", 4096, NULL, 10,
                          NULL, 0);  // 机械四轮控制任务，运行在核心0
  xTaskCreatePinnedToCore(Balance_Car_Task, "balance_task", 4096, NULL, 10,
                          NULL, 0);  // 平衡车控制任务，运行在核心0
  xTaskCreatePinnedToCore(imu_hal_read_task, "imu_read_task", 4096, NULL, 8,
                          NULL, 0);  // IMU读取任务，运行在核心0
}

static void user_component_init(void) {
  tusb_serial_init();
  vTaskDelay(pdMS_TO_TICKS(1000));

  nvs_component_init();
  app_espnow_init();
  motor_init();
  buzzer_init();
  Mecanum_Init();
  imu_hal_init();
  lvgl_component_init();
  rtos_event_init();

  /* Delay briefly so LVGL can finish its first frame render */
  vTaskDelay(pdMS_TO_TICKS(500));

  /* Check if car.bin exists — defer OTA dialog to LVGL task */
  if (ota_usb_check_firmware_exists()) {
    ESP_LOGI("main", "Firmware file car.bin detected on USB");
    s_ota_pending = true;
  } else {
    ESP_LOGD("main", "No OTA firmware found on USB");
  }

  buzzer_on();
  vTaskDelay(pdMS_TO_TICKS(200));
  buzzer_off();

  // debug_init();
}

void app_main(void) {
  user_component_init();

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(10));

    // const char *msg = "start\n";
    // uart_write_bytes(UART_NUM_0, msg, strlen(msg));
  }
}
