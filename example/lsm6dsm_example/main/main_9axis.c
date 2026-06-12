/**
 * @file main_9axis.c
 * @brief Complete 9-axis demo using LSM6DSM Mode 2 + LIS2MDL
 *
 * This example demonstrates:
 *   1. Initializing LSM6DSM (accel + gyro)
 *   2. Configuring LIS2MDL magnetometer via LSM6DSM pass-through
 *   3. Setting up LSM6DSM Sensor Hub (Mode 2) to auto-read LIS2MDL
 *   4. Reading all 9 axes + temperature in a loop
 *
 * Hardware: ESP32 with LSM6DSMTR + LIS2MDLTR on shared I2C bus
 *           SDA=GPIO17, SCL=GPIO18
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lsm6dsm.h"
#include "lsm6dsm_9axis.h"
#include "tusb_serial.h"

static const char *TAG = "9AXIS_DEMO";

void app_main(void)
{
    ESP_LOGI(TAG, "===========================================");
    ESP_LOGI(TAG, "  9-Axis Sensor Fusion Demo");
    ESP_LOGI(TAG, "  LSM6DSMTR (Accel+Gyro) + LIS2MDLTR (Mag)");
    ESP_LOGI(TAG, "  Sensor Hub Mode 2");
    ESP_LOGI(TAG, "===========================================");

    /* Initialize USB CDC-ACM for serial output */
    tusb_serial_init();

    /* Small delay for USB enumeration */
    vTaskDelay(pdMS_TO_TICKS(500));

    /* Initialize 9-axis system */
    esp_err_t ret = lsm6dsm_9axis_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "9-Axis initialization FAILED! err=%d", ret);
        ESP_LOGE(TAG, "Check wiring:");
        ESP_LOGE(TAG, "  1. LSM6DSM SDA/SCL -> ESP17/ESP18");
        ESP_LOGE(TAG, "  2. LIS2MDL SDA/SCL -> same bus as LSM6DSM");
        ESP_LOGE(TAG, "  3. Both sensors have 3.3V power and pull-up resistors");
        ESP_LOGE(TAG, "  4. LSM6DSM CS pin -> 3.3V (I2C mode)");
        return;
    }

    lsm6dsm_9axis_data_t data;
    float temperature = 0.0f;
    int sample_count = 0;

    /* Main loop: read and display all 9 axes */
    while (1) {
        ret = lsm6dsm_9axis_read(&data);
        if (ret == ESP_OK) {
            /* Read temperature (optional) */
            lsm6dsm_read_temperature(&temperature);

            sample_count++;

            /* Print header every 20 samples */
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
            ESP_LOGI(TAG, "  MAGNETOMETER| %9.2f  %9.2f  %9.2f  |  mGauss",
                     data.mag_x, data.mag_y, data.mag_z);
            ESP_LOGI(TAG, "  Temperature: %.2f °C", temperature);

            /* Calculate total magnetic field strength (should be ~250-650 mGauss for Earth) */
            float total_mag = sqrtf(data.mag_x * data.mag_x +
                                    data.mag_y * data.mag_y +
                                    data.mag_z * data.mag_z);
            ESP_LOGI(TAG, "  |B| = %.1f mGauss (Earth field: ~250-650 mGauss)", total_mag);

            /* Calculate acceleration magnitude (should be ~1g at rest) */
            float total_accel = sqrtf(data.accel_x * data.accel_x +
                                      data.accel_y * data.accel_y +
                                      data.accel_z * data.accel_z);
            ESP_LOGI(TAG, "  |A| = %.3f g", total_accel);

            ESP_LOGI(TAG, "------------------------------------------------------------");

        } else {
            ESP_LOGE(TAG, "Read data failed: %d", ret);
        }

        /* ~10 Hz output rate */
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}