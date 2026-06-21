#include "device/inc/imu_hal.h"
#include "device/inc/icm42688.h"
#include "device/inc/mpu6050.h"
#include "device/inc/lsm6dsm.h"
#include "driver/inc/nvs.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "imu_hal";

// Unified data instance visible to the entire application
imu_data_t imu_data;

// Current active IMU type (loaded from NVS at init)
static imu_type_t s_active_type = IMU_TYPE_ICM42688;

// ──────────────────────────────────────────────
// Internal sync helpers (copy sensor struct ↔ imu_data)
// ──────────────────────────────────────────────

static void sync_icm_to_hal(void) {
    imu_data.accel_x = icm42688_data.accel_x;
    imu_data.accel_y = icm42688_data.accel_y;
    imu_data.accel_z = icm42688_data.accel_z;
    imu_data.gyro_x  = icm42688_data.gyro_x;
    imu_data.gyro_y  = icm42688_data.gyro_y;
    imu_data.gyro_z  = icm42688_data.gyro_z;
    imu_data.gyro_x_offset = icm42688_data.gyro_x_offset;
    imu_data.gyro_y_offset = icm42688_data.gyro_y_offset;
    imu_data.gyro_z_offset = icm42688_data.gyro_z_offset;
    imu_data.roll    = icm42688_data.roll;
    imu_data.pitch   = icm42688_data.pitch;
    imu_data.yaw     = icm42688_data.yaw;
    imu_data.calibration_progress = icm42688_data.calibration_progress;
    imu_data.auto_lpf = icm42688_data.auto_lpf;
    imu_data.auto_kp  = icm42688_data.auto_kp;
    memcpy(imu_data.calib_msg, icm42688_data.calib_msg, sizeof(imu_data.calib_msg));
}

static void sync_hal_to_icm(void) {
    icm42688_data.gyro_x_offset = imu_data.gyro_x_offset;
    icm42688_data.gyro_y_offset = imu_data.gyro_y_offset;
    icm42688_data.gyro_z_offset = imu_data.gyro_z_offset;
    icm42688_data.auto_lpf      = imu_data.auto_lpf;
    icm42688_data.auto_kp       = imu_data.auto_kp;
    strncpy(icm42688_data.calib_msg, imu_data.calib_msg, sizeof(icm42688_data.calib_msg));
}

static void sync_mpu_to_hal(void) {
    imu_data.accel_x = mpu6050_data.accel_x;
    imu_data.accel_y = mpu6050_data.accel_y;
    imu_data.accel_z = mpu6050_data.accel_z;
    imu_data.gyro_x  = mpu6050_data.gyro_x;
    imu_data.gyro_y  = mpu6050_data.gyro_y;
    imu_data.gyro_z  = mpu6050_data.gyro_z;
    imu_data.gyro_x_offset = mpu6050_data.gyro_x_offset;
    imu_data.gyro_y_offset = mpu6050_data.gyro_y_offset;
    imu_data.gyro_z_offset = mpu6050_data.gyro_z_offset;
    imu_data.roll    = mpu6050_data.roll;
    imu_data.pitch   = mpu6050_data.pitch;
    imu_data.yaw     = mpu6050_data.yaw;
    imu_data.calibration_progress = mpu6050_data.calibration_progress;
    imu_data.auto_lpf = mpu6050_data.auto_lpf;
    imu_data.auto_kp  = mpu6050_data.auto_kp;
    memcpy(imu_data.calib_msg, mpu6050_data.calib_msg, sizeof(imu_data.calib_msg));
}

static void sync_hal_to_mpu(void) {
    mpu6050_data.gyro_x_offset = imu_data.gyro_x_offset;
    mpu6050_data.gyro_y_offset = imu_data.gyro_y_offset;
    mpu6050_data.gyro_z_offset = imu_data.gyro_z_offset;
    mpu6050_data.auto_lpf      = imu_data.auto_lpf;
    mpu6050_data.auto_kp       = imu_data.auto_kp;
    strncpy(mpu6050_data.calib_msg, imu_data.calib_msg, sizeof(mpu6050_data.calib_msg));
}

static void sync_lsm_to_hal(void) {
    imu_data.accel_x = lsm6dsm_data.accel_x;
    imu_data.accel_y = lsm6dsm_data.accel_y;
    imu_data.accel_z = lsm6dsm_data.accel_z;
    imu_data.gyro_x  = lsm6dsm_data.gyro_x;
    imu_data.gyro_y  = lsm6dsm_data.gyro_y;
    imu_data.gyro_z  = lsm6dsm_data.gyro_z;
    imu_data.gyro_x_offset = lsm6dsm_data.gyro_x_offset;
    imu_data.gyro_y_offset = lsm6dsm_data.gyro_y_offset;
    imu_data.gyro_z_offset = lsm6dsm_data.gyro_z_offset;
    imu_data.roll    = lsm6dsm_data.roll;
    imu_data.pitch   = lsm6dsm_data.pitch;
    imu_data.yaw     = lsm6dsm_data.yaw;
    imu_data.calibration_progress = lsm6dsm_data.calibration_progress;
    imu_data.auto_lpf = lsm6dsm_data.auto_lpf;
    imu_data.auto_kp  = lsm6dsm_data.auto_kp;
    memcpy(imu_data.calib_msg, lsm6dsm_data.calib_msg, sizeof(imu_data.calib_msg));
}

