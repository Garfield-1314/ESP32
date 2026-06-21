#include "device/inc/lsm6dsm.h"

#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "Algorithm/inc/imu_fusion.h"
#include "device/inc/imu_hal.h"
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

static const char *TAG = "LSM6DSM";
static i2c_master_bus_handle_t bus_handle;
static i2c_master_dev_handle_t dev_handle;
static bool s_init_ok = false;
static uint8_t s_i2c_addr = LSM6DSM_I2C_ADDR;

// Accelerometer sensitivity divisor (LSB per g)
// Sensitivity from datasheet: ±2g=0.061mg/LSB → 1/0.000061 ≈ 16393
static float lsm6dsm_acc_ssvt = 16393.44f;
// Gyroscope sensitivity divisor (LSB per dps)
// Sensitivity from datasheet: ±2000dps=70mdps/LSB → 1/0.07 ≈ 14.286
static float lsm6dsm_gyro_ssvt = 14.286f;

lsm6dsm_data_t lsm6dsm_data;

// Filter buffers
static float gyro_x_buf[FILTER_BUF_LEN];
static float gyro_y_buf[FILTER_BUF_LEN];
static float gyro_z_buf[FILTER_BUF_LEN];
static int filter_idx = 0;

// Step 3: 加速度计二阶IIR陷波滤波器
// 用于抑制电机PWM谐波耦合到加速度计的特定频率振动
// 中心频率: 250Hz (PWM 500Hz的二次谐波), Q值: 10
// 系数通过 bilinear transform 预计算
typedef struct {
  float b0, b1, b2;
  float a1, a2;
  float x1, x2, y1, y2;
} notch_filter_iir_t;

static notch_filter_iir_t acc_notch_x;
static notch_filter_iir_t acc_notch_y;
static notch_filter_iir_t acc_notch_z;

static void notch_filter_init(notch_filter_iir_t *f, float center_freq_hz,
                               float q, float fs) {
  float w0 = 2.0f * M_PI * center_freq_hz / fs;
  float alpha = sinf(w0) / (2.0f * q);

  float b0 = (1.0f + cosf(w0)) / (2.0f * (1.0f + alpha));
  float b1 = -(1.0f + cosf(w0)) / (1.0f + alpha);
  float b2 = (1.0f + cosf(w0)) / (2.0f * (1.0f + alpha));
  float a1 = -2.0f * cosf(w0) / (1.0f + alpha);
  float a2 = (1.0f - alpha) / (1.0f + alpha);

  f->b0 = b0; f->b1 = b1; f->b2 = b2;
  f->a1 = a1; f->a2 = a2;
  f->x1 = f->x2 = f->y1 = f->y2 = 0.0f;
}

static float notch_filter_apply(notch_filter_iir_t *f, float input) {
  float output = f->b0 * input + f->b1 * f->x1 + f->b2 * f->x2
                 - f->a1 * f->y1 - f->a2 * f->y2;
  f->x2 = f->x1; f->x1 = input;
  f->y2 = f->y1; f->y1 = output;
  return output;
}

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

static esp_err_t lsm6dsm_write_reg(uint8_t reg, uint8_t data) {
  if (dev_handle == NULL) {
    return ESP_ERR_INVALID_STATE;
  }

  uint8_t buf[2] = {reg, data};
  return i2c_master_transmit(dev_handle, buf, sizeof(buf), -1);
}

static esp_err_t lsm6dsm_read_regs(uint8_t reg, uint8_t *data, size_t len) {
  if (dev_handle == NULL) {
    return ESP_ERR_INVALID_STATE;
  }

  return i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, -1);
}

static void lsm6dsm_release_i2c(void) {
  if (dev_handle) {
    i2c_master_bus_rm_device(dev_handle);
    dev_handle = NULL;
  }

  if (bus_handle) {
    i2c_del_master_bus(bus_handle);
    bus_handle = NULL;
  }
}

