#include "device/inc/mpu6050.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "Algorithm/inc/imu_fusion.h"
#include "device/inc/imu_hal.h"
#include "device/inc/motor.h"
#include "driver/i2c_master.h"
#include "ui/inc/cmd_page.h"
// 调试：持续在 cmd 页面输出 MPU6050 数据。调试完成后注释掉此行即可关闭。
// #define MPU6050_DEBUG_CMD

#include "driver/inc/nvs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FILTER_BUF_LEN 10

static const char *TAG = "MPU6050";
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;
static float mpu6050_acc_ssvt = 16384.0f;   // for 2G
static float mpu6050_gyro_ssvt = 131.0f;    // for 250dps (250 / 32768 = ~0.00763, but MPU6050 uses degrees/sec)

mpu6050_data_t mpu6050_data;

// Filter buffers
static float gyro_x_buf[FILTER_BUF_LEN];
static float gyro_y_buf[FILTER_BUF_LEN];
static float gyro_z_buf[FILTER_BUF_LEN];
static int filter_idx = 0;

// Band-pass filter structure for gyro
typedef struct {
    float lpf_alpha;
    float hpf_alpha;
    float prev_lpf;
    float prev_hpf;
    float prev_input;
} gyro_bpf_t;

static gyro_bpf_t gyro_filters[3];

static void gyro_bpf_init(gyro_bpf_t *f, float lpf_cutoff, float hpf_cutoff,
                          float fs) {
    float dt = 1.0f / fs;
    float rc_lpf = 1.0f / (2.0f * M_PI * lpf_cutoff);
    f->lpf_alpha = dt / (rc_lpf + dt);

    float rc_hpf = 1.0f / (2.0f * M_PI * hpf_cutoff);
    f->hpf_alpha = rc_hpf / (rc_hpf + dt);

    f->prev_lpf = 0;
    f->prev_hpf = 0;
    f->prev_input = 0;
}

static float gyro_bpf_apply(gyro_bpf_t *f, float input) {
    // 1st order Low-pass filter
    f->prev_lpf = f->lpf_alpha * input + (1.0f - f->lpf_alpha) * f->prev_lpf;
    return f->prev_lpf;
}

static esp_err_t mpu6050_write_reg(uint8_t reg, uint8_t data) {
    uint8_t buf[2] = {reg, data};
    return i2c_master_transmit(dev_handle, buf, sizeof(buf), -1);
}

static esp_err_t mpu6050_read_regs(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, -1);
}

void mpu6050_set_dlpf(uint8_t dlpf_cfg) {
    mpu6050_write_reg(MPU6050_CONFIG, dlpf_cfg & 0x07);
    ESP_LOGI(TAG, "DLPF set to 0x%02X", dlpf_cfg & 0x07);
}

void mpu6050_set_sample_rate(uint8_t div) {
    // Sample Rate = Gyroscope Output Rate / (1 + SMPLRT_DIV)
    // When DLPF is enabled, Gyroscope Output Rate = 1kHz
    mpu6050_write_reg(MPU6050_SMPLRT_DIV, div);
    ESP_LOGI(TAG, "Sample rate divider set to %d (actual rate: %d Hz)", div,
             div ? 1000 / (1 + div) : 1000);
}

void mpu6050_set_range(mpu6050_afs_t afs, mpu6050_gfs_t gfs) {
    // Set accelerometer full scale
    mpu6050_write_reg(MPU6050_ACCEL_CONFIG, (afs << 3) & 0x18);
    // Set gyroscope full scale
    mpu6050_write_reg(MPU6050_GYRO_CONFIG, (gfs << 3) & 0x18);

    // Configure sensitivity scale values
    switch (afs) {
        case MPU6050_AFS_2G:
            mpu6050_acc_ssvt = 16384.0f;
            break;
        case MPU6050_AFS_4G:
            mpu6050_acc_ssvt = 8192.0f;
            break;
        case MPU6050_AFS_8G:
            mpu6050_acc_ssvt = 4096.0f;
            break;
        case MPU6050_AFS_16G:
            mpu6050_acc_ssvt = 2048.0f;
            break;
        default:
            mpu6050_acc_ssvt = 16384.0f;
            break;
    }

    switch (gfs) {
        case MPU6050_GFS_250DPS:
            mpu6050_gyro_ssvt = 131.0f;
            break;
        case MPU6050_GFS_500DPS:
            mpu6050_gyro_ssvt = 65.5f;
            break;
        case MPU6050_GFS_1000DPS:
            mpu6050_gyro_ssvt = 32.8f;
            break;
        case MPU6050_GFS_2000DPS:
            mpu6050_gyro_ssvt = 16.4f;
            break;
        default:
            mpu6050_gyro_ssvt = 131.0f;
            break;
    }
}

