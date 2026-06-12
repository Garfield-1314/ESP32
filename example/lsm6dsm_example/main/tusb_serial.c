/**
 * @file tusb_serial.c
 * @brief USB CDC-ACM serial initialization for ESP-IDF
 *
 * Initializes TinyUSB CDC ACM as a virtual serial port (USB-to-UART bridge)
 * and redirects ESP-IDF log output (ESP_LOGI, etc.) to it.
 *
 * Hardware requirements:
 *   - ESP chip with USB-OTG peripheral (ESP32-S2, S3, P4, etc.)
 *   - USB D+ (GPIO_DP) and D- (GPIO_DM) connected to a USB port
 */

#include "tusb_serial.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "tinyusb.h"
#include "tusb_cdc_acm.h"

static const char *TAG = "tusb_serial";

/**
 * @brief Custom vprintf hook — redirect ESP log output to CDC ACM
 */
static int cdc_vprintf(const char *fmt, va_list args)
{
    char buf[256];
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    if (len > 0) {
        tinyusb_cdcacm_write_queue(TINYUSB_CDC_ACM_0,
                                   (const uint8_t *)buf,
                                   len < (int)sizeof(buf) ? len : sizeof(buf) - 1);
        tinyusb_cdcacm_write_flush(TINYUSB_CDC_ACM_0, 0);
    }
    return len;
}

void tusb_serial_init(void)
{
    ESP_LOGI(TAG, "Initializing USB CDC ACM (virtual serial port)...");

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

    /* Initialize CDC-ACM */
    tinyusb_config_cdcacm_t acm_cfg = {
        .usb_dev = TINYUSB_USBDEV_0,
        .cdc_port = TINYUSB_CDC_ACM_0,
        .callback_rx = NULL,           /* No command processor needed */
        .callback_rx_wanted_char = NULL,
        .callback_line_state_changed = NULL,
        .callback_line_coding_changed = NULL,
    };

    ESP_ERROR_CHECK(tusb_cdc_acm_init(&acm_cfg));

    /* Redirect ESP log output to CDC ACM */
    esp_log_set_vprintf(cdc_vprintf);

    ESP_LOGI(TAG, "USB CDC ACM ready — log output via USB serial");
}