static uint8_t lsm6dsm_accel_fs_bits(lsm6dsm_afs_t afs) {
  switch (afs) {
    case LSM6DSM_AFS_2G:
      return 0;
    case LSM6DSM_AFS_16G:
      return 1;
    case LSM6DSM_AFS_4G:
      return 2;
    case LSM6DSM_AFS_8G:
      return 3;
    default:
      return 0;
  }
}

static float lsm6dsm_accel_divisor(lsm6dsm_afs_t afs) {
  switch (afs) {
    case LSM6DSM_AFS_2G:
      return 16393.44f;  // 0.061 mg/LSB
    case LSM6DSM_AFS_4G:
      return 8196.72f;   // 0.122 mg/LSB
    case LSM6DSM_AFS_8G:
      return 4098.36f;   // 0.244 mg/LSB
    case LSM6DSM_AFS_16G:
      return 2049.18f;   // 0.488 mg/LSB
    default:
      return 16393.44f;
  }
}

static uint8_t lsm6dsm_gyro_fs_bits(lsm6dsm_gfs_t gfs, bool *fs_125) {
  *fs_125 = false;

  switch (gfs) {
    case LSM6DSM_GFS_125DPS:
      *fs_125 = true;
      return 0;
    case LSM6DSM_GFS_250DPS:
      return 0;
    case LSM6DSM_GFS_500DPS:
      return 1;
    case LSM6DSM_GFS_1000DPS:
      return 2;
    case LSM6DSM_GFS_2000DPS:
      return 3;
    default:
      return 3;
  }
}

static float lsm6dsm_gyro_divisor(lsm6dsm_gfs_t gfs) {
  switch (gfs) {
    case LSM6DSM_GFS_125DPS:
      return 228.57f;  // 4.375 mdps/LSB
    case LSM6DSM_GFS_250DPS:
      return 114.29f;  // 8.75 mdps/LSB
    case LSM6DSM_GFS_500DPS:
      return 57.14f;   // 17.50 mdps/LSB
    case LSM6DSM_GFS_1000DPS:
      return 28.57f;   // 35 mdps/LSB
    case LSM6DSM_GFS_2000DPS:
      return 14.29f;   // 70 mdps/LSB
    default:
      return 14.29f;
  }
}

static void lsm6dsm_sync_to_hal(void) {
  imu_data.accel_x = lsm6dsm_data.accel_x;
  imu_data.accel_y = lsm6dsm_data.accel_y;
  imu_data.accel_z = lsm6dsm_data.accel_z;
  imu_data.gyro_x = lsm6dsm_data.gyro_x;
  imu_data.gyro_y = lsm6dsm_data.gyro_y;
  imu_data.gyro_z = lsm6dsm_data.gyro_z;
  imu_data.gyro_x_offset = lsm6dsm_data.gyro_x_offset;
  imu_data.gyro_y_offset = lsm6dsm_data.gyro_y_offset;
  imu_data.gyro_z_offset = lsm6dsm_data.gyro_z_offset;
  imu_data.roll = lsm6dsm_data.roll;
  imu_data.pitch = lsm6dsm_data.pitch;
  imu_data.yaw = lsm6dsm_data.yaw;
  imu_data.calibration_progress = lsm6dsm_data.calibration_progress;
  imu_data.auto_lpf = lsm6dsm_data.auto_lpf;
  imu_data.auto_kp = lsm6dsm_data.auto_kp;
  memcpy(imu_data.calib_msg, lsm6dsm_data.calib_msg, sizeof(imu_data.calib_msg));
}

