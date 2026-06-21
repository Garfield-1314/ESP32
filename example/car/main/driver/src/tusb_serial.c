/*
 * SPDX-FileCopyrightText: 2022-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 *
 * USB TinyUSB serial driver init.
 * CDC ACM is used for USB serial communication with the host (OpenMV).
 */

#include "driver/inc/tusb_serial.h"

#include <string.h>
#include <stdarg.h>

#include "Command/inc/command.h"
#include "esp_log.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"

static const char *TAG = "tusb_serial";

/**
 * @brief CDC RX callback — feed received data into the command processor
 */
static void cdc_rx_cb(int itf, cdcacm_event_t *event)
{
    (void)event;
    static uint8_t buf[64];
    size_t rx_size = 0;

    esp_err_t ret = tinyusb_cdcacm_read((tinyusb_cdcacm_itf_t)itf, buf, sizeof(buf), &rx_size);
    if (ret == ESP_OK && rx_size > 0) {
        command_receive(buf, rx_size);
    }
}

/**
 * @brief Custom vprintf hook — redirect ESP log output to CDC ACM
 */
static int cdc_vprintf(const char *fmt, va_list args) {
    char buf[256];
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    if (len > 0) {
        tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0, (const uint8_t *)buf,
                                   len < (int)sizeof(buf) ? len : sizeof(buf) - 1);
        tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
    }
    return len;
}

void tusb_serial_init(void) {
  command_init();

  ESP_LOGI(TAG, "USB initialization (CDC ACM enabled)");

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
#endif  // TUD_OPT_HIGH_SPEED
  };

  ESP_ERROR_CHECK(tinyusb_driver_install(&tusb_cfg));

  /* Initialize CDC-ACM */
  tinyusb_config_cdcacm_t acm_cfg = {
    .usb_dev = TINYUSB_USBDEV_0,
    .cdc_port = TINYUSB_CDC_ACM_0,
    .callback_rx = cdc_rx_cb,
    .callback_rx_wanted_char = NULL,
    .callback_line_state_changed = NULL,
    .callback_line_coding_changed = NULL,
  };

  ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

  /* Redirect ESP log output (ESP_LOGI, ESP_LOGW, etc.) to CDC ACM */
  esp_log_set_vprintf(cdc_vprintf);

  ESP_LOGI(TAG, "tusb_serial initialized (CDC ACM active, log via USB)");
}
