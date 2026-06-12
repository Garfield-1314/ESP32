/**
 * @file lsm6dsm_9axis.c
 * @brief Complete 9-axis driver using LSM6DSMTR Mode 2 (Sensor Hub)
 *
 * This file implements:
 *   1. LIS2MDL configuration via pass-through from LSM6DSM
 *   2. LSM6DSM Sensor Hub (Mode 2) setup for reading LIS2MDL
 *   3. 9-axis data read (accel + gyro + mag)
 *
 * Hardware connection (typical):
 *   ESP32 SDA ──────────► LSM6DSM SDA ──► LIS2MDL SDA
 *   ESP32 SCL ──────────► LSM6DSM SCL ──► LIS2MDL SCL
 *
 * The LIS2MDL MUST share the same I2C bus as the LSM6DSM.
 * No dedicated auxiliary I2C pins are needed — the LSM6DSM sensor
 * hub uses the same pins in pass-through or master mode.
 */

#include "lsm6dsm_9axis.h"

#include <stdio.h>
#include <string.h>

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lsm6dsm.h"
#include "lis2mdl.h"

static const char *TAG = "9AXIS";

/* ==================== Embedded Page Access ==================== */

/**
 * @brief Switch to the embedded function page (where sensor hub regs live).
 */
static esp_err_t lsm6dsm_enter_embedded_page(void)
{
    return lsm6dsm_write_reg(LSM6DSM_FUNC_CFG_ACCESS, LSM6DSM_FUNC_CFG_EN);
}

/**
 * @brief Return to the normal register page.
 */
static esp_err_t lsm6dsm_exit_embedded_page(void)
{
    return lsm6dsm_write_reg(LSM6DSM_FUNC_CFG_ACCESS, 0x00);
}

/* ==================== LIS2MDL Configuration via Pass-Through ==================== */

/**
 * @brief Write a register on LIS2MDL via LSM6DSM pass-through.
 *
 * This uses the sensor hub's write capability to send data to the
 * external sensor while pass-through mode is enabled.
 */
static esp_err_t lis2mdl_write_reg_via_pt(uint8_t reg, uint8_t data)
{
    esp_err_t ret;

    /* Enter embedded page */
    ret = lsm6dsm_enter_embedded_page();
    if (ret != ESP_OK) return ret;

    /* Configure SLV0 for write: address = LIS2MDL, RW=0 (write) */
    /* SLV0_ADD = (LIS2MDL_I2C_ADDR << 1) | 0 = 0x3C (write address) */
    ret = lsm6dsm_write_reg(LSM6DSM_SLV0_ADD, (LIS2MDL_I2C_ADDR << 1) | 0);
    if (ret != ESP_OK) goto exit;

    /* Set subaddress = target register on LIS2MDL */
    ret = lsm6dsm_write_reg(LSM6DSM_SLV0_SUBADD, reg);
    if (ret != ESP_OK) goto exit;

    /* Write the data byte to DATAWRITE_SRC_MODE_SUB_SLV0 (0x0E) */
    ret = lsm6dsm_write_reg(0x0E, data);
    if (ret != ESP_OK) goto exit;

    /* Configure SLAVE0_CONFIG: write 1 byte (numop=1), rate=0, src_mode=0 */
    ret = lsm6dsm_write_reg(LSM6DSM_SLAVE0_CONFIG, 0x01);
    if (ret != ESP_OK) goto exit;

exit:
    lsm6dsm_exit_embedded_page();
    return ret;
}

/**
 * @brief Read a register from LIS2MDL via LSM6DSM pass-through.
 */