static esp_err_t lsm6dsm_apply_range(lsm6dsm_afs_t afs, lsm6dsm_aodr_t aodr,
                                     lsm6dsm_gfs_t gfs,
                                     lsm6dsm_godr_t godr) {
  // Configure accelerometer: CTRL1_XL (10h)
  // Bits [7:4]: ODR_XL[3:0], Bits [3:2]: FS_XL[1:0], Bit [1]: LPF1_BW_SEL, Bit [0]: BW0_XL
  uint8_t fs_xl = lsm6dsm_accel_fs_bits(afs);
  esp_err_t ret =
      lsm6dsm_write_reg(LSM6DSM_CTRL1_XL, ((uint8_t)aodr << 4) | (fs_xl << 2));
  if (ret != ESP_OK) {
    return ret;
  }

  // Configure gyroscope: CTRL2_G (11h)
  // Bits [7:4]: ODR_G[3:0], Bits [3:2]: FS_G[1:0], Bit [1]: FS_125
  bool fs_125 = false;
  uint8_t fs_g = lsm6dsm_gyro_fs_bits(gfs, &fs_125);
  ret = lsm6dsm_write_reg(LSM6DSM_CTRL2_G,
                          ((uint8_t)godr << 4) | (fs_g << 2) |
                              ((uint8_t)fs_125 << 1));
  if (ret != ESP_OK) {
    return ret;
  }

  // Set sensitivity divisors based on full-scale
  lsm6dsm_acc_ssvt = lsm6dsm_accel_divisor(afs);
  lsm6dsm_gyro_ssvt = lsm6dsm_gyro_divisor(gfs);

  return ESP_OK;
}

void lsm6dsm_set_range(lsm6dsm_afs_t afs, lsm6dsm_aodr_t aodr,
                       lsm6dsm_gfs_t gfs, lsm6dsm_godr_t godr) {
  esp_err_t ret = lsm6dsm_apply_range(afs, aodr, gfs, godr);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure range: %d", ret);
  }
}

/**
 * @brief Probe LSM6DSM at a specific I2C address by reading WHO_AM_I.
 * @return true if device responded and WHO_AM_I == 0x6A
 */
static bool lsm6dsm_probe_addr(uint8_t addr) {
  // Remove old device if any, then add at new address
  if (dev_handle) {
    i2c_master_bus_rm_device(dev_handle);
    dev_handle = NULL;
  }

  i2c_device_config_t dev_cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = addr,
      .scl_speed_hz = LSM6DSM_CLK_SPEED,
  };

  esp_err_t dev_err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
  if (dev_err != ESP_OK) {
    ESP_LOGW(TAG, "i2c_master_bus_add_device(0x%02X) failed: %d", addr, dev_err);
    return false;
  }

  uint8_t whoami = 0;
  esp_err_t ret = ESP_FAIL;
  for (int retry = 3; retry > 0; retry--) {
    ret = lsm6dsm_read_regs(LSM6DSM_WHO_AM_I, &whoami, 1);
    if (ret == ESP_OK && whoami == LSM6DSM_WHO_AM_I_VAL) {
      s_i2c_addr = addr;
      ESP_LOGI(TAG, "Probe addr 0x%02X -> WHO_AM_I = 0x%02X", addr, whoami);
      return true;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }

  ESP_LOGW(TAG, "Probe addr 0x%02X failed: err=%d WHO_AM_I=0x%02X", addr,
           ret, whoami);
  return false;
}

