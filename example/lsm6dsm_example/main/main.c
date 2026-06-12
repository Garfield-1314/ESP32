/**
 * @file main.c
 * @brief LSM6DSM example for ESP-IDF
 *
 * This example demonstrates:
 *   1. Initializing the LSM6DSM on I2C bus
 *   2. Reading 6-axis data (accelerometer + gyroscope)
 *   3. Printing data over serial
 *
 * To use on a different ESP device:
 *   - Edit LSM6DSM_I2C_PORT, LSM6DSM_PIN_SDA, LSM6DSM_PIN_SCL in lsm6dsm.h
 */

#include <stdio.h>
#include <stdlib.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lsm6dsm.h"
#include "tusb_serial.h"

static const char *TAG = "example";

void app_main(void)
{
    ESP_LOGI(TAG, "=================================");
    ESP_LOGI(TAG, "  LSM6DSM Example - ESP-IDF");
    ESP_LOGI(TAG, "=================================");

    /* 0. Initialize USB CDC-ACM (virtual serial port) for debugging */
    tusb_serial_init();

    /* 1. Initialize LSM6DSM */
    esp_err_t ret = lsm6dsm_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LSM6DSM initialization FAILED! err=%d", ret);
        ESP_LOGE(TAG, "Check your wiring and I2C pins in lsm6dsm.h");
        return;
    }

    /* 2. (Optional) Change range/ODR if desired.
     *    Default is: ±2g accel, ±2000dps gyro, 416Hz ODR for both.
     *    Example: ±4g accel, ±500dps gyro, 208Hz ODR.
     *
     *    lsm6dsm_set_range(LSM6DSM_AFS_4G, LSM6DSM_AODR_208HZ,
     *                       LSM6DSM_GFS_500DPS, LSM6DSM_GODR_208HZ);
     */

    float accel[3], gyro[3];

    /* 3. Main loop: read and print data */
    while (1) {
        ret = lsm6dsm_read_data(accel, gyro);
        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "ACCEL: X=%7.3f g  Y=%7.3f g  Z=%7.3f g  |  "
                          "GYRO: X=%8.2f dps  Y=%8.2f dps  Z=%8.2f dps",
                     accel[0], accel[1], accel[2],
                     gyro[0], gyro[1], gyro[2]);
        } else {
            ESP_LOGE(TAG, "Read data failed: %d", ret);
        }

        /* Delay ~100ms between reads (~10 Hz output rate) */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}