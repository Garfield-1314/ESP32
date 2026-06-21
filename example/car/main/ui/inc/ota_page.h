#pragma once

#include "lvgl.h"

/**
 * @brief Show the OTA firmware upgrade confirmation dialog
 * Displays an LVGL message box asking user to Upgrade or Cancel
 */
void ota_show_dialog(void);