void lsm6dsm_init(void) {
  s_init_ok = false;

  ESP_LOGI(TAG, "Init LSM6DSM on I2C%d (SDA:%d, SCL:%d), probing 0x6A / 0x6B",
           LSM6DSM_I2C_PORT, LSM6DSM_PIN_SDA, LSM6DSM_PIN_SCL);

  // Create I2C master bus (port 1, shared with other IMUs)
  i2c_master_bus_config_t i2c_mst_config = {
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .i2c_port = LSM6DSM_I2C_PORT,
      .scl_io_num = LSM6DSM_PIN_SCL,
      .sda_io_num = LSM6DSM_PIN_SDA,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,  // Enable internal pull-ups as fallback
  };

  esp_err_t bus_err = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
  if (bus_err != ESP_OK) {
    ESP_LOGE(TAG, "FAILED to create I2C bus! err=%d. Check SDA(%d)/SCL(%d) pins",
             bus_err, LSM6DSM_PIN_SDA, LSM6DSM_PIN_SCL);
    bus_handle = NULL;
    return;
  }

  // Try both possible addresses (depends on SA0 pin level)
  uint8_t found_addr = 0;
  if (lsm6dsm_probe_addr(LSM6DSM_I2C_ADDR_LOW)) {
    found_addr = LSM6DSM_I2C_ADDR_LOW;
  } else if (lsm6dsm_probe_addr(LSM6DSM_I2C_ADDR_HIGH)) {
    found_addr = LSM6DSM_I2C_ADDR_HIGH;
  }

  if (found_addr == 0) {
    ESP_LOGE(TAG, "LSM6DSM NOT DETECTED at 0x%02X or 0x%02X!", LSM6DSM_I2C_ADDR_LOW, LSM6DSM_I2C_ADDR_HIGH);
    ESP_LOGE(TAG, "Check hardware:");
    ESP_LOGE(TAG, "  1. VDD (pin 8) -> 3.3V, VDDIO (pin 9) -> 3.3V, GND (pin 6/7) -> GND");
    ESP_LOGE(TAG, "  2. SDA (GPIO%d) <-> LSM6DSM pin 14, SCL (GPIO%d) <-> LSM6DSM pin 13",
             LSM6DSM_PIN_SDA, LSM6DSM_PIN_SCL);
    ESP_LOGE(TAG, "  3. 4.7kΩ pull-up resistors on SDA and SCL to 3.3V (internal pull-up enabled as fallback)");
    ESP_LOGE(TAG, "  4. CS (pin 12) -> 3.3V (for I2C mode). DO NOT leave floating!");
    ESP_LOGE(TAG, "  5. SA0 (pin 1) -> GND (addr=0x%02X) OR 3.3V (addr=0x%02X)",
             LSM6DSM_I2C_ADDR_LOW, LSM6DSM_I2C_ADDR_HIGH);
    lsm6dsm_release_i2c();
    return;
  }

  ESP_LOGI(TAG, "LSM6DSM detected at 0x%02X", found_addr);

  vTaskDelay(pdMS_TO_TICKS(10));

  // Software reset
  esp_err_t ret = lsm6dsm_write_reg(LSM6DSM_CTRL3_C, LSM6DSM_CTRL3_C_SW_RESET);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Software reset failed: %d", ret);
    lsm6dsm_release_i2c();
    return;
  }
  vTaskDelay(pdMS_TO_TICKS(50));

  uint8_t whoami = 0;
  ret = lsm6dsm_read_regs(LSM6DSM_WHO_AM_I, &whoami, 1);
  if (ret != ESP_OK || whoami != LSM6DSM_WHO_AM_I_VAL) {
    ESP_LOGE(TAG, "WHO_AM_I check after reset failed: err=%d value=0x%02X",
             ret, whoami);
    lsm6dsm_release_i2c();
    return;
  }

  // BDU=1 keeps L/H output bytes coherent; IF_INC=1 enables burst reads over I2C.
  ret = lsm6dsm_write_reg(LSM6DSM_CTRL3_C,
                          LSM6DSM_CTRL3_C_BDU | LSM6DSM_CTRL3_C_IF_INC);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure CTRL3_C: %d", ret);
    lsm6dsm_release_i2c();
    return;
  }
  vTaskDelay(pdMS_TO_TICKS(10));

  ret = lsm6dsm_write_reg(LSM6DSM_CTRL4_C, 0x00);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to keep I2C enabled in CTRL4_C: %d", ret);
    lsm6dsm_release_i2c();
    return;
  }

  // Default range: ±2g, 416Hz accel; ±2000dps, 416Hz gyro
  ret = lsm6dsm_apply_range(LSM6DSM_AFS_2G, LSM6DSM_AODR_416HZ,
                            LSM6DSM_GFS_2000DPS, LSM6DSM_GODR_416HZ);
  if (ret != ESP_OK) {
    ESP_LOGE(TAG, "Failed to configure default accel/gyro range: %d", ret);
    lsm6dsm_release_i2c();
    return;
  }
  vTaskDelay(pdMS_TO_TICKS(50));

  s_init_ok = true;
  ESP_LOGI(TAG, "LSM6DSM ready on I2C address 0x%02X", s_i2c_addr);

  // Initialize filter buffers
  for (int i = 0; i < FILTER_BUF_LEN; i++) {
    gyro_x_buf[i] = 0.0f;
    gyro_y_buf[i] = 0.0f;
    gyro_z_buf[i] = 0.0f;
  }

  // Step 3: 初始化加速度计陷波滤波器
  // 中心频率: 250Hz (PWM 500Hz的二次谐波，机械共振常见频率), Q值: 8
  notch_filter_init(&acc_notch_x, 250.0f, 8.0f, motor_settings.frequency_hz);
  notch_filter_init(&acc_notch_y, 250.0f, 8.0f, motor_settings.frequency_hz);
  notch_filter_init(&acc_notch_z, 250.0f, 8.0f, motor_settings.frequency_hz);

  imu_fusion_init();
  offset_lsm6dsm_nvs_load();

  // Apply loaded or default band-pass filters
  for (int i = 0; i < 3; i++) {
    gyro_bpf_init(&gyro_filters[i], lsm6dsm_data.auto_lpf, 0.1f,
                  motor_settings.frequency_hz);
  }
  imu_fusion_set_gains(lsm6dsm_data.auto_kp, 0.0f);
}

