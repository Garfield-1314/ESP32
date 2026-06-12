/**
 * @file lis2mdl.h
 * @brief LIS2MDLTR 3-axis magnetometer driver (I2C) for ESP-IDF
 *
 * This header provides register definitions, sensitivity values, and
 * low-level helpers for the ST LIS2MDLTR digital magnetometer.
 *
 * In the 9-axis system, the LIS2MDL is connected to the LSM6DSM's
 * Sensor Hub (Mode 2) auxiliary I2C bus.  The host MCU does NOT talk
 * to the LIS2MDL directly — instead the LSM6DSM reads the magnetometer
 * automatically and stores the data in its SENSORHUB output registers.
 *
 * However, this header is also usable for direct-I2C access when the
 * LIS2MDL is on a separate bus.
 */

#ifndef _LIS2MDL_H_
#define _LIS2MDL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== I2C Address ==================== */
/* SDO/SA0 pin:
 *   GND → 0x1C (7-bit) → 0x38 write / 0x39 read (8-bit)
 *   VDD → 0x1D (7-bit) → 0x3A write / 0x3B read (8-bit)
 * NOTE: Some datasheets show 0x1E as the 7-bit address.
 *       Verify against your hardware wiring.
 */
#define LIS2MDL_I2C_ADDR          0x1E   /* 7-bit, SDO=GND */
#define LIS2MDL_WHO_AM_I_VAL      0x40

/* ==================== Registers ==================== */
#define LIS2MDL_WHO_AM_I          0x4F
#define LIS2MDL_CFG_REG_A         0x60
#define LIS2MDL_CFG_REG_B         0x61
#define LIS2MDL_CFG_REG_C         0x62
#define LIS2MDL_INT_CTRL_REG      0x63
#define LIS2MDL_INT_SOURCE_REG    0x64
#define LIS2MDL_INT_THS_L_REG     0x65
#define LIS2MDL_INT_THS_H_REG     0x66
#define LIS2MDL_STATUS_REG        0x67
#define LIS2MDL_OUTX_L_REG        0x68
#define LIS2MDL_OUTX_H_REG        0x69
#define LIS2MDL_OUTY_L_REG        0x6A
#define LIS2MDL_OUTY_H_REG        0x6B
#define LIS2MDL_OUTZ_L_REG        0x6C
#define LIS2MDL_OUTZ_H_REG        0x6D
#define LIS2MDL_TEMP_OUT_L_REG    0x6E
#define LIS2MDL_TEMP_OUT_H_REG    0x6F
#define LIS2MDL_OFFSET_X_L_REG    0x70
#define LIS2MDL_OFFSET_X_H_REG    0x71
#define LIS2MDL_OFFSET_Y_L_REG    0x72
#define LIS2MDL_OFFSET_Y_H_REG    0x73
#define LIS2MDL_OFFSET_Z_L_REG    0x74
#define LIS2MDL_OFFSET_Z_H_REG    0x75

/* ==================== CFG_REG_A bits ==================== */
#define LIS2MDL_CFG_A_MD_MASK     0xC0   /* bits [7:6] operating mode */
#define   LIS2MDL_MD_CONTINUOUS   0x00
#define   LIS2MDL_MD_SINGLE       0x40
#define   LIS2MDL_MD_IDLE         0x80
#define LIS2MDL_CFG_A_ODR_MASK    0x30   /* bits [5:4] output data rate */
#define   LIS2MDL_ODR_10HZ        0x00
#define   LIS2MDL_ODR_20HZ        0x10
#define   LIS2MDL_ODR_50HZ        0x20
#define   LIS2MDL_ODR_100HZ       0x30
#define LIS2MDL_CFG_A_LPF         0x08   /* bit 3: low-pass filter */
#define LIS2MDL_CFG_A_SOFT_RST    0x04   /* bit 2: software reset */
#define LIS2MDL_CFG_A_REBOOT      0x02   /* bit 1: memory reboot */
#define LIS2MDL_CFG_A_INT_ON_DO   0x01   /* bit 0: int on data ready */

/* ==================== CFG_REG_B bits ==================== */
#define LIS2MDL_CFG_B_OFF_CANC    0xE0   /* bits [7:5] offset cancellation */
#define LIS2MDL_CFG_B_SETP        0x10   /* bit 4: periodic set/reset */
#define LIS2MDL_CFG_B_INT_ON_PIN  0x04   /* bit 2: INT pin mode */
#define LIS2MDL_CFG_B_I2C_DIS     0x02   /* bit 1: disable I2C */
#define LIS2MDL_CFG_B_OFF_CNT     0x01   /* bit 0: offset counter */

/* ==================== CFG_REG_C bits ==================== */
#define LIS2MDL_CFG_C_DRDY_ON_PIN 0xF0   /* bits [7:4] DRDY on pin mask */
#define LIS2MDL_CFG_C_INT_ON_INT  0x08   /* bit 3: int on INT pin */
#define LIS2MDL_CFG_C_INT_ON_DY   0x04   /* bit 2: DRDY on INT pin */
#define LIS2MDL_CFG_C_BDU         0x02   /* bit 1: block data update */
#define LIS2MDL_CFG_C_BLE         0x01   /* bit 0: big/little endian */

/* ==================== Sensitivity ==================== */
/* Full scale: ±50 Gauss (±5000 µT)
 * Sensitivity: 1.5 mGauss/LSB (1500 nT/LSB, 0.15 µT/LSB)
 *   magnetic_field (mG) = raw_value * 1.5
 */
#define LIS2MDL_SENSITIVITY       1.5f    /* mGauss per LSB */

/* ==================== Data Conversion ==================== */

/**
 * @brief Convert raw LSB reading to milligauss.
 * @param raw 16-bit signed raw value from sensor
 * @return magnetic field in mGauss
 */
static inline float lis2mdl_raw_to_mgauss(int16_t raw)
{
    return (float)raw * LIS2MDL_SENSITIVITY;
}

#ifdef __cplusplus
}
#endif

#endif /* _LIS2MDL_H_ */