void mpu6050_init(void) {
    ESP_LOGI(TAG, "Init MPU6050 on I2C%d (SDA:%d, SCL:%d)", MPU6050_I2C_PORT,
             MPU6050_PIN_SDA, MPU6050_PIN_SCL);

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = MPU6050_I2C_PORT,
        .scl_io_num = MPU6050_PIN_SCL,
        .sda_io_num = MPU6050_PIN_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,  // 外部有4.7KΩ上拉，关闭内部上拉
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_I2C_ADDR,
        .scl_speed_hz = MPU6050_CLK_SPEED,
    };

    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

    vTaskDelay(pdMS_TO_TICKS(100));

    // Verify device identity
    uint8_t who_am_i = 0;
    mpu6050_read_regs(MPU6050_WHO_AM_I, &who_am_i, 1);
    ESP_LOGI(TAG, "WHO_AM_I = 0x%02X (expected 0x68)", who_am_i);
    if (who_am_i != 0x68) {
        ESP_LOGW(TAG, "Unexpected WHO_AM_I value! Continuing anyway...");
    }

    // Reset device
    mpu6050_write_reg(MPU6050_PWR_MGMT_1, 0x80);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Wake up device (disable sleep, use internal oscillator)
    mpu6050_write_reg(MPU6050_PWR_MGMT_1, 0x00);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Set DLPF to 94Hz (good balance of noise vs response)
    mpu6050_set_dlpf(MPU6050_DLPF_94HZ);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Set sample rate to 1kHz (divider = 0)
    mpu6050_set_sample_rate(0);
    vTaskDelay(pdMS_TO_TICKS(10));

    // Configuration: ±2G, ±2000dps
    mpu6050_set_range(MPU6050_AFS_2G, MPU6050_GFS_2000DPS);

    vTaskDelay(pdMS_TO_TICKS(10));

    // Initialize filter buffers
    for (int i = 0; i < FILTER_BUF_LEN; i++) {
        gyro_x_buf[i] = 0.0f;
        gyro_y_buf[i] = 0.0f;
        gyro_z_buf[i] = 0.0f;
    }

    imu_fusion_init();
    offset_mpu6050_nvs_load();

    // Apply loaded or default band-pass filters
    for (int i = 0; i < 3; i++) {
        gyro_bpf_init(&gyro_filters[i], mpu6050_data.auto_lpf, 0.1f,
                      motor_settings.frequency_hz);
    }
    imu_fusion_set_gains(mpu6050_data.auto_kp, 0.0f);
    // mpu6050_calibration(1000);
}

