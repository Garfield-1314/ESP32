/**
 * @file lsm6dsm_9axis.h
 * @brief Complete 9-axis driver using LSM6DSMTR Mode 2 (Sensor Hub)
 *
 * Architecture (Mode 2):
 *   ┌──────────┐   I2C   ┌──────────┐  Aux I2C  ┌──────────┐
 *   │  ESP32   │◄───────►│ LSM6DSM │◄─────────►│ LIS2MDL │
 *   │  (Host)  │         │ (Master)│  (Sensor  │ (Mag)   │
 *   └──────────┘           │  + Accel│   Hub)   └──────────┘
 *                           │  + Gyro │
 *                          └──────────┘
 *
 * The LSM6DSM acts as both:
 *   - I2C slave (to host MCU) for accel/gyro/temperature
 *   - I2C master (Sensor Hub, Mode 2) reading LIS2MDL magnetometer
 *     automatically.  Magnetometer data appears in the LSM6DSM's
 *     SENSORHUB output registers (0x2E-0x33).
 *
 * The host MCU reads all 9 axes from a single device (LSM6DSM).
 *
 * Usage:
 *   lsm6dsm_9axis_init();         // Init LSM6DSM + configure sensor hub
 *   lsm6dsm_9axis_read(&data);    // Read accel + gyro + mag
 */

#ifndef _LSM6DSM_9AXIS_H_
#define _LSM6DSM_9AXIS_H_

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== Key Register Extensions ==================== */
/*
 * Registers already defined in lsm6dsm.h:
 *   0x0F  WHO_AM_I
 *   0x10  CTRL1_XL
 *   0x11  CTRL2_G
 *   0x12  CTRL3_C
 *   0x15  CTRL6_C
 *   0x16  CTRL7_G
 *   0x17  CTRL8_XL
 *   0x22-0x2D  Gyro & Accel output registers
 *
 * Sensor Hub / Mode 2 registers (NEW):
 *   0x01  FUNC_CFG_ACCESS   — page select for embedded functions
 *   0x02  SLV0_ADD          — Slave 0 address (LIS2MDL)
 *   0x03  SLV0_SUBADD       — Slave 0 register to read
 *   0x04  SLAVE0_CONFIG     — Slave 0 config (bytes, rate)
 *   0x1A  MASTER_CONFIG     — Sensor hub master config
 *   0x2E-0x33  SENSORHUB1-6 — Mag data from sensor hub
 */

/* FUNC_CFG_ACCESS (0x01) */
#define LSM6DSM_FUNC_CFG_ACCESS         0x01
#define LSM6DSM_FUNC_CFG_EN             0x20   /* func_cfg_en=1 (embedded page) */

/* Sensor Hub slave registers (embedded page, 0x02-0x0D) */
#define LSM6DSM_SLV0_ADD                0x02
#define LSM6DSM_SLV0_SUBADD             0x03
#define LSM6DSM_SLAVE0_CONFIG           0x04
#define LSM6DSM_SLV1_ADD                0x05
#define LSM6DSM_SLV1_SUBADD             0x06
#define LSM6DSM_SLAVE1_CONFIG           0x07
#define LSM6DSM_SLV2_ADD                0x08
#define LSM6DSM_SLV2_SUBADD             0x09
#define LSM6DSM_SLAVE2_CONFIG           0x0A
#define LSM6DSM_SLV3_ADD                0x0B
#define LSM6DSM_SLV3_SUBADD             0x0C
#define LSM6DSM_SLAVE3_CONFIG           0x0D

/* SLAVE0_CONFIG bits */
#define LSM6DSM_SLAVE0_NUMOP(n)         ((n) & 0x07)          /* bits [2:0] */
#define LSM6DSM_SLAVE0_RATE(r)          (((r) & 0x03) << 6)   /* bits [7:6] */
#define   LSM6DSM_SLV_RATE_DISABLE      0x00
#define   LSM6DSM_SLV_RATE_ODR_DIV_2    0x40   /* sensor ODR / 2 */
#define   LSM6DSM_SLV_RATE_ODR_DIV_4    0x80   /* sensor ODR / 4 */
#define   LSM6DSM_SLV_RATE_ODR_DIV_8    0xC0   /* sensor ODR / 8 */

