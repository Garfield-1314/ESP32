#ifndef _LSM6DSM_H_
#define _LSM6DSM_H_

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Configuration ==================== */
/* Set to 1 to enable LIS2MDL magnetometer (9-axis mode).
   Requires LIS2MDL connected to LSM6DSM auxiliary I2C bus. */
#define LSM6DSM_USE_MAGNETOMETER   1

/* ==================== I2C Configuration ==================== */
/* Modify these macros to match your hardware wiring */
#define LSM6DSM_I2C_PORT     0
#define LSM6DSM_PIN_SDA      17
#define LSM6DSM_PIN_SCL      18
#define LSM6DSM_CLK_SPEED    400000

/* ==================== I2C Address ==================== */
/* SA0 = GND → 0x6A, SA0 = VCC → 0x6B */
#define LSM6DSM_I2C_ADDR_LOW   0x6A
#define LSM6DSM_I2C_ADDR_HIGH  0x6B

#define LIS2MDL_I2C_ADDR       0x1E   /* 7-bit, fixed address */

/* ==================== Registers ==================== */
#define LSM6DSM_WHO_AM_I      0x0F
#define LSM6DSM_WHO_AM_I_VAL  0x6A
#define LSM6DSM_CTRL1_XL      0x10  /* Accelerometer control */
#define LSM6DSM_CTRL2_G       0x11  /* Gyroscope control */
#define LSM6DSM_CTRL3_C       0x12  /* Control register 3 */
#define LSM6DSM_CTRL4_C       0x13  /* Control register 4 */
#define LSM6DSM_CTRL6_C       0x15  /* Control register 6 */
#define LSM6DSM_CTRL7_G       0x16  /* Control register 7 (gyro) */
#define LSM6DSM_CTRL8_XL      0x17  /* Control register 8 (accel) */
#define LSM6DSM_OUTX_L_G      0x22  /* Gyroscope X low byte */
#define LSM6DSM_OUTX_H_G      0x23
#define LSM6DSM_OUTY_L_G      0x24
#define LSM6DSM_OUTY_H_G      0x25
#define LSM6DSM_OUTZ_L_G      0x26
#define LSM6DSM_OUTZ_H_G      0x27
#define LSM6DSM_OUTX_L_XL     0x28  /* Accelerometer X low byte */
#define LSM6DSM_OUTX_H_XL     0x29
#define LSM6DSM_OUTY_L_XL     0x2A
#define LSM6DSM_OUTY_H_XL     0x2B
#define LSM6DSM_OUTZ_L_XL     0x2C
#define LSM6DSM_OUTZ_H_XL     0x2D

/* 9-axis / Sensor Hub registers (only used when LSM6DSM_USE_MAGNETOMETER=1) */
#define LSM6DSM_MASTER_CONFIG   0x1A
#define LSM6DSM_CTRL10_C        0x19
#define LSM6DSM_FUNC_CFG_ACCESS 0x01
#define LSM6DSM_FUNC_CFG_EN     0x20
#define LSM6DSM_SLV0_ADD        0x02
#define LSM6DSM_SLV0_SUBADD     0x03
#define LSM6DSM_SLAVE0_CONFIG   0x04
#define LSM6DSM_SENSORHUB1_REG  0x2E
#define LSM6DSM_FUNC_SRC2       0x54
#define LSM6DSM_MASTER_MASTER_ON    0x01
#define LSM6DSM_MASTER_PASS_THROUGH 0x04
#define LSM6DSM_MASTER_PULL_UP_EN   0x08

/* LIS2MDL registers */
#define LIS2MDL_WHO_AM_I       0x4F
#define LIS2MDL_WHO_AM_I_VAL   0x40
#define LIS2MDL_CFG_REG_A      0x60
#define LIS2MDL_CFG_REG_C      0x62
#define LIS2MDL_OUTX_L_REG     0x68
#define LIS2MDL_SENSITIVITY    1.5f
#define LIS2MDL_MD_CONTINUOUS  0x00
#define LIS2MDL_ODR_100HZ      0x30
#define LIS2MDL_CFG_A_LPF      0x08
#define LIS2MDL_CFG_A_SOFT_RST 0x04
#define LIS2MDL_CFG_C_BDU      0x02

/* CTRL3_C bits */
#define LSM6DSM_CTRL3_C_SW_RESET 0x01
#define LSM6DSM_CTRL3_C_IF_INC   0x04
#define LSM6DSM_CTRL3_C_BDU      0x40

