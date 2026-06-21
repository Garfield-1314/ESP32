#ifndef __NVS_H__
#define __NVS_H__

#include "esp_err.h"

/**
 * @brief Initialize the default NVS partition.
 *        This function should be called once at startup.
 *
 * @return esp_err_t ESP_OK on success, or error code.
 */
esp_err_t nvs_init_custom(void);

/**
 * @brief Write an int32_t value to NVS.
 *
 * @param key The key to store the value under.
 * @param value The value to store.
 * @return esp_err_t ESP_OK on success, or error code.
 */
esp_err_t nvs_write_i32(const char *key, int32_t value);

/**
 * @brief Read an int32_t value from NVS.
 *
 * @param key The key to read the value from.
 * @param value Pointer to store the read value.
 * @return esp_err_t ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if key doesn't
 * exist, or other error.
 */
esp_err_t nvs_read_i32(const char *key, int32_t *value);

/**
 * @brief Write a float value to NVS.
 *
 * @param key The key to store the value under.
 * @param value The value to store.
 * @return esp_err_t ESP_OK on success, or error code.
 */
esp_err_t nvs_write_float(const char *key, float value);

/**
 * @brief Read a float value from NVS.
 *
 * @param key The key to read the value from.
 * @param value Pointer to store the read value.
 * @return esp_err_t ESP_OK on success, ESP_ERR_NVS_NOT_FOUND if key doesn't
 * exist, or other error.
 */
esp_err_t nvs_read_float(const char *key, float *value);

/**
 * @brief Write a string to NVS.
 *
 * @param key The key to store the string under.
 * @param value The string to store.
 * @return esp_err_t ESP_OK on success, or error code.
 */
esp_err_t nvs_write_str(const char *key, const char *value);

/**
 * @brief Read a string from NVS.
 *
 * @param key The key to read the string from.
 * @param out_value Buffer to store the read string.
 * @param length Pointer to the size of the buffer. On return, it contains the
 * actual length of the string. If out_value is NULL, length will be updated
 * with the required buffer size.
 * @return esp_err_t ESP_OK on success, or error code.
 */
esp_err_t nvs_read_str(const char *key, char *out_value, size_t *length);

void nvs_write_read_example();

#endif  // __NVS_H__
