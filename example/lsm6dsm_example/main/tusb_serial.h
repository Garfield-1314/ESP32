#pragma once

/**
 * @brief Initialize USB CDC-ACM (virtual serial port) for debugging output.
 *        Redirects ESP-IDF log output (ESP_LOGI, etc.) to the USB serial port.
 */
void tusb_serial_init(void);