/* MASTER_CONFIG (0x1A) */
#define LSM6DSM_MASTER_CONFIG           0x1A
#define LSM6DSM_MASTER_MASTER_ON        0x01   /* bit 0: enable sensor hub */
#define LSM6DSM_MASTER_IRON_EN          0x02   /* bit 1: hard-iron enable */
#define LSM6DSM_MASTER_PASS_THROUGH     0x04   /* bit 2: pass-through mode */
#define LSM6DSM_MASTER_PULL_UP_EN       0x08   /* bit 3: internal pull-up */
#define LSM6DSM_MASTER_START_CFG        0x10   /* bit 4: start config */
#define LSM6DSM_MASTER_DATA_VAL_SEL     0x40   /* bit 6: data valid sel */
#define LSM6DSM_MASTER_DRDY_ON_INT1     0x80   /* bit 7: drdy on INT1 */

/* Sensor Hub output registers (normal page, 0x2E-0x39) */
#define LSM6DSM_SENSORHUB1_REG          0x2E
#define LSM6DSM_SENSORHUB2_REG          0x2F
#define LSM6DSM_SENSORHUB3_REG          0x30
#define LSM6DSM_SENSORHUB4_REG          0x31
#define LSM6DSM_SENSORHUB5_REG          0x32
#define LSM6DSM_SENSORHUB6_REG          0x33
#define LSM6DSM_SENSORHUB7_REG          0x34
#define LSM6DSM_SENSORHUB8_REG          0x35
#define LSM6DSM_SENSORHUB9_REG          0x36
#define LSM6DSM_SENSORHUB10_REG         0x37
#define LSM6DSM_SENSORHUB11_REG         0x38
#define LSM6DSM_SENSORHUB12_REG         0x39

/* CTRL10_C (0x19) — Sensor hub enable */
#define LSM6DSM_CTRL10_C                0x19
#define LSM6DSM_CTRL10_FUNC_EN          0x04   /* bit 2: embedded func enable */

/* Status register for sensor hub */
#define LSM6DSM_FUNC_SRC1               0x53
#define LSM6DSM_FUNC_SRC2               0x54
#define   LSM6DSM_FUNC_SRC2_SLV0_NACK   0x08   /* bit 3: slave 0 NACK */

/* ==================== 9-Axis Data Structure ==================== */

typedef struct {
    float accel_x;       /* g */
    float accel_y;       /* g */
    float accel_z;       /* g */
    float gyro_x;        /* degrees per second */
    float gyro_y;        /* degrees per second */
    float gyro_z;        /* degrees per second */
    float mag_x;         /* mGauss */
    float mag_y;         /* mGauss */
    float mag_z;         /* mGauss */
} lsm6dsm_9axis_data_t;

/* ==================== API ==================== */

/**
 * @brief Initialize LSM6DSM + configure Sensor Hub (Mode 2) for LIS2MDL.
 *
 * Steps:
 *   1. Initialize LSM6DSM accel + gyro (calls lsm6dsm_init internally)
 *   2. Enable pass-through, configure LIS2MDL (continuous mode, BDU, ODR)
 *   3. Disable pass-through
 *   4. Configure SLV0 to read LIS2MDL data (6 bytes from 0x68)
 *   5. Enable sensor hub master (MASTER_ON)
 *   6. Set LSM6DSM ODR to start sensor hub sampling
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lsm6dsm_9axis_init(void);

/**
 * @brief Read all 9 axes in one shot.
 *
 * Reads accel + gyro from LSM6DSM output registers, and magnetometer
 * from the sensor hub output registers (SENSORHUB1-6).
 *
 * @param data  Pointer to 9-axis data structure (output)
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lsm6dsm_9axis_read(lsm6dsm_9axis_data_t *data);

/**
 * @brief Read temperature from LSM6DSM.
 * @param temp_c  Output: temperature in degrees Celsius
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lsm6dsm_read_temperature(float *temp_c);

#ifdef __cplusplus
}
#endif

#endif /* _LSM6DSM_9AXIS_H_ */