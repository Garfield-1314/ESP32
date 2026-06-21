#ifndef _IMU_HAL_H_
#define _IMU_HAL_H_

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Supported IMU types
typedef enum {
    IMU_TYPE_ICM42688 = 0,
    IMU_TYPE_MPU6050 = 1,
    IMU_TYPE_LSM6DSM = 2,
} imu_type_t;

#define IMU_TYPE_INVALID (-1)

// Unified IMU data structure (fields identical for both sensors)
typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float gyro_x_offset;
    float gyro_y_offset;
    float gyro_z_offset;
    float roll;
    float pitch;
    float yaw;
    int calibration_progress;  // 0-100
    char calib_msg[64];
    float auto_lpf;
    float auto_kp;
} imu_data_t;

extern imu_data_t imu_data;

/**
 * @brief Initialize IMU based on NVS stored type.
 *        Reads "imu_type" from NVS, defaults to ICM42688 if not set.
 */
void imu_hal_init(void);

/**
 * @brief Read raw accel/gyro data from the active IMU.
 *
 * @param accel_data Array of 3 floats: ax, ay, az (in m/s² or g depending on sensor)
 * @param gyro_data  Array of 3 floats: gx, gy, gz (in rad/s)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t imu_hal_read_data(float *accel_data, float *gyro_data);

/**
 * @brief Run calibration on the active IMU.
 *
 * @param samples Number of samples for offset calculation
 */
void imu_hal_calibration(int samples);

/**
 * @brief Read task for FreeRTOS (calls the active IMU's read task logic).
 */
void imu_hal_read_task(void *arg);

/**
 * @brief Load NVS calibration offsets for the active IMU.
 */
void imu_hal_nvs_load(void);

/**
 * @brief Save NVS calibration offsets for the active IMU.
 */
void imu_hal_nvs_save(void);

/**
 * @brief Get the currently active IMU type.
 * @return imu_type_t
 */
imu_type_t imu_hal_get_type(void);

/**
 * @brief Get human-readable IMU name string.
 * @return const char*
 */
const char *imu_hal_get_type_str(void);

/**
 * @brief Set the IMU type (persisted to NVS, takes effect after reboot).
 *
 * @param type imu_type_t to set
 */
void imu_hal_set_type(imu_type_t type);

/**
 * @brief Get the number of supported IMU types.
 * @return int
 */
int imu_hal_get_type_count(void);

#endif /* _IMU_HAL_H_ */