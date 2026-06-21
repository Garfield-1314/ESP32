#include "ui/inc/ota_page.h"
#include "driver/inc/ota_usb.h"

#include "esp_log.h"
#include "lvgl.h"

static const char *TAG = "ota_page";

/* Button texts, terminated by "" */
static const char *btn_txts[] = {"Upgrade", "Cancel", ""};

/* Keep a pointer to the message box so callbacks can close it */
static lv_obj_t *ota_mbox = NULL;

/**
 * @brief Event handler for the OTA message box buttons
 */
static void ota_btn_event_cb(lv_event_t *e) {
    lv_obj_t *obj = lv_event_get_current_target(e);
    uint16_t idx = lv_msgbox_get_active_btn(obj);
    const char *txt = lv_msgbox_get_active_btn_text(obj);

    ESP_LOGI(TAG, "User pressed button %d: %s", idx, txt);

    if (idx == 0) {
        /* "Upgrade" — Perform OTA upgrade */
        ESP_LOGI(TAG, "Starting OTA update from USB...");
        lv_msgbox_close(obj);
        ota_mbox = NULL;

        /* Perform update (calls esp_restart on success) */
        esp_err_t ret = ota_usb_perform_update();
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "OTA update failed: %s", esp_err_to_name(ret));
            /* Show error on screen (reboot might not happen) */
            lv_obj_t *err_label = lv_label_create(lv_scr_act());
            lv_label_set_recolor(err_label, true);
            lv_label_set_text(err_label, "#FF0000 OTA FAILED!#\nPlease restart");
            lv_obj_center(err_label);
        }
    } else {
        /* "Cancel" — Just close the dialog, keep firmware file on USB */
        ESP_LOGI(TAG, "User cancelled OTA upgrade");
        lv_msgbox_close(obj);
        ota_mbox = NULL;
    }
}

void ota_show_dialog(void) {
    /* If a dialog is already open, don't create another */
    if (ota_mbox != NULL) {
        return;
    }

    ota_mbox = lv_msgbox_create(NULL,
                                 "OTA Update",
                                 "car.bin firmware detected on USB,\nupgrade now?",
                                 btn_txts,
                                 false);
    if (ota_mbox == NULL) {
        ESP_LOGE(TAG, "Failed to create msgbox");
        return;
    }

    /* Center the message box on screen */
    lv_obj_center(ota_mbox);

    /* Register event callback for button presses */
    lv_obj_add_event_cb(ota_mbox, ota_btn_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    ESP_LOGI(TAG, "OTA upgrade dialog displayed");
}