static esp_err_t lis2mdl_read_reg_via_pt(uint8_t reg, uint8_t *data)
{
    esp_err_t ret;

    /* Enter embedded page */
    ret = lsm6dsm_enter_embedded_page();
    if (ret != ESP_OK) return ret;

    /* Configure SLV0 for read: address = LIS2MDL, RW=1 (read) */
    ret = lsm6dsm_write_reg(LSM6DSM_SLV0_ADD, (LIS2MDL_I2C_ADDR << 1) | 1);
    if (ret != ESP_OK) goto exit;

    /* Set subaddress = register to read */
    ret = lsm6dsm_write_reg(LSM6DSM_SLV0_SUBADD, reg);
    if (ret != ESP_OK) goto exit;

    /* Read 1 byte */
    ret = lsm6dsm_write_reg(LSM6DSM_SLAVE0_CONFIG, 0x01);
    if (ret != ESP_OK) goto exit;

exit:
    lsm6dsm_exit_embedded_page();

    /* After exiting embedded page, the sensor hub stores the result
     * in SENSORHUB1_REG (0x2E).  Read it back. */
    if (ret == ESP_OK) {
        ret = lsm6dsm_read_regs(LSM6DSM_SENSORHUB1_REG, data, 1);
    }

    return ret;
}

/**
 * @brief Configure LIS2MDL to continuous mode via LSM6DSM pass-through.
 *
 * This function is called once during initialization.  It temporarily
 * enables pass-through mode on the LSM6DSM to let the host MCU
 * configure the LIS2MDL registers through the LSM6DSM bridge.
 */
