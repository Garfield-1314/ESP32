#include "driver/inc/ota_usb.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "esp_log.h"
#include "esp_ota_ops.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ota_usb";

/* Path to firmware file on USB drive */
#define OTA_FILE_PATH "/usb/car.bin"

/* Read buffer size for chunked OTA write (4KB) */
#define OTA_BUF_SIZE 4096

bool ota_usb_check_firmware_exists(void) {
    struct stat st;
    if (stat(OTA_FILE_PATH, &st) != 0) {
        return false;
    }
    return (st.st_size > 0);
}

esp_err_t ota_usb_perform_update(void) {
    FILE *f = fopen(OTA_FILE_PATH, "rb");
    if (f == NULL) {
        ESP_LOGE(TAG, "Cannot open %s", OTA_FILE_PATH);
        return ESP_ERR_NOT_FOUND;
    }

    /* Get file size */
    struct stat st;
    stat(OTA_FILE_PATH, &st);
    size_t file_size = st.st_size;
    ESP_LOGI(TAG, "Found firmware file, size: %u bytes", (unsigned)file_size);

    /* Get the inactive OTA partition (the one NOT currently booted) */
    const esp_partition_t *update_partition =
        esp_ota_get_next_update_partition(NULL);
    if (update_partition == NULL) {
        ESP_LOGE(TAG, "No OTA partition available");
        fclose(f);
        return ESP_ERR_NOT_FOUND;
    }
    ESP_LOGI(TAG, "Writing to partition %s @ 0x%lx, size 0x%lx",
             update_partition->label,
             (unsigned long)update_partition->address,
             (unsigned long)update_partition->size);

    /* Start OTA — writes app image header + marks partition pending */
    esp_ota_handle_t ota_handle;
    esp_err_t ret = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_begin failed: %s", esp_err_to_name(ret));
        fclose(f);
        return ret;
    }

    /* Allocate read buffer */
    uint8_t *buf = malloc(OTA_BUF_SIZE);
    if (buf == NULL) {
        ESP_LOGE(TAG, "Failed to allocate OTA buffer");
        esp_ota_abort(ota_handle);
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    /* Read firmware file and write via esp_ota_write */
    size_t bytes_written = 0;
    while (bytes_written < file_size) {
        size_t to_read = (file_size - bytes_written < OTA_BUF_SIZE)
                             ? (file_size - bytes_written)
                             : OTA_BUF_SIZE;

        size_t nread = fread(buf, 1, to_read, f);
        if (nread != to_read) {
            ESP_LOGE(TAG, "Read error at offset %u", (unsigned)bytes_written);
            free(buf);
            esp_ota_abort(ota_handle);
            fclose(f);
            return ESP_FAIL;
        }

        ret = esp_ota_write(ota_handle, buf, to_read);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "esp_ota_write error: %s", esp_err_to_name(ret));
            free(buf);
            esp_ota_abort(ota_handle);
            fclose(f);
            return ret;
        }

        bytes_written += to_read;

        /* Progress logging every 256KB */
        if ((bytes_written % (256 * 1024)) == 0) {
            ESP_LOGI(TAG, "Progress: %u / %u bytes (%.0f%%)",
                     (unsigned)bytes_written, (unsigned)file_size,
                     100.0 * bytes_written / file_size);
        }
    }

    free(buf);

    /* Finalize OTA — validates image and marks partition as VALID */
    ret = esp_ota_end(ota_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed (image invalid): %s",
                 esp_err_to_name(ret));
        fclose(f);
        return ret;
    }

    fclose(f);

    /* Set the boot partition to the newly written partition */
    ret = esp_ota_set_boot_partition(update_partition);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed: %s",
                 esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Boot partition set to %s. Rebooting...",
             update_partition->label);

    /* Delete firmware file so it doesn't trigger dialog on next boot */
    ota_usb_remove_firmware();

    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();

    return ESP_OK;
}

void ota_usb_remove_firmware(void) {
    if (unlink(OTA_FILE_PATH) == 0) {
        ESP_LOGI(TAG, "Deleted %s", OTA_FILE_PATH);
    } else {
        ESP_LOGW(TAG, "Failed to delete %s", OTA_FILE_PATH);
    }
}