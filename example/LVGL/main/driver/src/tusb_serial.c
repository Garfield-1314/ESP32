/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include <stdint.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"
#include "tusb_console.h"
#include "sdkconfig.h"
#include "driver/inc/tusb_serial.h"

static const char *TAG = "tusb_serial";
static uint8_t rx_buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];

static QueueHandle_t app_queue;
typedef struct {
  uint8_t buf[CONFIG_TINYUSB_CDC_RX_BUFSIZE + 1];
  size_t buf_len;
  uint8_t itf;
} app_message_t;

void tinyusb_cdc_rx_callback(int itf, cdcacm_event_t *event)
{
  size_t rx_size = 0;
  esp_err_t ret = tinyusb_cdcacm_read(itf, rx_buf, CONFIG_TINYUSB_CDC_RX_BUFSIZE, &rx_size);
  if (ret == ESP_OK) {
    app_message_t tx_msg = { .buf_len = rx_size, .itf = itf };
    memcpy(tx_msg.buf, rx_buf, rx_size);
    xQueueSend(app_queue, &tx_msg, 0);
  } else {
    ESP_LOGE(TAG, "Read Error");
  }
}

void tinyusb_cdc_line_state_changed_callback(int itf, cdcacm_event_t *event)
{
  int dtr = event->line_state_changed_data.dtr;
  int rts = event->line_state_changed_data.rts;
  ESP_LOGI(TAG, "Line state changed on channel %d: DTR:%d, RTS:%d", itf, dtr, rts);
}

static void tusb_serial_task(void *arg)
{
  app_message_t msg;
  while (1) {
    if (xQueueReceive(app_queue, &msg, portMAX_DELAY)) {
      if (msg.buf_len) {
        ESP_LOGI(TAG, "Data from channel %d:", msg.itf);
        ESP_LOG_BUFFER_HEXDUMP(TAG, msg.buf, msg.buf_len, ESP_LOG_INFO);
        tinyusb_cdcacm_write_queue(msg.itf, msg.buf, msg.buf_len);
        tinyusb_cdcacm_write_flush(msg.itf, 0);
      }
    }
  }
}

void tusb_serial_init(void)
{
  app_queue = xQueueCreate(5, sizeof(app_message_t));
  assert(app_queue);

  ESP_LOGI(TAG, "USB initialization");
  const tinyusb_config_t tusb_cfg = {
      .device_descriptor = NULL,
      .string_descriptor = NULL,
      .external_phy = false,
#if (TUD_OPT_HIGH_SPEED)
      .fs_configuration_descriptor = NULL,
      .hs_configuration_descriptor = NULL,
      .qualifier_descriptor = NULL,
#else
      .configuration_descriptor = NULL,
#endif
  };

  ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

  tinyusb_config_cdcacm_t acm_cfg = {
      .usb_dev = TINYUSB_USBDEV_0,
      .cdc_port = TINYUSB_CDC_ACM_0,
      .rx_unread_buf_sz = 64,
      .callback_rx = &tinyusb_cdc_rx_callback,
      .callback_rx_wanted_char = NULL,
      .callback_line_state_changed = NULL,
      .callback_line_coding_changed = NULL};

  ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));
  ESP_ERROR_CHECK(tinyusb_cdcacm_register_callback(
      TINYUSB_CDC_ACM_0, CDC_EVENT_LINE_STATE_CHANGED,
      &tinyusb_cdc_line_state_changed_callback));

  ESP_ERROR_CHECK(esp_tusb_init_console(TINYUSB_CDC_ACM_0));
  ESP_LOGI(TAG, "USB initialization DONE");

  xTaskCreate(tusb_serial_task, "tusb_serial_task", 4096, NULL, 5, NULL);
}