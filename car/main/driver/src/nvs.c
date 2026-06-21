#include "driver/inc/nvs.h"

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "nvs.h"  // ESP-IDF nvs header
#include "nvs_flash.h"

static const char *TAG = "driver_nvs";
static const char *NVS_NAMESPACE = "storage";

esp_err_t nvs_init_custom(void) {
  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
      err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_LOGW(TAG, "Erasing NVS flash due to corruption or version mismatch");
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  return err;
}

esp_err_t nvs_write_i32(const char *key, int32_t value) {
  nvs_handle_t my_handle;
  esp_err_t err;

  err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return err;
  }

  err = nvs_set_i32(my_handle, key, value);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to write %s!", key);
    nvs_close(my_handle);
    return err;
  }

  err = nvs_commit(my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to commit NVS changes!");
  }

  nvs_close(my_handle);
  return err;
}

esp_err_t nvs_read_i32(const char *key, int32_t *value) {
  nvs_handle_t my_handle;
  esp_err_t err;

  err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
  if (err != ESP_OK) {
    // Don't log error here as finding no key might be expected
    return err;
  }

  err = nvs_get_i32(my_handle, key, value);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGE(TAG, "Error (%s) reading!", esp_err_to_name(err));
  }

  nvs_close(my_handle);
  return err;
}

// esp_err_t nvs_write_int(const char *key, int value){
// }

// esp_err_t nvs_read_int(const char *key, int *value){
// }

esp_err_t nvs_write_float(const char *key, float value) {
  nvs_handle_t my_handle;
  esp_err_t err;

  err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return err;
  }

  err = nvs_set_blob(my_handle, key, &value, sizeof(float));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to write float %s!", key);
    nvs_close(my_handle);
    return err;
  }

  err = nvs_commit(my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to commit NVS changes!");
  }

  nvs_close(my_handle);
  return err;
}

esp_err_t nvs_read_float(const char *key, float *value) {
  nvs_handle_t my_handle;
  esp_err_t err;
  size_t required_size = sizeof(float);

  err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
  if (err != ESP_OK) {
    return err;
  }

  err = nvs_get_blob(my_handle, key, value, &required_size);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGE(TAG, "Error (%s) reading float!", esp_err_to_name(err));
  }

  nvs_close(my_handle);
  return err;
}

esp_err_t nvs_write_str(const char *key, const char *value) {
  nvs_handle_t my_handle;
  esp_err_t err;

  err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
    return err;
  }

  err = nvs_set_str(my_handle, key, value);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to write string %s!", key);
    nvs_close(my_handle);
    return err;
  }

  err = nvs_commit(my_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to commit NVS changes!");
  }

  nvs_close(my_handle);
  return err;
}

esp_err_t nvs_read_str(const char *key, char *out_value, size_t *length) {
  nvs_handle_t my_handle;
  esp_err_t err;

  err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &my_handle);
  if (err != ESP_OK) {
    return err;
  }

  err = nvs_get_str(my_handle, key, out_value, length);
  if (err != ESP_OK && err != ESP_ERR_NVS_NOT_FOUND) {
    ESP_LOGE(TAG, "Error (%s) reading string!", esp_err_to_name(err));
  }

  nvs_close(my_handle);
  return err;
}

void nvs_write_read_example() {
  const char *key = "example_key";
  const char *value_to_write = "Hello, NVS!";
  char value_read[50];
  size_t length = sizeof(value_read);

  // Write string to NVS
  if (nvs_write_str(key, value_to_write) == ESP_OK) {
    ESP_LOGI(TAG, "Successfully wrote string to NVS");
  }

  // Read string from NVS
  if (nvs_read_str(key, value_read, &length) == ESP_OK) {
    ESP_LOGI(TAG, "Read string from NVS: %s", value_read);
  } else {
    ESP_LOGW(TAG, "String not found in NVS");
  }
}