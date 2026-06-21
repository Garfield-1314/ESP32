#include "device/inc/icm42688.h"

#include <math.h>
#include <stdio.h>

#include "Algorithm/inc/imu_fusion.h"
#include "device/inc/motor.h"
#include "driver/i2c_master.h"
#include "driver/inc/nvs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define FILTER_BUF_LEN 10

static const char *TAG = "ICM42688";
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;
static float icm42688_acc_ssvt = 16384.0f;
static float icm42688_gyro_ssvt = 16.384f;

icm42688_data_t icm42688_data;

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

static esp_err_t icm42688_write_reg(uint8_t reg, uint8_t data) {
  uint8_t buf[2] = {reg, data};
  return i2c_master_transmit(dev_handle, buf, sizeof(buf), -1);
}

static esp_err_t icm42688_read_regs(uint8_t reg, uint8_t *data, size_t len) {
  return i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, -1);
}

void icm42688_set_range(icm42688_afs_t afs, icm42688_aodr_t aodr,
                        icm42688_gfs_t gfs, icm42688_godr_t godr) {
  icm42688_write_reg(ICM42688_ACCEL_CONFIG0, (afs << 5) | (aodr));
  icm42688_write_reg(ICM42688_GYRO_CONFIG0, (gfs << 5) | (godr));

  switch (afs) {
    case ICM42688_AFS_2G:
      icm42688_acc_ssvt = 16384.0f;
      break;
    case ICM42688_AFS_4G:
      icm42688_acc_ssvt = 8192.0f;
      break;
    case ICM42688_AFS_8G:
      icm42688_acc_ssvt = 4096.0f;
      break;
    case ICM42688_AFS_16G:
      icm42688_acc_ssvt = 2048.0f;
      break;
    default:
      icm42688_acc_ssvt = 16384.0f;  // Default 2G
      break;
  }

  switch (gfs) {
    case ICM42688_GFS_15_625DPS:
      icm42688_gyro_ssvt = 2097.2f;
      break;
    case ICM42688_GFS_31_25DPS:
      icm42688_gyro_ssvt = 1048.6f;
      break;
    case ICM42688_GFS_62_5DPS:
      icm42688_gyro_ssvt = 524.3f;
      break;
    case ICM42688_GFS_125DPS:
      icm42688_gyro_ssvt = 262.1f;
      break;
    case ICM42688_GFS_250DPS:
      icm42688_gyro_ssvt = 131.1f;
      break;
    case ICM42688_GFS_500DPS:
      icm42688_gyro_ssvt = 65.5f;
      break;
    case ICM42688_GFS_1000DPS:
      icm42688_gyro_ssvt = 32.8f;
      break;
    case ICM42688_GFS_2000DPS:
      icm42688_gyro_ssvt = 16.4f;
      break;
    default:
      icm42688_gyro_ssvt = 16.4f;  // Default 2000dps
      break;
  }
}

void icm42688_init(void) {
  ESP_LOGI(TAG, "Init ICM42688 on I2C%d (SDA:%d, SCL:%d)", IMU_I2C_PORT,
           IMU_PIN_SDA, IMU_PIN_SCL);

    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = IMU_I2C_PORT,
        .scl_io_num = IMU_PIN_SCL,
        .sda_io_num = IMU_PIN_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,  // 外部有上拉电阻，关闭内部上拉
    };

  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = IMU_I2C_ADDR,
      .scl_speed_hz = IMU_CLK_SPEED,
  };

  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle));

  vTaskDelay(pdMS_TO_TICKS(100));

  // Reset device
  icm42688_write_reg(ICM42688_DEVICE_CONFIG, 0x01);
  vTaskDelay(pdMS_TO_TICKS(100));

  // Turn on accel and gyro in low noise mode
  icm42688_write_reg(ICM42688_PWR_MGMT0, 0x0F);
  vTaskDelay(pdMS_TO_TICKS(10));

  // Configuration: 2g, 1kHz, 2000dps, 1kHz
  icm42688_set_range(ICM42688_AFS_2G, ICM42688_AODR_1000HZ,
                     ICM42688_GFS_2000DPS, ICM42688_GODR_1000HZ);

  vTaskDelay(pdMS_TO_TICKS(10));

  // Initialize filter buffers
  for (int i = 0; i < FILTER_BUF_LEN; i++) {
    gyro_x_buf[i] = 0.0f;
    gyro_y_buf[i] = 0.0f;
    gyro_z_buf[i] = 0.0f;
  }

  imu_fusion_init();
  offset_icm42688_nvs_load();

  // Apply loaded or default band-pass filters
  for (int i = 0; i < 3; i++) {
    gyro_bpf_init(&gyro_filters[i], icm42688_data.auto_lpf, 0.1f,
                  motor_settings.frequency_hz);
  }
  imu_fusion_set_gains(icm42688_data.auto_kp, 0.0f);
  // icm42688_calibration(1000);
}