/* ==================== Enumerations ==================== */

/* Accelerometer full-scale selection */
typedef enum {
    LSM6DSM_AFS_2G  = 0,
    LSM6DSM_AFS_4G  = 1,
    LSM6DSM_AFS_8G  = 2,
    LSM6DSM_AFS_16G = 3
} lsm6dsm_afs_t;

/* Accelerometer ODR */
typedef enum {
    LSM6DSM_AODR_POWER_DOWN = 0,
    LSM6DSM_AODR_12_5HZ     = 1,
    LSM6DSM_AODR_26HZ       = 2,
    LSM6DSM_AODR_52HZ       = 3,
    LSM6DSM_AODR_104HZ      = 4,
    LSM6DSM_AODR_208HZ      = 5,
    LSM6DSM_AODR_416HZ      = 6,
    LSM6DSM_AODR_833HZ      = 7,
    LSM6DSM_AODR_1660HZ     = 8,
    LSM6DSM_AODR_3330HZ     = 9,
    LSM6DSM_AODR_6660HZ     = 10,
} lsm6dsm_aodr_t;

/* Gyroscope full-scale selection */
typedef enum {
    LSM6DSM_GFS_125DPS   = 0,
    LSM6DSM_GFS_250DPS   = 1,
    LSM6DSM_GFS_500DPS   = 2,
    LSM6DSM_GFS_1000DPS  = 3,
    LSM6DSM_GFS_2000DPS  = 4,
} lsm6dsm_gfs_t;

/* Gyroscope ODR */
typedef enum {
    LSM6DSM_GODR_POWER_DOWN = 0,
    LSM6DSM_GODR_12_5HZ     = 1,
    LSM6DSM_GODR_26HZ       = 2,
    LSM6DSM_GODR_52HZ       = 3,
    LSM6DSM_GODR_104HZ      = 4,
    LSM6DSM_GODR_208HZ      = 5,
    LSM6DSM_GODR_416HZ      = 6,
    LSM6DSM_GODR_833HZ      = 7,
    LSM6DSM_GODR_1660HZ     = 8,
    LSM6DSM_GODR_3330HZ     = 9,
    LSM6DSM_GODR_6660HZ     = 10,
} lsm6dsm_godr_t;

/* ==================== Data Structure ==================== */
typedef struct {
    float accel_x;       /* g */
    float accel_y;       /* g */
    float accel_z;       /* g */
    float gyro_x;        /* degrees per second */
    float gyro_y;        /* degrees per second */
    float gyro_z;        /* degrees per second */
    float mag_x;         /* mGauss (0 if magnetometer not available) */
    float mag_y;         /* mGauss */
    float mag_z;         /* mGauss */
    float temperature;   /* degrees Celsius */
} lsm6dsm_all_data_t;

/* ==================== API ==================== */

/**
 * @brief Initialize LSM6DSM on I2C bus.
 *        Probes both possible addresses, resets the device,
 *        enables BDU and IF_INC, sets default range.
 *        If LSM6DSM_USE_MAGNETOMETER=1, also configures LIS2MDL
 *        via Sensor Hub.
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lsm6dsm_init(void);

/**
 * @brief Reconfigure accelerometer and gyroscope range/ODR.
 */
void lsm6dsm_set_range(lsm6dsm_afs_t afs, lsm6dsm_aodr_t aodr,
                       lsm6dsm_gfs_t gfs, lsm6dsm_godr_t godr);

/**
 * @brief Read 6-axis data (accelerometer + gyroscope).
 *        Legacy function for backward compatibility.
 * @param accel_data If non-NULL, filled with [ax, ay, az] in g
 * @param gyro_data  If non-NULL, filled with [gx, gy, gz] in dps
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lsm6dsm_read_data(float *accel_data, float *gyro_data);

/**
 * @brief Read all available data (accel + gyro + mag + temperature).
 *        If magnetometer is not available, mag fields are set to 0.
 * @param data  Output data
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lsm6dsm_read_all(lsm6dsm_all_data_t *data);

/**
 * @brief Read temperature from LSM6DSM.
 * @param temp_c  Output: temperature in degrees Celsius
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lsm6dsm_read_temperature(float *temp_c);

#ifdef __cplusplus
}
#endif

#endif /* _LSM6DSM_H_ */