static esp_err_t lis2mdl_init_via_passthrough(void)
{
    esp_err_t ret;
    uint8_t whoami = 0;
    uint8_t cfg_a, cfg_c;

    ESP_LOGI(TAG, "Configuring LIS2MDL via pass-through...");

    /* Step 1: Enable pass-through on LSM6DSM */
    ret = lsm6dsm_write_reg(LSM6DSM_MASTER_CONFIG,
                             LSM6DSM_MASTER_PASS_THROUGH | LSM6DSM_MASTER_PULL_UP_EN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable pass-through: %d", ret);
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Step 2: Verify LIS2MDL presence */
    ret = lis2mdl_read_reg_via_pt(LIS2MDL_WHO_AM_I, &whoami);
    if (ret != ESP_OK || whoami != LIS2MDL_WHO_AM_I_VAL) {
        ESP_LOGE(TAG, "LIS2MDL not detected! WHO_AM_I = 0x%02X (expected 0x%02X)",
                 whoami, LIS2MDL_WHO_AM_I_VAL);
        /* Disable pass-through and return error */
        lsm6dsm_write_reg(LSM6DSM_MASTER_CONFIG, 0x00);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "LIS2MDL detected! WHO_AM_I = 0x%02X", whoami);

    /* Step 3: Software reset LIS2MDL */
    ret = lis2mdl_write_reg_via_pt(LIS2MDL_CFG_REG_A, LIS2MDL_CFG_A_SOFT_RST);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LIS2MDL soft reset failed: %d", ret);
        lsm6dsm_write_reg(LSM6DSM_MASTER_CONFIG, 0x00);
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(20));

    /* Step 4: Configure CFG_REG_A — continuous mode, 100Hz ODR, LPF on */
    cfg_a = LIS2MDL_MD_CONTINUOUS |   /* continuous mode */
            LIS2MDL_ODR_100HZ    |   /* 100 Hz ODR */
            LIS2MDL_CFG_A_LPF;       /* enable low-pass filter */
    ret = lis2mdl_write_reg_via_pt(LIS2MDL_CFG_REG_A, cfg_a);
    if (ret != ESP_OK) goto fail;
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Step 5: Configure CFG_REG_C — enable BDU (block data update) */
    cfg_c = LIS2MDL_CFG_C_BDU;
    ret = lis2mdl_write_reg_via_pt(LIS2MDL_CFG_REG_C, cfg_c);
    if (ret != ESP_OK) goto fail;
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Step 6: Disable pass-through */
    ret = lsm6dsm_write_reg(LSM6DSM_MASTER_CONFIG, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to disable pass-through: %d", ret);
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    ESP_LOGI(TAG, "LIS2MDL configured successfully via pass-through");
    return ESP_OK;

fail:
    lsm6dsm_write_reg(LSM6DSM_MASTER_CONFIG, 0x00);
    ESP_LOGE(TAG, "LIS2MDL configuration failed: %d", ret);
    return ret;
}

/* ==================== Sensor Hub (Mode 2) Setup ==================== */

/**
 * @brief Configure LSM6DSM Sensor Hub (Mode 2) to read LIS2MDL.
 *
 * After this, the LSM6DSM will automatically read 6 bytes from
 * LIS2MDL starting at OUTX_L_REG (0x68) on every sensor ODR cycle.
 * The data appears in SENSORHUB1_REG through SENSORHUB6_REG.
 */
static esp_err_t lsm6dsm_sensor_hub_setup(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Configuring LSM6DSM Sensor Hub (Mode 2)...");

    /* === Step 1: Enter embedded page to configure slaves === */
    ret = lsm6dsm_enter_embedded_page();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enter embedded page: %d", ret);
        return ret;
    }

    /* Step 2: Configure SLV0_ADD — LIS2MDL read address (8-bit) */
    /* 8-bit read address = (7-bit addr << 1) | 1 */
    uint8_t lis2mdl_read_addr = (LIS2MDL_I2C_ADDR << 1) | 1;
    ret = lsm6dsm_write_reg(LSM6DSM_SLV0_ADD, lis2mdl_read_addr);
    if (ret != ESP_OK) goto exit_ep;

    /* Step 3: SLV0_SUBADD — start reading from OUTX_L_REG (0x68) */
    ret = lsm6dsm_write_reg(LSM6DSM_SLV0_SUBADD, LIS2MDL_OUTX_L_REG);
    if (ret != ESP_OK) goto exit_ep;

    /* Step 4: SLAVE0_CONFIG — read 6 bytes (3 axes × 2 bytes each) */
    /*   bits [2:0] = 6 (number of bytes to read)
     *   bit  [3]   = 0 (src_mode: continuous increment)
     *   bits [5:4] = 0 (aux_sens_on: disabled)
     *   bits [7:6] = SLAVE0_RATE: we use ODR/2 here so mag is sampled
     *                at half the LSM6DSM ODR rate.  If LSM6DSM is
     *                at 416Hz, mag is sampled at ~208Hz.
     */
    uint8_t slave0_cfg = LSM6DSM_SLAVE0_NUMOP(6) |   /* 6 bytes */
                         LSM6DSM_SLV_RATE_ODR_DIV_2; /* rate = ODR/2 */
    ret = lsm6dsm_write_reg(LSM6DSM_SLAVE0_CONFIG, slave0_cfg);
    if (ret != ESP_OK) goto exit_ep;

    ret = lsm6dsm_exit_embedded_page();
    if (ret != ESP_OK) return ret;

    /* === Step 5: Enable embedded functions in CTRL10_C === */
    ret = lsm6dsm_write_reg(LSM6DSM_CTRL10_C, LSM6DSM_CTRL10_FUNC_EN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable func in CTRL10_C: %d", ret);
        return ret;
    }

    /* === Step 6: Enable sensor hub master === */
    /* MASTER_CONFIG = MASTER_ON | PULL_UP_EN */
    ret = lsm6dsm_write_reg(LSM6DSM_MASTER_CONFIG,
                             LSM6DSM_MASTER_MASTER_ON |
                             LSM6DSM_MASTER_PULL_UP_EN);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable master: %d", ret);
        return ret;
    }

    vTaskDelay(pdMS_TO_TICKS(10));

    /* Check for NACK from LIS2MDL */
    uint8_t func_src2 = 0;
    lsm6dsm_read_regs(LSM6DSM_FUNC_SRC2, &func_src2, 1);
    if (func_src2 & LSM6DSM_FUNC_SRC2_SLV0_NACK) {
        ESP_LOGW(TAG, "LIS2MDL NACK detected on sensor hub! Check wiring.");
    } else {
        ESP_LOGI(TAG, "Sensor Hub reading LIS2MDL — no NACK");
    }

    ESP_LOGI(TAG, "LSM6DSM Sensor Hub configured successfully");
    return ESP_OK;

