#include "lsm6dsm.h"

#include <stdio.h>
#include <string.h>

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LSM6DSM";

/* I2C handles */
static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t dev_handle = NULL;
static uint8_t s_i2c_addr = 0;

/* Sensitivity divisors (LSB per g / LSB per dps) */
static float s_acc_ssvt = 16393.44f;   /* ±2g: 0.061 mg/LSB */
static float s_gyro_ssvt = 14.286f;    /* ±2000dps: 70 mdps/LSB */

/* ==================== Low-level I2C ==================== */

static esp_err_t lsm6dsm_write_reg(uint8_t reg, uint8_t data)
{
    if (dev_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    uint8_t buf[2] = {reg, data};
    return i2c_master_transmit(dev_handle, buf, sizeof(buf), -1);
}

static esp_err_t lsm6dsm_read_regs(uint8_t reg, uint8_t *data, size_t len)
{
    if (dev_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return i2c_master_transmit_receive(dev_handle, &reg, 1, data, len, -1);
}

static void lsm6dsm_release_i2c(void)
{
    if (dev_handle) {
        i2c_master_bus_rm_device(dev_handle);
        dev_handle = NULL;
    }
    if (bus_handle) {
        i2c_del_master_bus(bus_handle);
        bus_handle = NULL;
    }
}

/* ==================== Sensitivity helpers ==================== */

static uint8_t accel_fs_bits(lsm6dsm_afs_t afs)
{
    switch (afs) {
        case LSM6DSM_AFS_2G:  return 0;
        case LSM6DSM_AFS_16G: return 1;
        case LSM6DSM_AFS_4G:  return 2;
        case LSM6DSM_AFS_8G:  return 3;
        default:              return 0;
    }
}

static float accel_divisor(lsm6dsm_afs_t afs)
{
    switch (afs) {
        case LSM6DSM_AFS_2G:  return 16393.44f;  /* 0.061 mg/LSB  */
        case LSM6DSM_AFS_4G:  return 8196.72f;   /* 0.122 mg/LSB  */
        case LSM6DSM_AFS_8G:  return 4098.36f;   /* 0.244 mg/LSB  */
        case LSM6DSM_AFS_16G: return 2049.18f;   /* 0.488 mg/LSB  */
        default:              return 16393.44f;
    }
}

static uint8_t gyro_fs_bits(lsm6dsm_gfs_t gfs, bool *fs_125)
{
    *fs_125 = false;
    switch (gfs) {
        case LSM6DSM_GFS_125DPS:
            *fs_125 = true;
            return 0;
        case LSM6DSM_GFS_250DPS:  return 0;
        case LSM6DSM_GFS_500DPS:  return 1;
        case LSM6DSM_GFS_1000DPS: return 2;
        case LSM6DSM_GFS_2000DPS: return 3;
        default:                  return 3;
    }
}

static float gyro_divisor(lsm6dsm_gfs_t gfs)
{
    switch (gfs) {
        case LSM6DSM_GFS_125DPS:  return 228.57f;  /* 4.375 mdps/LSB  */
        case LSM6DSM_GFS_250DPS:  return 114.29f;  /* 8.75  mdps/LSB  */
        case LSM6DSM_GFS_500DPS:  return 57.14f;   /* 17.50 mdps/LSB  */
        case LSM6DSM_GFS_1000DPS: return 28.57f;   /* 35    mdps/LSB  */
        case LSM6DSM_GFS_2000DPS: return 14.29f;   /* 70    mdps/LSB  */
        default:                  return 14.29f;
    }
}

/* ==================== Probe ==================== */

static bool lsm6dsm_probe_addr(uint8_t addr)
{
    /* Remove old device handle if any */
    if (dev_handle) {
        i2c_master_bus_rm_device(dev_handle);
        dev_handle = NULL;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = LSM6DSM_CLK_SPEED,
    };

    esp_err_t err = i2c_master_bus_add_device(bus_handle, &dev_cfg, &dev_handle);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "i2c_master_bus_add_device(0x%02X) failed: %d", addr, err);
        return false;
    }

    uint8_t whoami = 0;
    for (int retry = 3; retry > 0; retry--) {
        err = lsm6dsm_read_regs(LSM6DSM_WHO_AM_I, &whoami, 1);
        if (err == ESP_OK && whoami == LSM6DSM_WHO_AM_I_VAL) {
            s_i2c_addr = addr;
            ESP_LOGI(TAG, "Probe 0x%02X -> WHO_AM_I = 0x%02X", addr, whoami);
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    ESP_LOGW(TAG, "Probe 0x%02X failed: err=%d WHO_AM_I=0x%02X", addr, err, whoami);
    return false;
}

/* ==================== Apply range ==================== */

static esp_err_t lsm6dsm_apply_range(lsm6dsm_afs_t afs, lsm6dsm_aodr_t aodr,
                                     lsm6dsm_gfs_t gfs, lsm6dsm_godr_t godr)
{
    /* Accelerometer: CTRL1_XL */
    uint8_t fs_xl = accel_fs_bits(afs);
    esp_err_t ret = lsm6dsm_write_reg(LSM6DSM_CTRL1_XL,
                                       ((uint8_t)aodr << 4) | (fs_xl << 2));
    if (ret != ESP_OK) return ret;

    /* Gyroscope: CTRL2_G */
    bool fs_125 = false;
    uint8_t fs_g = gyro_fs_bits(gfs, &fs_125);
    ret = lsm6dsm_write_reg(LSM6DSM_CTRL2_G,
                             ((uint8_t)godr << 4) | (fs_g << 2) |
                             ((uint8_t)fs_125 << 1));
    if (ret != ESP_OK) return ret;

    s_acc_ssvt = accel_divisor(afs);
    s_gyro_ssvt = gyro_divisor(gfs);

    return ESP_OK;
}

void lsm6dsm_set_range(lsm6dsm_afs_t afs, lsm6dsm_aodr_t aodr,
                        lsm6dsm_gfs_t gfs, lsm6dsm_godr_t godr)
{
    esp_err_t ret = lsm6dsm_apply_range(afs, aodr, gfs, godr);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set range: %d", ret);
    }
}

/* ==================== Init ==================== */

esp_err_t lsm6dsm_init(void)
{
    ESP_LOGI(TAG, "Init LSM6DSM on I2C%d (SDA:%d, SCL:%d)",
             LSM6DSM_I2C_PORT, LSM6DSM_PIN_SDA, LSM6DSM_PIN_SCL);

    /* Create I2C master bus */
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = LSM6DSM_I2C_PORT,
        .scl_io_num = LSM6DSM_PIN_SCL,
        .sda_io_num = LSM6DSM_PIN_SDA,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C bus! err=%d", ret);
        bus_handle = NULL;
        return ret;
    }

    /* Try both possible addresses */
    uint8_t found_addr = 0;
    if (lsm6dsm_probe_addr(LSM6DSM_I2C_ADDR_LOW)) {
        found_addr = LSM6DSM_I2C_ADDR_LOW;
    } else if (lsm6dsm_probe_addr(LSM6DSM_I2C_ADDR_HIGH)) {
        found_addr = LSM6DSM_I2C_ADDR_HIGH;
    }

    if (found_addr == 0) {
        ESP_LOGE(TAG, "LSM6DSM NOT DETECTED at 0x%02X or 0x%02X!",
                 LSM6DSM_I2C_ADDR_LOW, LSM6DSM_I2C_ADDR_HIGH);
        ESP_LOGE(TAG, "Check hardware:");
        ESP_LOGE(TAG, "  1. VDD->3.3V, VDDIO->3.3V, GND->GND");
        ESP_LOGE(TAG, "  2. SDA/SCL wiring and pull-up resistors");
        ESP_LOGE(TAG, "  3. CS pin -> 3.3V (I2C mode)");
        ESP_LOGE(TAG, "  4. SA0 pin -> GND (0x%02X) or 3.3V (0x%02X)",
                 LSM6DSM_I2C_ADDR_LOW, LSM6DSM_I2C_ADDR_HIGH);
        lsm6dsm_release_i2c();
        return ESP_ERR_NOT_FOUND;
    }

    ESP_LOGI(TAG, "LSM6DSM detected at 0x%02X", found_addr);
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Software reset */
    ret = lsm6dsm_write_reg(LSM6DSM_CTRL3_C, LSM6DSM_CTRL3_C_SW_RESET);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Software reset failed: %d", ret);
        lsm6dsm_release_i2c();
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(50));

    /* Verify WHO_AM_I after reset */
    uint8_t whoami = 0;
    ret = lsm6dsm_read_regs(LSM6DSM_WHO_AM_I, &whoami, 1);
    if (ret != ESP_OK || whoami != LSM6DSM_WHO_AM_I_VAL) {
        ESP_LOGE(TAG, "WHO_AM_I after reset failed: err=%d val=0x%02X",
                 ret, whoami);
        lsm6dsm_release_i2c();
        return (ret != ESP_OK) ? ret : ESP_ERR_INVALID_RESPONSE;
    }

    /* BDU + IF_INC */
    ret = lsm6dsm_write_reg(LSM6DSM_CTRL3_C,
                             LSM6DSM_CTRL3_C_BDU | LSM6DSM_CTRL3_C_IF_INC);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure CTRL3_C: %d", ret);
        lsm6dsm_release_i2c();
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Keep I2C enabled (CTRL4_C bit 2 must be 0) */
    ret = lsm6dsm_write_reg(LSM6DSM_CTRL4_C, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write CTRL4_C: %d", ret);
        lsm6dsm_release_i2c();
        return ret;
    }

    /* Default range: ±2g, 416Hz accel; ±2000dps, 416Hz gyro */
    ret = lsm6dsm_apply_range(LSM6DSM_AFS_2G, LSM6DSM_AODR_416HZ,
                               LSM6DSM_GFS_2000DPS, LSM6DSM_GODR_416HZ);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set default range: %d", ret);
        lsm6dsm_release_i2c();
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(50));

    ESP_LOGI(TAG, "LSM6DSM ready on I2C addr 0x%02X", s_i2c_addr);
    return ESP_OK;
}

