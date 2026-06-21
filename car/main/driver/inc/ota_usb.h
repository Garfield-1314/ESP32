#pragma once

#include <stdbool.h>
#include "esp_err.h"

/**
 * @brief Check if car.bin exists on the USB drive
 * @return true if /usb/car.bin exists and has non-zero size
 */
bool ota_usb_check_firmware_exists(void);

/**
 * @brief Perform OTA update from /usb/car.bin (requires FAT filesystem mounted at /usb)
 * Writes to the inactive OTA partition, sets boot partition, and reboots
 * @return ESP_OK on success (does not return, reboots), error otherwise
 */
esp_err_t ota_usb_perform_update(void);

/**
 * @brief Delete /usb/car.bin from the USB drive
 * Call this when user cancels the upgrade
 */
void ota_usb_remove_firmware(void);