exit_ep:
    lsm6dsm_exit_embedded_page();
    ESP_LOGE(TAG, "Sensor hub slave config failed: %d", ret);
    return ret;
}

/* ==================== Public API ==================== */

esp_err_t lsm6dsm_9axis_init(void)
{
    ESP_LOGI(TAG, "=== LSM6DSM + LIS2MDL 9-Axis Initialization ===");

    /* Step 1: Initialize LSM6DSM (accel + gyro) using existing driver */
    esp_err_t ret = lsm6dsm_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LSM6DSM init failed: %d", ret);
        return ret;
    }

    /* Step 2: Configure LIS2MDL via pass-through */
    ret = lis2mdl_init_via_passthrough();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LIS2MDL init failed: %d", ret);
        return ret;
    }

    /* Step 3: Set up Sensor Hub (Mode 2) to auto-read LIS2MDL */
    ret = lsm6dsm_sensor_hub_setup();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Sensor Hub setup failed: %d", ret);
        return ret;
    }

    ESP_LOGI(TAG, "=== 9-Axis system ready! ===");
    return ESP_OK;
}

esp_err_t lsm6dsm_9axis_read(lsm6dsm_9axis_data_t *data)
{
    if (data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    /* Read accel + gyro using existing 6-axis function */
    float accel[3], gyro[3];
    esp_err_t ret = lsm6dsm_read_data(accel, gyro);
    if (ret != ESP_OK) {
        return ret;
    }

    data->accel_x = accel[0];
    data->accel_y = accel[1];
    data->accel_z = accel[2];
    data->gyro_x  = gyro[0];
    data->gyro_y  = gyro[1];
    data->gyro_z  = gyro[2];

    /* Read magnetometer from Sensor Hub output registers.
     * SENSORHUB1_REG = LIS2MDL OUTX_L (low byte of X mag)
     * SENSORHUB2_REG = LIS2MDL OUTX_H (high byte of X mag)
     * SENSORHUB3_REG = LIS2MDL OUTY_L
     * SENSORHUB4_REG = LIS2MDL OUTY_H
     * SENSORHUB5_REG = LIS2MDL OUTZ_L
     * SENSORHUB6_REG = LIS2MDL OUTZ_H
     */
    uint8_t mag_raw[6];
    ret = lsm6dsm_read_regs(LSM6DSM_SENSORHUB1_REG, mag_raw, 6);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read magnetometer data: %d", ret);
        data->mag_x = 0.0f;
        data->mag_y = 0.0f;
        data->mag_z = 0.0f;
        return ret;
    }

    /* Convert from little-endian 16-bit signed to physical value */
    /* Sensitivity: 1.5 mGauss/LSB */
    int16_t mag_x_raw = (int16_t)(((uint16_t)mag_raw[1] << 8) | mag_raw[0]);
    int16_t mag_y_raw = (int16_t)(((uint16_t)mag_raw[3] << 8) | mag_raw[2]);
    int16_t mag_z_raw = (int16_t)(((uint16_t)mag_raw[5] << 8) | mag_raw[4]);

    data->mag_x = (float)mag_x_raw * LIS2MDL_SENSITIVITY;
    data->mag_y = (float)mag_y_raw * LIS2MDL_SENSITIVITY;
    data->mag_z = (float)mag_z_raw * LIS2MDL_SENSITIVITY;

    return ESP_OK;
}

esp_err_t lsm6dsm_read_temperature(float *temp_c)
{
    if (temp_c == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t raw[2];
    esp_err_t ret = lsm6dsm_read_regs(0x20, raw, 2);  /* OUT_TEMP_L, OUT_TEMP_H */
    if (ret != ESP_OK) {
        return ret;
    }

    /* LSM6DSM temperature: 16-bit signed, 256 LSB/°C, offset at 25°C */
    int16_t temp_raw = (int16_t)(((uint16_t)raw[1] << 8) | raw[0]);
    *temp_c = (float)temp_raw / 256.0f + 25.0f;

    return ESP_OK;
}