void mpu6050_calibration(int samples) {
    ESP_LOGI(TAG, "Starting MPU6050 calibration and auto-tuning...");

    bool previous_motor_state = is_motor_enabled;
    motor_set_enable(false);  // Stop motors to avoid vibration during calibration

    float gx_sum = 0, gy_sum = 0, gz_sum = 0;
    float gx_sq_sum = 0, gy_sq_sum = 0, gz_sq_sum = 0;

    mpu6050_data.gyro_x_offset = 0;
    mpu6050_data.gyro_y_offset = 0;
    mpu6050_data.gyro_z_offset = 0;
    mpu6050_data.calibration_progress = 0;

    for (int i = 0; i < samples; i++) {
        if (mpu6050_read_data(NULL, NULL) != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        float gx = mpu6050_data.gyro_x;
        float gy = mpu6050_data.gyro_y;
        float gz = mpu6050_data.gyro_z;

        gx_sum += gx;
        gy_sum += gy;
        gz_sum += gz;

        gx_sq_sum += gx * gx;
        gy_sq_sum += gy * gy;
        gz_sq_sum += gz * gz;

        mpu6050_data.calibration_progress = (i * 100) / samples;
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    mpu6050_data.gyro_x_offset = gx_sum / samples;
    mpu6050_data.gyro_y_offset = gy_sum / samples;
    mpu6050_data.gyro_z_offset = gz_sum / samples;

    // Calculate standard deviation (noise level)
    float gx_var = (gx_sq_sum / samples) -
                   (mpu6050_data.gyro_x_offset * mpu6050_data.gyro_x_offset);
    float gy_var = (gy_sq_sum / samples) -
                   (mpu6050_data.gyro_y_offset * mpu6050_data.gyro_y_offset);
    float gz_var = (gz_sq_sum / samples) -
                   (mpu6050_data.gyro_z_offset * mpu6050_data.gyro_z_offset);

    float max_var = gx_var;
    if (gy_var > max_var) max_var = gy_var;
    if (gz_var > max_var) max_var = gz_var;
    float std_dev = sqrtf(max_var > 0 ? max_var : 0);

    float dynamic_lpf = 30.0f;
    if (std_dev > 0.5f) {
        dynamic_lpf = 30.0f / (1.0f + (std_dev - 0.5f) * 2.0f);
    }
    if (dynamic_lpf < 5.0f) dynamic_lpf = 5.0f;

    float dynamic_hpf = 0.1f;

    ESP_LOGI(TAG, "Noise StdDev: %.4f, Auto-tuning LPF to %.2f Hz", std_dev,
             dynamic_lpf);

    for (int i = 0; i < 3; i++) {
        gyro_bpf_init(&gyro_filters[i], dynamic_lpf, dynamic_hpf,
                      motor_settings.frequency_hz);
    }

    mpu6050_data.auto_lpf = dynamic_lpf;
    mpu6050_data.auto_kp = 0.0f;
    snprintf(mpu6050_data.calib_msg, sizeof(mpu6050_data.calib_msg),
             "LPF:%.1fHz Kalman", dynamic_lpf);

    mpu6050_data.calibration_progress = 100;

    ESP_LOGI(TAG, "Calibration done. Offsets: X=%.2f Y=%.2f Z=%.2f",
             mpu6050_data.gyro_x_offset, mpu6050_data.gyro_y_offset,
             mpu6050_data.gyro_z_offset);

    offset_mpu6050_nvs_save();

    // Reset fusion and internal data to start from 0
    imu_fusion_init();
    mpu6050_data.roll = 0;
    mpu6050_data.pitch = 0;
    mpu6050_data.yaw = 0;
    mpu6050_data.gyro_x = 0;
    mpu6050_data.gyro_y = 0;
    mpu6050_data.gyro_z = 0;

    for (int i = 0; i < FILTER_BUF_LEN; i++) {
        gyro_x_buf[i] = 0.0f;
        gyro_y_buf[i] = 0.0f;
        gyro_z_buf[i] = 0.0f;
    }

    for (int i = 0; i < 3; i++) {
        gyro_filters[i].prev_lpf = 0;
        gyro_filters[i].prev_hpf = 0;
        gyro_filters[i].prev_input = 0;
    }

    motor_set_enable(previous_motor_state);
}

esp_err_t mpu6050_read_data(float *accel_data, float *gyro_data) {
    uint8_t raw[14];
    // Read all 14 bytes starting from ACCEL_XOUT_H (0x3B)
    esp_err_t ret = mpu6050_read_regs(MPU6050_ACCEL_XOUT_H, raw, 14);
    if (ret != ESP_OK) {
        return ret;
    }

    // Parse Accelerometer (big-endian 16-bit)
    int16_t ax = (raw[0] << 8) | raw[1];
    int16_t ay = (raw[2] << 8) | raw[3];
    int16_t az = (raw[4] << 8) | raw[5];

    mpu6050_data.accel_x = ax / mpu6050_acc_ssvt;
    mpu6050_data.accel_y = ay / mpu6050_acc_ssvt;
    mpu6050_data.accel_z = az / mpu6050_acc_ssvt;

    // Skip temperature (raw[6..7])

    // Parse Gyroscope (big-endian 16-bit)
    int16_t gx = (raw[8] << 8) | raw[9];
    int16_t gy = (raw[10] << 8) | raw[11];
    int16_t gz = (raw[12] << 8) | raw[13];

    mpu6050_data.gyro_x = gx / mpu6050_gyro_ssvt - mpu6050_data.gyro_x_offset;
    mpu6050_data.gyro_y = gy / mpu6050_gyro_ssvt - mpu6050_data.gyro_y_offset;
    mpu6050_data.gyro_z = gz / mpu6050_gyro_ssvt - mpu6050_data.gyro_z_offset;

    if (accel_data) {
        accel_data[0] = mpu6050_data.accel_x;
        accel_data[1] = mpu6050_data.accel_y;
        accel_data[2] = mpu6050_data.accel_z;
    }

    if (gyro_data) {
        gyro_data[0] = mpu6050_data.gyro_x;
        gyro_data[1] = mpu6050_data.gyro_y;
        gyro_data[2] = mpu6050_data.gyro_z;
    }

    return ESP_OK;
}

static float filter_apply(float *buf, float new_val) {
    buf[filter_idx] = new_val;

    float min = buf[0];
    float max = buf[0];
    float sum = 0.0f;

    for (int i = 0; i < FILTER_BUF_LEN; i++) {
        float v = buf[i];
        if (v < min) min = v;
        if (v > max) max = v;
        sum += v;
    }

    return (sum - min - max) / (FILTER_BUF_LEN - 2);
}

void mpu6050_read_task(void *arg) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency =
        pdMS_TO_TICKS(1000 / motor_settings.frequency_hz);
    float dt = 1.0f / motor_settings.frequency_hz;

#ifdef MPU6050_DEBUG_CMD
    char dbg_buf[128];
    int dbg_cnt = 0;
#endif

    for (;;) {
        if (mpu6050_read_data(NULL, NULL) != ESP_OK) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }

        float ma_gx = filter_apply(gyro_x_buf, mpu6050_data.gyro_x);
        float ma_gy = filter_apply(gyro_y_buf, mpu6050_data.gyro_y);
        float ma_gz = filter_apply(gyro_z_buf, mpu6050_data.gyro_z);

        filter_idx++;
        if (filter_idx >= FILTER_BUF_LEN) filter_idx = 0;

        mpu6050_data.gyro_x = gyro_bpf_apply(&gyro_filters[0], ma_gx);
        mpu6050_data.gyro_y = gyro_bpf_apply(&gyro_filters[1], ma_gy);
        mpu6050_data.gyro_z = gyro_bpf_apply(&gyro_filters[2], ma_gz);

        imu_fusion_update(mpu6050_data.accel_x, mpu6050_data.accel_y,
                          mpu6050_data.accel_z, mpu6050_data.gyro_x,
                          mpu6050_data.gyro_y, mpu6050_data.gyro_z, dt);

        imu_fusion_get_euler(&mpu6050_data.roll, &mpu6050_data.pitch,
                             &mpu6050_data.yaw);

        // Sync to HAL global imu_data for UI display
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

#ifdef MPU6050_DEBUG_CMD
        dbg_cnt++;
        if (dbg_cnt >= 50) {  // ~每50帧打印一次（约1Hz），避免刷太快
            dbg_cnt = 0;
            int len = snprintf(dbg_buf, sizeof(dbg_buf),
                "MPU A:%.3f %.3f %.3f G:%.2f %.2f %.2f R:%.1f P:%.1f Y:%.1f\n",
                mpu6050_data.accel_x, mpu6050_data.accel_y, mpu6050_data.accel_z,
                mpu6050_data.gyro_x,  mpu6050_data.gyro_y,  mpu6050_data.gyro_z,
                mpu6050_data.roll,    mpu6050_data.pitch,   mpu6050_data.yaw);
            cmd_page_add_data(dbg_buf, len);
        }
#endif

        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void offset_mpu6050_nvs_init(void) {
    esp_err_t err = nvs_init_custom();
    if (err == ESP_OK) {
        offset_mpu6050_nvs_load();
    } else {
        ESP_LOGE(TAG, "NVS Init Failed");
    }
}

void offset_mpu6050_nvs_save(void) {
    nvs_write_float("imu_ox", mpu6050_data.gyro_x_offset);
    nvs_write_float("imu_oy", mpu6050_data.gyro_y_offset);
    nvs_write_float("imu_oz", mpu6050_data.gyro_z_offset);
    nvs_write_float("imu_lpf", mpu6050_data.auto_lpf);
    nvs_write_float("imu_kp", mpu6050_data.auto_kp);
    ESP_LOGI(TAG, "IMU parameters saved to NVS");
}

void offset_mpu6050_nvs_load(void) {
    if (nvs_read_float("imu_ox", &mpu6050_data.gyro_x_offset) != ESP_OK)
        mpu6050_data.gyro_x_offset = 0.0f;
    if (nvs_read_float("imu_oy", &mpu6050_data.gyro_y_offset) != ESP_OK)
        mpu6050_data.gyro_y_offset = 0.0f;
    if (nvs_read_float("imu_oz", &mpu6050_data.gyro_z_offset) != ESP_OK)
        mpu6050_data.gyro_z_offset = 0.0f;

    if (nvs_read_float("imu_lpf", &mpu6050_data.auto_lpf) != ESP_OK)
        mpu6050_data.auto_lpf = 30.0f;
    if (nvs_read_float("imu_kp", &mpu6050_data.auto_kp) != ESP_OK)
        mpu6050_data.auto_kp = 0.0f;  // Default 0 for Kalman

    /* Set Kalman message on load instead of Kp */
    snprintf(mpu6050_data.calib_msg, sizeof(mpu6050_data.calib_msg),
             "LPF:%.1fHz Kalman", mpu6050_data.auto_lpf);

    ESP_LOGI(TAG,
             "IMU parameters loaded: Offsets(%.2f, %.2f, %.2f) LPF:%.1f Kalman",
             mpu6050_data.gyro_x_offset, mpu6050_data.gyro_y_offset,
             mpu6050_data.gyro_z_offset, mpu6050_data.auto_lpf);
}