void icm42688_calibration(int samples) {
  ESP_LOGI(TAG, "Starting ICM42688 calibration and auto-tuning...");

  bool previous_motor_state = is_motor_enabled;
  motor_set_enable(false);  // Stop motors to avoid vibration during calibration

  float gx_sum = 0, gy_sum = 0, gz_sum = 0;
  float gx_sq_sum = 0, gy_sq_sum = 0, gz_sq_sum = 0;

  icm42688_data.gyro_x_offset = 0;
  icm42688_data.gyro_y_offset = 0;
  icm42688_data.gyro_z_offset = 0;
  icm42688_data.calibration_progress = 0;

  for (int i = 0; i < samples; i++) {
    if (icm42688_read_data(NULL, NULL) != ESP_OK) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    float gx = icm42688_data.gyro_x;
    float gy = icm42688_data.gyro_y;
    float gz = icm42688_data.gyro_z;

    gx_sum += gx;
    gy_sum += gy;
    gz_sum += gz;

    gx_sq_sum += gx * gx;
    gy_sq_sum += gy * gy;
    gz_sq_sum += gz * gz;

    icm42688_data.calibration_progress = (i * 100) / samples;
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  icm42688_data.gyro_x_offset = gx_sum / samples;
  icm42688_data.gyro_y_offset = gy_sum / samples;
  icm42688_data.gyro_z_offset = gz_sum / samples;

  // Calculate standard deviation (noise level)
  float gx_var = (gx_sq_sum / samples) -
                 (icm42688_data.gyro_x_offset * icm42688_data.gyro_x_offset);
  float gy_var = (gy_sq_sum / samples) -
                 (icm42688_data.gyro_y_offset * icm42688_data.gyro_y_offset);
  float gz_var = (gz_sq_sum / samples) -
                 (icm42688_data.gyro_z_offset * icm42688_data.gyro_z_offset);

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

  icm42688_data.auto_lpf = dynamic_lpf;
  icm42688_data.auto_kp = 0.0f;
  snprintf(icm42688_data.calib_msg, sizeof(icm42688_data.calib_msg),
           "LPF:%.1fHz Kalman", dynamic_lpf);

  icm42688_data.calibration_progress = 100;

  ESP_LOGI(TAG, "Calibration done. Offsets: X=%.2f Y=%.2f Z=%.2f",
           icm42688_data.gyro_x_offset, icm42688_data.gyro_y_offset,
           icm42688_data.gyro_z_offset);

  offset_icm42688_nvs_save();

  // Reset fusion and internal data to start from 0
  imu_fusion_init();
  icm42688_data.roll = 0;
  icm42688_data.pitch = 0;
  icm42688_data.yaw = 0;
  icm42688_data.gyro_x = 0;
  icm42688_data.gyro_y = 0;
  icm42688_data.gyro_z = 0;

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

esp_err_t icm42688_read_data(float *accel_data, float *gyro_data) {
  uint8_t raw[14];
  esp_err_t ret = icm42688_read_regs(ICM42688_ACCEL_DATA_X1, raw, 14);
  if (ret != ESP_OK) {
    return ret;
  }

  // Parse Accelerometer
  int16_t ax = (raw[0] << 8) | raw[1];
  int16_t ay = (raw[2] << 8) | raw[3];
  int16_t az = (raw[4] << 8) | raw[5];

  icm42688_data.accel_x = ax / icm42688_acc_ssvt;
  icm42688_data.accel_y = ay / icm42688_acc_ssvt;
  icm42688_data.accel_z = az / icm42688_acc_ssvt;

  // Parse Gyroscope
  int16_t gx = (raw[6] << 8) | raw[7];
  int16_t gy = (raw[8] << 8) | raw[9];
  int16_t gz = (raw[10] << 8) | raw[11];

  icm42688_data.gyro_x = gx / icm42688_gyro_ssvt - icm42688_data.gyro_x_offset;
  icm42688_data.gyro_y = gy / icm42688_gyro_ssvt - icm42688_data.gyro_y_offset;
  icm42688_data.gyro_z = gz / icm42688_gyro_ssvt - icm42688_data.gyro_z_offset;

  if (accel_data) {
    accel_data[0] = icm42688_data.accel_x;
    accel_data[1] = icm42688_data.accel_y;
    accel_data[2] = icm42688_data.accel_z;
  }

  if (gyro_data) {
    gyro_data[0] = icm42688_data.gyro_x;
    gyro_data[1] = icm42688_data.gyro_y;
    gyro_data[2] = icm42688_data.gyro_z;
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

void icm42688_read_task(void *arg) {
  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency =
      pdMS_TO_TICKS(1000 / motor_settings.frequency_hz);
  float dt = 1.0f / motor_settings.frequency_hz;

  for (;;) {
    if (icm42688_read_data(NULL, NULL) != ESP_OK) {
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    float ma_gx = filter_apply(gyro_x_buf, icm42688_data.gyro_x);
    float ma_gy = filter_apply(gyro_y_buf, icm42688_data.gyro_y);
    float ma_gz = filter_apply(gyro_z_buf, icm42688_data.gyro_z);

    filter_idx++;
    if (filter_idx >= FILTER_BUF_LEN) filter_idx = 0;

    icm42688_data.gyro_x = gyro_bpf_apply(&gyro_filters[0], ma_gx);
    icm42688_data.gyro_y = gyro_bpf_apply(&gyro_filters[1], ma_gy);
    icm42688_data.gyro_z = gyro_bpf_apply(&gyro_filters[2], ma_gz);

    imu_fusion_update(icm42688_data.accel_x, icm42688_data.accel_y,
                      icm42688_data.accel_z, icm42688_data.gyro_x,
                      icm42688_data.gyro_y, icm42688_data.gyro_z, dt);

    imu_fusion_get_euler(&icm42688_data.roll, &icm42688_data.pitch,
                         &icm42688_data.yaw);

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void offset_icm42688_nvs_init(void) {
  esp_err_t err = nvs_init_custom();
  if (err == ESP_OK) {
    offset_icm42688_nvs_load();
  } else {
    ESP_LOGE(TAG, "NVS Init Failed");
  }
}

void offset_icm42688_nvs_save(void) {
  nvs_write_float("imu_ox", icm42688_data.gyro_x_offset);
  nvs_write_float("imu_oy", icm42688_data.gyro_y_offset);
  nvs_write_float("imu_oz", icm42688_data.gyro_z_offset);
  nvs_write_float("imu_lpf", icm42688_data.auto_lpf);
  nvs_write_float("imu_kp", icm42688_data.auto_kp);
  ESP_LOGI(TAG, "IMU parameters saved to NVS");
}

void offset_icm42688_nvs_load(void) {
  if (nvs_read_float("imu_ox", &icm42688_data.gyro_x_offset) != ESP_OK)
    icm42688_data.gyro_x_offset = 0.0f;
  if (nvs_read_float("imu_oy", &icm42688_data.gyro_y_offset) != ESP_OK)
    icm42688_data.gyro_y_offset = 0.0f;
  if (nvs_read_float("imu_oz", &icm42688_data.gyro_z_offset) != ESP_OK)
    icm42688_data.gyro_z_offset = 0.0f;

  if (nvs_read_float("imu_lpf", &icm42688_data.auto_lpf) != ESP_OK)
    icm42688_data.auto_lpf = 30.0f;
  if (nvs_read_float("imu_kp", &icm42688_data.auto_kp) != ESP_OK)
    icm42688_data.auto_kp = 0.0f;  // Default 0 for Kalman

  /* Set Kalman message on load instead of Kp */
  snprintf(icm42688_data.calib_msg, sizeof(icm42688_data.calib_msg),
           "LPF:%.1fHz Kalman", icm42688_data.auto_lpf);

  ESP_LOGI(TAG,
           "IMU parameters loaded: Offsets(%.2f, %.2f, %.2f) LPF:%.1f Kalman",
           icm42688_data.gyro_x_offset, icm42688_data.gyro_y_offset,
           icm42688_data.gyro_z_offset, icm42688_data.auto_lpf);
}
