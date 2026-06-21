#ifndef _MPU6050_H_
#define _MPU6050_H_

#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// MPU6050 I2C Configuration (same pins as ICM42688)
#define MPU6050_I2C_PORT 1
#define MPU6050_PIN_SDA 17
#define MPU6050_PIN_SCL 18
#define MPU6050_CLK_SPEED 400000

// MPU6050 I2C address selection via AD0 pin:
//   AD0 = LOW  (GND or floating/internal pull-down)  → 0x68  (7-bit)
//   AD0 = HIGH (to VCC 3.3V)                        → 0x69  (7-bit)
// The AD0 pin has an internal pull-down resistor, so floating/unconnected = 0x68.
#define MPU6050_I2C_ADDR_LOW  0x68  // AD0 = GND or floating
#define MPU6050_I2C_ADDR_HIGH 0x69  // AD0 = VCC
#define MPU6050_I2C_ADDR MPU6050_I2C_ADDR_LOW  // Default: AD0 = LOW

// MPU6050 Registers
#define MPU6050_SMPLRT_DIV 0x19
#define MPU6050_CONFIG      0x1A
#define MPU6050_GYRO_CONFIG 0x1B
#define MPU6050_ACCEL_CONFIG 0x1C
#define MPU6050_INT_PIN_CFG 0x37
#define MPU6050_INT_ENABLE  0x38
#define MPU6050_ACCEL_XOUT_H 0x3B
#define MPU6050_ACCEL_XOUT_L 0x3C
#define MPU6050_ACCEL_YOUT_H 0x3D
#define MPU6050_ACCEL_YOUT_L 0x3E
#define MPU6050_ACCEL_ZOUT_H 0x3F
#define MPU6050_ACCEL_ZOUT_L 0x40
#define MPU6050_TEMP_OUT_H  0x41
#define MPU6050_TEMP_OUT_L  0x42
#define MPU6050_GYRO_XOUT_H 0x43
#define MPU6050_GYRO_XOUT_L 0x44
#define MPU6050_GYRO_YOUT_H 0x45
#define MPU6050_GYRO_YOUT_L 0x46
#define MPU6050_GYRO_ZOUT_H 0x47
#define MPU6050_GYRO_ZOUT_L 0x48
#define MPU6050_PWR_MGMT_1  0x6B
#define MPU6050_PWR_MGMT_2  0x6C
#define MPU6050_WHO_AM_I    0x75

// DLPF configuration (in CONFIG register, bits 2:0)
#define MPU6050_DLPF_260HZ  0x00  // Accel BW: 260Hz, Gyro BW: 256Hz, Delay: 0.98ms
#define MPU6050_DLPF_184HZ  0x01  // Accel BW: 184Hz, Gyro BW: 188Hz, Delay: 1.88ms
#define MPU6050_DLPF_94HZ   0x02  // Accel BW: 94Hz,  Gyro BW: 98Hz,  Delay: 2.88ms
#define MPU6050_DLPF_44HZ   0x03  // Accel BW: 44Hz,  Gyro BW: 42Hz,  Delay: 4.87ms
#define MPU6050_DLPF_21HZ   0x04  // Accel BW: 21Hz,  Gyro BW: 20Hz,  Delay: 8.87ms
#define MPU6050_DLPF_10HZ   0x05  // Accel BW: 10Hz,  Gyro BW: 10Hz,  Delay: 13.80ms
#define MPU6050_DLPF_5HZ    0x06  // Accel BW: 5Hz,   Gyro BW: 5Hz,   Delay: 19.00ms

typedef enum {
    MPU6050_AFS_2G  = 0,
    MPU6050_AFS_4G  = 1,
    MPU6050_AFS_8G  = 2,
    MPU6050_AFS_16G = 3
} mpu6050_afs_t;

typedef enum {
    MPU6050_GFS_250DPS  = 0,
    MPU6050_GFS_500DPS  = 1,
    MPU6050_GFS_1000DPS = 2,
    MPU6050_GFS_2000DPS = 3
} mpu6050_gfs_t;

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
} mpu6050_data_t;

extern mpu6050_data_t mpu6050_data;

void mpu6050_init(void);
void mpu6050_set_range(mpu6050_afs_t afs, mpu6050_gfs_t gfs);
void mpu6050_set_dlpf(uint8_t dlpf_cfg);
void mpu6050_set_sample_rate(uint8_t div);
esp_err_t mpu6050_read_data(float *accel_data, float *gyro_data);
void mpu6050_read_task(void *arg);
void mpu6050_calibration(int samples);

void offset_mpu6050_nvs_init(void);
void offset_mpu6050_nvs_save(void);
void offset_mpu6050_nvs_load(void);

#endif /* _MPU6050_H_ */