/* ==================== Read data ==================== */

esp_err_t lsm6dsm_read_data(float *accel_data, float *gyro_data)
{
    if (dev_handle == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    /* Burst read 12 bytes: gyro X low (0x22) through accel Z high (0x2D) */
    uint8_t raw[12];
    esp_err_t ret = lsm6dsm_read_regs(LSM6DSM_OUTX_L_G, raw, 12);
    if (ret != ESP_OK) {
        return ret;
    }

    /* Gyroscope (bytes 0-5) */
    int16_t gx = (int16_t)(((uint16_t)raw[1] << 8) | raw[0]);
    int16_t gy = (int16_t)(((uint16_t)raw[3] << 8) | raw[2]);
    int16_t gz = (int16_t)(((uint16_t)raw[5] << 8) | raw[4]);

    if (gyro_data) {
        gyro_data[0] = (float)gx / s_gyro_ssvt;
        gyro_data[1] = (float)gy / s_gyro_ssvt;
        gyro_data[2] = (float)gz / s_gyro_ssvt;
    }

    /* Accelerometer (bytes 6-11) */
    int16_t ax = (int16_t)(((uint16_t)raw[7] << 8) | raw[6]);
    int16_t ay = (int16_t)(((uint16_t)raw[9] << 8) | raw[8]);
    int16_t az = (int16_t)(((uint16_t)raw[11] << 8) | raw[10]);

    if (accel_data) {
        accel_data[0] = (float)ax / s_acc_ssvt;
        accel_data[1] = (float)ay / s_acc_ssvt;
        accel_data[2] = (float)az / s_acc_ssvt;
    }

    return ESP_OK;
}