void lsm6dsm_calibration(int samples) {
  if (!s_init_ok) {
    ESP_LOGE(TAG, "Cannot calibrate: LSM6DSM is not initialized");
    return;
  }

  if (samples <= 0) {
    ESP_LOGE(TAG, "Cannot calibrate: invalid sample count %d", samples);
    return;
  }

  ESP_LOGI(TAG, "Starting LSM6DSM calibration and auto-tuning...");

  bool previous_motor_state = is_motor_enabled;
  motor_set_enable(false);

  float gx_sum = 0, gy_sum = 0, gz_sum = 0;
  float gx_sq_sum = 0, gy_sq_sum = 0, gz_sq_sum = 0;

  lsm6dsm_data.gyro_x_offset = 0;
  lsm6dsm_data.gyro_y_offset = 0;
  lsm6dsm_data.gyro_z_offset = 0;
  lsm6dsm_data.calibration_progress = 0;

  int valid_samples = 0;
  for (int i = 0; i < samples; i++) {
    if (lsm6dsm_read_data(NULL, NULL) != ESP_OK) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }

    float gx = lsm6dsm_data.gyro_x;
    float gy = lsm6dsm_data.gyro_y;
    float gz = lsm6dsm_data.gyro_z;

    gx_sum += gx;
    gy_sum += gy;
    gz_sum += gz;

    gx_sq_sum += gx * gx;
    gy_sq_sum += gy * gy;
    gz_sq_sum += gz * gz;

    valid_samples++;
    lsm6dsm_data.calibration_progress = ((i + 1) * 100) / samples;
    vTaskDelay(pdMS_TO_TICKS(1));
  }

  if (valid_samples == 0) {
    lsm6dsm_data.calibration_progress = 0;
    snprintf(lsm6dsm_data.calib_msg, sizeof(lsm6dsm_data.calib_msg),
             "Calibration failed");
    ESP_LOGE(TAG, "Calibration failed: no valid I2C samples");
    lsm6dsm_sync_to_hal();
    motor_set_enable(previous_motor_state);
    return;
  }

  lsm6dsm_data.gyro_x_offset = gx_sum / valid_samples;
  lsm6dsm_data.gyro_y_offset = gy_sum / valid_samples;
  lsm6dsm_data.gyro_z_offset = gz_sum / valid_samples;

  // Calculate standard deviation (noise level)
  float gx_var = (gx_sq_sum / valid_samples) -
                 (lsm6dsm_data.gyro_x_offset * lsm6dsm_data.gyro_x_offset);
  float gy_var = (gy_sq_sum / valid_samples) -
                 (lsm6dsm_data.gyro_y_offset * lsm6dsm_data.gyro_y_offset);
  float gz_var = (gz_sq_sum / valid_samples) -
                 (lsm6dsm_data.gyro_z_offset * lsm6dsm_data.gyro_z_offset);

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

  lsm6dsm_data.auto_lpf = dynamic_lpf;
  lsm6dsm_data.auto_kp = 0.0f;
  snprintf(lsm6dsm_data.calib_msg, sizeof(lsm6dsm_data.calib_msg),
           "LPF:%.1fHz Kalman", dynamic_lpf);

  lsm6dsm_data.calibration_progress = 100;

  ESP_LOGI(TAG, "Calibration done. Offsets: X=%.2f Y=%.2f Z=%.2f",
           lsm6dsm_data.gyro_x_offset, lsm6dsm_data.gyro_y_offset,
           lsm6dsm_data.gyro_z_offset);

  offset_lsm6dsm_nvs_save();

  // Reset fusion and internal data to start from 0
  imu_fusion_init();
  lsm6dsm_data.roll = 0;
  lsm6dsm_data.pitch = 0;
  lsm6dsm_data.yaw = 0;
  lsm6dsm_data.gyro_x = 0;
  lsm6dsm_data.gyro_y = 0;
  lsm6dsm_data.gyro_z = 0;

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

  lsm6dsm_sync_to_hal();
  motor_set_enable(previous_motor_state);
}

