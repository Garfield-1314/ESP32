/**
 * @file main.c
 * @brief LSM6DSM example with optional LIS2MDL magnetometer (9-axis)
 *
 * Features:
 *   - Initialize LSM6DSM (accel + gyro)
 *   - If LSM6DSM_USE_MAGNETOMETER=1 in lsm6dsm.h, also initializes
 *     LIS2MDL magnetometer via Sensor Hub (9-axis mode)
 *   - Reads and prints all available axes + temperature
 *
 * Hardware:
 *   LSM6DSM: SDA=GPIO17, SCL=GPIO18
 *   LIS2MDL: Connected to LSM6DSM auxiliary I2C (internal)
 */

#include <stdio.h>
#include <math.h>

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

#if LSM6DSM_USE_MAGNETOMETER
    ESP_LOGI(TAG, "  9-Axis Mode (with LIS2MDL)");
#else
    ESP_LOGI(TAG, "  6-Axis Mode (no magnetometer)");
#endif

    ESP_LOGI(TAG, "=================================");

    /* Initialize USB CDC-ACM (virtual serial port) */
    tusb_serial_init();

    /* Initialize LSM6DSM (and LIS2MDL if enabled) */
    esp_err_t ret = lsm6dsm_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LSM6DSM initialization FAILED! err=%d", ret);
        ESP_LOGE(TAG, "Check your wiring and I2C pins in lsm6dsm.h");
        return;
    }

    lsm6dsm_all_data_t data;
    float temperature = 0.0f;
    int sample_count = 0;

    while (1) {
        ret = lsm6dsm_read_all(&data);
        if (ret == ESP_OK) {
            lsm6dsm_read_temperature(&temperature);
            sample_count++;

            if ((sample_count % 20) == 1) {
                ESP_LOGI(TAG, "");
                ESP_LOGI(TAG, "Sample #%d", sample_count);
                ESP_LOGI(TAG, "------------------------------------------------------------");
                ESP_LOGI(TAG, "  Sensor      |   X         Y         Z     |  Unit");
                ESP_LOGI(TAG, "------------------------------------------------------------");
            }

            ESP_LOGI(TAG, "  ACCEL       | %9.3f  %9.3f  %9.3f  |  g",
                     data.accel_x, data.accel_y, data.accel_z);
            ESP_LOGI(TAG, "  GYRO        | %9.2f  %9.2f  %9.2f  |  dps",
                     data.gyro_x, data.gyro_y, data.gyro_z);

#if LSM6DSM_USE_MAGNETOMETER
            ESP_LOGI(TAG, "  MAGNETOMETER| %9.2f  %9.2f  %9.2f  |  mGauss",
                     data.mag_x, data.mag_y, data.mag_z);

            /* Total magnetic field (should be ~250-650 mGauss) */
            float total_mag = sqrtf(data.mag_x * data.mag_x +
                                    data.mag_y * data.mag_y +
                                    data.mag_z * data.mag_z);
            ESP_LOGI(TAG, "  |B| = %.1f mGauss", total_mag);
#endif

            /* Total acceleration (should be ~1g at rest) */
            float total_accel = sqrtf(data.accel_x * data.accel_x +
                                      data.accel_y * data.accel_y +
                                      data.accel_z * data.accel_z);
            ESP_LOGI(TAG, "  |A| = %.3f g  |  Temp = %.2f C",
                     total_accel, temperature);
            ESP_LOGI(TAG, "------------------------------------------------------------");

        } else {
            ESP_LOGE(TAG, "Read data failed: %d", ret);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}