static void sync_hal_to_lsm(void) {
    lsm6dsm_data.gyro_x_offset = imu_data.gyro_x_offset;
    lsm6dsm_data.gyro_y_offset = imu_data.gyro_y_offset;
    lsm6dsm_data.gyro_z_offset = imu_data.gyro_z_offset;
    lsm6dsm_data.auto_lpf      = imu_data.auto_lpf;
    lsm6dsm_data.auto_kp       = imu_data.auto_kp;
    strncpy(lsm6dsm_data.calib_msg, imu_data.calib_msg, sizeof(lsm6dsm_data.calib_msg));
}

// ──────────────────────────────────────────────
// Public API
// ──────────────────────────────────────────────

void imu_hal_init(void) {
    // Read IMU type from NVS
    int32_t type_val = IMU_TYPE_ICM42688;
    esp_err_t ret = nvs_read_i32("imu_type", &type_val);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "imu_type not found in NVS, defaulting to ICM42688");
        type_val = IMU_TYPE_ICM42688;
        nvs_write_i32("imu_type", type_val);
    }
    if (type_val != IMU_TYPE_ICM42688 && type_val != IMU_TYPE_MPU6050 &&
        type_val != IMU_TYPE_LSM6DSM) {
        ESP_LOGW(TAG, "Invalid imu_type %ld, falling back to ICM42688", (long)type_val);
        type_val = IMU_TYPE_ICM42688;
        nvs_write_i32("imu_type", type_val);
    }
    s_active_type = (imu_type_t)type_val;
    ESP_LOGI(TAG, "Active IMU: %s", imu_hal_get_type_str());

    // Initialize the selected sensor
    if (s_active_type == IMU_TYPE_ICM42688) {
        icm42688_init();
        offset_icm42688_nvs_init();
    } else if (s_active_type == IMU_TYPE_MPU6050) {
        mpu6050_init();
        offset_mpu6050_nvs_init();
    } else {
        lsm6dsm_init();
        offset_lsm6dsm_nvs_init();
    }
    imu_hal_nvs_load();
}

esp_err_t imu_hal_read_data(float *accel_data, float *gyro_data) {
    if (s_active_type == IMU_TYPE_ICM42688) {
        return icm42688_read_data(accel_data, gyro_data);
    } else if (s_active_type == IMU_TYPE_MPU6050) {
        return mpu6050_read_data(accel_data, gyro_data);
    } else {
        return lsm6dsm_read_data(accel_data, gyro_data);
    }
}

void imu_hal_read_task(void *arg) {
    if (s_active_type == IMU_TYPE_ICM42688) {
        icm42688_read_task(arg);
    } else if (s_active_type == IMU_TYPE_MPU6050) {
        mpu6050_read_task(arg);
    } else {
        lsm6dsm_read_task(arg);
    }
}

void imu_hal_calibration(int samples) {
    if (s_active_type == IMU_TYPE_ICM42688) {
        icm42688_calibration(samples);
        sync_icm_to_hal();
    } else if (s_active_type == IMU_TYPE_MPU6050) {
        mpu6050_calibration(samples);
        sync_mpu_to_hal();
    } else {
        lsm6dsm_calibration(samples);
        sync_lsm_to_hal();
    }
}

void imu_hal_nvs_load(void) {
    if (s_active_type == IMU_TYPE_ICM42688) {
        offset_icm42688_nvs_load();
        sync_icm_to_hal();
    } else if (s_active_type == IMU_TYPE_MPU6050) {
        offset_mpu6050_nvs_load();
        sync_mpu_to_hal();
    } else {
        offset_lsm6dsm_nvs_load();
        sync_lsm_to_hal();
    }
}

void imu_hal_nvs_save(void) {
    if (s_active_type == IMU_TYPE_ICM42688) {
        sync_hal_to_icm();
        offset_icm42688_nvs_save();
    } else if (s_active_type == IMU_TYPE_MPU6050) {
        sync_hal_to_mpu();
        offset_mpu6050_nvs_save();
    } else {
        sync_hal_to_lsm();
        offset_lsm6dsm_nvs_save();
    }
}

imu_type_t imu_hal_get_type(void) {
    return s_active_type;
}

const char *imu_hal_get_type_str(void) {
    switch (s_active_type) {
        case IMU_TYPE_ICM42688: return "ICM42688";
        case IMU_TYPE_MPU6050:  return "MPU6050";
        case IMU_TYPE_LSM6DSM:  return "LSM6DSM";
        default:                return "Unknown";
    }
}

void imu_hal_set_type(imu_type_t type) {
    if (type != IMU_TYPE_ICM42688 && type != IMU_TYPE_MPU6050 &&
        type != IMU_TYPE_LSM6DSM) return;
    s_active_type = type;
    nvs_write_i32("imu_type", (int32_t)type);
    ESP_LOGI(TAG, "IMU type set to %s (reboot to take effect)", imu_hal_get_type_str());
}

int imu_hal_get_type_count(void) {
    return 3;  // ICM42688, MPU6050, LSM6DSM
}