esp_err_t lsm6dsm_read_data(float *accel_data, float *gyro_data) {
  if (!s_init_ok) {
    return ESP_ERR_INVALID_STATE;
  }

  // Read gyro first (OUTX_L_G at 0x22, 6 bytes) then accel (OUTX_L_XL at 0x28, 6 bytes)
  // But we can read all 12 bytes in one burst starting from gyro X low
  uint8_t raw[12];
  esp_err_t ret = lsm6dsm_read_regs(LSM6DSM_OUTX_L_G, raw, 12);
  if (ret != ESP_OK) {
    return ret;
  }

  // Parse Gyroscope (regs 0x22-0x27)
  int16_t gx = (int16_t)(((uint16_t)raw[1] << 8) | raw[0]);
  int16_t gy = (int16_t)(((uint16_t)raw[3] << 8) | raw[2]);
  int16_t gz = (int16_t)(((uint16_t)raw[5] << 8) | raw[4]);

  lsm6dsm_data.gyro_x = gx / lsm6dsm_gyro_ssvt - lsm6dsm_data.gyro_x_offset;
  lsm6dsm_data.gyro_y = gy / lsm6dsm_gyro_ssvt - lsm6dsm_data.gyro_y_offset;
  lsm6dsm_data.gyro_z = gz / lsm6dsm_gyro_ssvt - lsm6dsm_data.gyro_z_offset;

  // Parse Accelerometer (regs 0x28-0x2D)
  int16_t ax = (int16_t)(((uint16_t)raw[7] << 8) | raw[6]);
  int16_t ay = (int16_t)(((uint16_t)raw[9] << 8) | raw[8]);
  int16_t az = (int16_t)(((uint16_t)raw[11] << 8) | raw[10]);

  lsm6dsm_data.accel_x = ax / lsm6dsm_acc_ssvt;
  lsm6dsm_data.accel_y = ay / lsm6dsm_acc_ssvt;
  lsm6dsm_data.accel_z = az / lsm6dsm_acc_ssvt;

  if (accel_data) {
    accel_data[0] = lsm6dsm_data.accel_x;
    accel_data[1] = lsm6dsm_data.accel_y;
    accel_data[2] = lsm6dsm_data.accel_z;
  }

  if (gyro_data) {
    gyro_data[0] = lsm6dsm_data.gyro_x;
    gyro_data[1] = lsm6dsm_data.gyro_y;
    gyro_data[2] = lsm6dsm_data.gyro_z;
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

void lsm6dsm_read_task(void *arg) {
  if (!s_init_ok) {
    ESP_LOGE(TAG, "LSM6DSM init failed, read task will not run.");
    vTaskDelete(NULL);
    return;
  }

  TickType_t xLastWakeTime = xTaskGetTickCount();
  const TickType_t xFrequency =
      pdMS_TO_TICKS(1000 / motor_settings.frequency_hz);
  float dt = 1.0f / motor_settings.frequency_hz;

  for (;;) {
    if (lsm6dsm_read_data(NULL, NULL) != ESP_OK) {
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    float ma_gx = filter_apply(gyro_x_buf, lsm6dsm_data.gyro_x);
    float ma_gy = filter_apply(gyro_y_buf, lsm6dsm_data.gyro_y);
    float ma_gz = filter_apply(gyro_z_buf, lsm6dsm_data.gyro_z);

    filter_idx++;
    if (filter_idx >= FILTER_BUF_LEN) filter_idx = 0;

    lsm6dsm_data.gyro_x = gyro_bpf_apply(&gyro_filters[0], ma_gx);
    lsm6dsm_data.gyro_y = gyro_bpf_apply(&gyro_filters[1], ma_gy);
    lsm6dsm_data.gyro_z = gyro_bpf_apply(&gyro_filters[2], ma_gz);

    // Step 3: 加速度计陷波滤波 - 抑制电机PWM谐波耦合的振动
    float acc_x_notched = notch_filter_apply(&acc_notch_x, lsm6dsm_data.accel_x);
    float acc_y_notched = notch_filter_apply(&acc_notch_y, lsm6dsm_data.accel_y);
    float acc_z_notched = notch_filter_apply(&acc_notch_z, lsm6dsm_data.accel_z);

    imu_fusion_update(acc_x_notched, acc_y_notched,
                      acc_z_notched, lsm6dsm_data.gyro_x,
                      lsm6dsm_data.gyro_y, lsm6dsm_data.gyro_z, dt);

    imu_fusion_get_euler(&lsm6dsm_data.roll, &lsm6dsm_data.pitch,
                         &lsm6dsm_data.yaw);

    lsm6dsm_sync_to_hal();

    vTaskDelayUntil(&xLastWakeTime, xFrequency);
  }
}

void offset_lsm6dsm_nvs_init(void) {
  esp_err_t err = nvs_init_custom();
  if (err == ESP_OK) {
    offset_lsm6dsm_nvs_load();
  } else {
    ESP_LOGE(TAG, "NVS Init Failed");
  }
}

void offset_lsm6dsm_nvs_save(void) {
  nvs_write_float("imu_ox", lsm6dsm_data.gyro_x_offset);
  nvs_write_float("imu_oy", lsm6dsm_data.gyro_y_offset);
  nvs_write_float("imu_oz", lsm6dsm_data.gyro_z_offset);
  nvs_write_float("imu_lpf", lsm6dsm_data.auto_lpf);
  nvs_write_float("imu_kp", lsm6dsm_data.auto_kp);
  ESP_LOGI(TAG, "IMU parameters saved to NVS");
}

void offset_lsm6dsm_nvs_load(void) {
  if (nvs_read_float("imu_ox", &lsm6dsm_data.gyro_x_offset) != ESP_OK)
    lsm6dsm_data.gyro_x_offset = 0.0f;
  if (nvs_read_float("imu_oy", &lsm6dsm_data.gyro_y_offset) != ESP_OK)
    lsm6dsm_data.gyro_y_offset = 0.0f;
  if (nvs_read_float("imu_oz", &lsm6dsm_data.gyro_z_offset) != ESP_OK)
    lsm6dsm_data.gyro_z_offset = 0.0f;

  if (nvs_read_float("imu_lpf", &lsm6dsm_data.auto_lpf) != ESP_OK)
    lsm6dsm_data.auto_lpf = 30.0f;
  if (nvs_read_float("imu_kp", &lsm6dsm_data.auto_kp) != ESP_OK)
    lsm6dsm_data.auto_kp = 0.0f;

  snprintf(lsm6dsm_data.calib_msg, sizeof(lsm6dsm_data.calib_msg),
           "LPF:%.1fHz Kalman", lsm6dsm_data.auto_lpf);

  ESP_LOGI(TAG,
           "IMU parameters loaded: Offsets(%.2f, %.2f, %.2f) LPF:%.1f Kalman",
           lsm6dsm_data.gyro_x_offset, lsm6dsm_data.gyro_y_offset,
           lsm6dsm_data.gyro_z_offset, lsm6dsm_data.auto_lpf);
}
