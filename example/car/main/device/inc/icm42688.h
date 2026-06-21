#ifndef _ICM42688_H_
#define _ICM42688_H_

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ICM42688 I2C Configuration
#define IMU_I2C_PORT 1
#define IMU_PIN_SDA 17
#define IMU_PIN_SCL 18
#define IMU_I2C_ADDR 0x68  // Default I2C address for ICM42688
#define IMU_CLK_SPEED 400000

// ICM42688 registers (Bank 0)
#define ICM42688_DEVICE_CONFIG 0x11
#define ICM42688_DRIVE_CONFIG 0x13
#define ICM42688_INT_CONFIG 0x14
#define ICM42688_FIFO_CONFIG 0x16
#define ICM42688_TEMP_DATA1 0x1D
#define ICM42688_TEMP_DATA0 0x1E
#define ICM42688_ACCEL_DATA_X1 0x1F
#define ICM42688_ACCEL_DATA_X0 0x20
#define ICM42688_ACCEL_DATA_Y1 0x21
#define ICM42688_ACCEL_DATA_Y0 0x22
#define ICM42688_ACCEL_DATA_Z1 0x23
#define ICM42688_ACCEL_DATA_Z0 0x24
#define ICM42688_GYRO_DATA_X1 0x25
#define ICM42688_GYRO_DATA_X0 0x26
#define ICM42688_GYRO_DATA_Y1 0x27
#define ICM42688_GYRO_DATA_Y0 0x28
#define ICM42688_GYRO_DATA_Z1 0x29
#define ICM42688_GYRO_DATA_Z0 0x2A
#define ICM42688_TMST_FSYNCH 0x2B
#define ICM42688_TMST_FSYNCL 0x2C
#define ICM42688_INT_STATUS 0x2D
#define ICM42688_FIFO_COUNTH 0x2E
#define ICM42688_FIFO_COUNTL 0x2F
#define ICM42688_FIFO_DATA 0x30
#define ICM42688_INT_STATUS2 0x37
#define ICM42688_INT_STATUS3 0x38
#define ICM42688_SIGNAL_PATH_RESET 0x4B
#define ICM42688_INTF_CONFIG0 0x4C
#define ICM42688_INTF_CONFIG1 0x4D
#define ICM42688_PWR_MGMT0 0x4E
#define ICM42688_GYRO_CONFIG0 0x4F
#define ICM42688_ACCEL_CONFIG0 0x50
#define ICM42688_GYRO_CONFIG1 0x51
#define ICM42688_GYRO_ACCEL_CONFIG0 0x52
#define ICM42688_ACCEL_CONFIG1 0x53
#define ICM42688_WHO_AM_I 0x75

typedef enum {
  ICM42688_AFS_16G = 0,
  ICM42688_AFS_8G,
  ICM42688_AFS_4G,
  ICM42688_AFS_2G
} icm42688_afs_t;

typedef enum {
  ICM42688_AODR_32000HZ = 1,
  ICM42688_AODR_16000HZ,
  ICM42688_AODR_8000HZ,
  ICM42688_AODR_4000HZ,
  ICM42688_AODR_2000HZ,
  ICM42688_AODR_1000HZ,
  ICM42688_AODR_200HZ,
  ICM42688_AODR_100HZ,
  ICM42688_AODR_50HZ,
  ICM42688_AODR_25HZ,
  ICM42688_AODR_12_5HZ,
  ICM42688_AODR_6_25HZ,
  ICM42688_AODR_3_125HZ,
  ICM42688_AODR_1_5625HZ,
  ICM42688_AODR_500HZ
} icm42688_aodr_t;

typedef enum {
  ICM42688_GFS_2000DPS = 0,
  ICM42688_GFS_1000DPS,
  ICM42688_GFS_500DPS,
  ICM42688_GFS_250DPS,
  ICM42688_GFS_125DPS,
  ICM42688_GFS_62_5DPS,
  ICM42688_GFS_31_25DPS,
  ICM42688_GFS_15_625DPS
} icm42688_gfs_t;

typedef enum {
  ICM42688_GODR_32000HZ = 1,
  ICM42688_GODR_16000HZ,
  ICM42688_GODR_8000HZ,
  ICM42688_GODR_4000HZ,
  ICM42688_GODR_2000HZ,
  ICM42688_GODR_1000HZ,
  ICM42688_GODR_200HZ,
  ICM42688_GODR_100HZ,
  ICM42688_GODR_50HZ,
  ICM42688_GODR_25HZ,
  ICM42688_GODR_12_5HZ,
  ICM42688_GODR_500HZ = 0xF
} icm42688_godr_t;

#define ICM42688_TMST_CONFIG 0x54
#define ICM42688_FIFO_CONFIG1 0x5F
#define ICM42688_FIFO_CONFIG2 0x60
#define ICM42688_FIFO_CONFIG3 0x61
#define ICM42688_FSYNC_CONFIG 0x62
#define ICM42688_INT_CONFIG0 0x63
#define ICM42688_INT_CONFIG1 0x64
#define ICM42688_INT_SOURCE0 0x65
#define ICM42688_INT_SOURCE1 0x66
#define ICM42688_INT_SOURCE3 0x68
#define ICM42688_INT_SOURCE4 0x69
#define ICM42688_FIFO_LOST_PKT0 0x6C
#define ICM42688_FIFO_LOST_PKT1 0x6D
#define ICM42688_SELF_TEST_CONFIG 0x70
#define ICM42688_WHO_AM_I 0x75
#define ICM42688_REG_BANK_SEL 0x76

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
} icm42688_data_t;

extern icm42688_data_t icm42688_data;

void icm42688_init(void);
void icm42688_set_range(icm42688_afs_t afs, icm42688_aodr_t aodr,
                        icm42688_gfs_t gfs, icm42688_godr_t godr);
esp_err_t icm42688_read_data(float *accel_data, float *gyro_data);
void icm42688_read_task(void *arg);
void icm42688_calibration(int samples);

void offset_icm42688_nvs_init(void);
void offset_icm42688_nvs_save(void);
void offset_icm42688_nvs_load(void);

#endif /* _ICM42688_H_ */
