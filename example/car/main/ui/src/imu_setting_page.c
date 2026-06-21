#include "ui/inc/imu_setting_page.h"

#include "device/inc/imu_hal.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ui/inc/keyboard.h"
#include "ui/inc/ui.h"

lv_obj_t *imu_setting_page;
static lv_obj_t *label_ypr;
static lv_obj_t *label_offset;
static lv_obj_t *calib_btn;
static lv_obj_t *bar_calib;
static lv_obj_t *ta_samples;
static lv_obj_t *label_params;
static lv_obj_t *ddlist_imu;
static lv_timer_t *imu_timer;
static bool is_calibrating = false;

static void imu_timer_cb(lv_timer_t *t) {
  if (imu_setting_page == NULL) {
    lv_timer_del(t);
    imu_timer = NULL;
    return;
  }

  // Sync HAL data for display (driver internal data already syncs via
  // calibration / read task)
  char buf[128];
  snprintf(buf, sizeof(buf), "Yaw: %.2f\nPitch: %.2f\nRoll: %.2f",
           imu_data.yaw, imu_data.pitch, imu_data.roll);
  lv_label_set_text(label_ypr, buf);

  snprintf(buf, sizeof(buf), "OffX: %.4f\nOffY: %.4f\nOffZ: %.4f\n%s",
           imu_data.gyro_x_offset, imu_data.gyro_y_offset,
           imu_data.gyro_z_offset, imu_hal_get_type_str());
  lv_label_set_text(label_offset, buf);

  // Display tuned parameters
  lv_label_set_text(label_params, imu_data.calib_msg);

  if (is_calibrating) {
    lv_obj_add_state(calib_btn, LV_STATE_DISABLED);
    lv_label_set_text(lv_obj_get_child(calib_btn, 0), "Calibrating...");
    lv_bar_set_value(bar_calib, imu_data.calibration_progress, LV_ANIM_ON);
    lv_obj_clear_flag(bar_calib, LV_OBJ_FLAG_HIDDEN);
  } else {
    lv_obj_clear_state(calib_btn, LV_STATE_DISABLED);
    lv_label_set_text(lv_obj_get_child(calib_btn, 0), "Start Calibration");
    lv_obj_add_flag(bar_calib, LV_OBJ_FLAG_HIDDEN);
  }
}

static void calibration_task(void *pvParameters) {
  is_calibrating = true;  // 开始校准
  const char *text = lv_textarea_get_text(ta_samples);
  int samples = atoi(text);
  if (samples <= 0) samples = 1000;
  imu_hal_calibration(samples);  // 进行校准
  imu_hal_nvs_save();
  is_calibrating = false;  // 校准完成
  vTaskDelete(NULL);
}

static void calib_btn_cb(lv_event_t *e) {
  if (is_calibrating) return;
  xTaskCreate(calibration_task, "calib_task", 4096, NULL, 5, NULL);
}

static void imu_ddlist_cb(lv_event_t *e) {
  uint16_t sel = lv_dropdown_get_selected(ddlist_imu);
  if (sel != imu_hal_get_type()) {
    imu_hal_set_type((imu_type_t)sel);
    // Show a message that reboot is needed
    lv_label_set_text(label_params, "Reboot required to switch IMU!");
  }
}

static void imu_setting_page_delete_cb(lv_event_t *e) {
  imu_setting_page = NULL;
  hide_keyboard();
  if (imu_timer) {
    lv_timer_del(imu_timer);
    imu_timer = NULL;
  }
}

void create_imu_setting_page(void) {
  imu_setting_page = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(imu_setting_page, lv_color_hex(0x003a57),
                            LV_PART_MAIN);
  lv_obj_t *title = lv_label_create(imu_setting_page);

  lv_label_set_text(title, "IMU Settings");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  // 返回按钮
  lv_obj_t *back_btn = lv_btn_create(imu_setting_page);

  lv_obj_set_size(back_btn, 70, 30);
  lv_obj_set_pos(back_btn, 0, 0);
  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
  lv_obj_center(back_label);
  lv_obj_add_event_cb(back_btn, switch_page_cb, LV_EVENT_CLICKED, "app_page");

  // ── IMU Type Selection Dropdown ──
  lv_obj_t *ddlist_label = lv_label_create(imu_setting_page);
  lv_label_set_text(ddlist_label, "IMU Type:");
  lv_obj_set_style_text_color(ddlist_label, lv_color_white(), 0);
  lv_obj_align(ddlist_label, LV_ALIGN_TOP_LEFT, 20, 35);

  ddlist_imu = lv_dropdown_create(imu_setting_page);
  lv_dropdown_set_options_static(ddlist_imu,
                                 "ICM42688\nMPU6050\nLSM6DSM");
  lv_dropdown_set_selected(ddlist_imu, imu_hal_get_type());
  lv_obj_set_size(ddlist_imu, 120, 30);
  lv_obj_align(ddlist_imu, LV_ALIGN_TOP_RIGHT, -20, 35);
  lv_dropdown_set_dir(ddlist_imu, LV_DIR_BOTTOM);
  lv_obj_add_event_cb(ddlist_imu, imu_ddlist_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // YPR Label
  label_ypr = lv_label_create(imu_setting_page);
  lv_obj_set_style_text_color(label_ypr, lv_color_white(), 0);
  lv_obj_align(label_ypr, LV_ALIGN_TOP_LEFT, 20, 70);
  lv_label_set_text(label_ypr, "Yaw: 0.00\nPitch: 0.00\nRoll: 0.00");

  // Offset Label
  label_offset = lv_label_create(imu_setting_page);
  lv_obj_set_style_text_color(label_offset, lv_color_white(), 0);
  lv_obj_align(label_offset, LV_ALIGN_TOP_LEFT, 160, 70);
  lv_label_set_text(label_offset, "OffX: 0.0000\nOffY: 0.0000\nOffZ: 0.0000");

  // Auto-tuned Params Label (also used for reboot hint)
  label_params = lv_label_create(imu_setting_page);
  lv_obj_set_style_text_color(label_params, lv_color_hex(0xFFCC00),
                              0);  // Gold color for importance
  lv_obj_align(label_params, LV_ALIGN_BOTTOM_MID, 0, -10);
  /* Set default text to indicate Kalman/filter is active */
  lv_label_set_text(label_params, "LPF:--Hz Kalman");

  // Samples Textarea
  lv_obj_t *ta_label = lv_label_create(imu_setting_page);
  lv_label_set_text(ta_label, "Samples:");
  lv_obj_set_style_text_color(ta_label, lv_color_white(), 0);
  lv_obj_align(ta_label, LV_ALIGN_CENTER, -60, 0);

  ta_samples = lv_textarea_create(imu_setting_page);
  lv_textarea_set_one_line(ta_samples, true);
  lv_textarea_set_accepted_chars(ta_samples, "0123456789");
  lv_textarea_set_text(ta_samples, "1000");
  lv_obj_set_size(ta_samples, 100, 40);
  lv_obj_align(ta_samples, LV_ALIGN_CENTER, 40, 0);
  lv_obj_add_event_cb(ta_samples, textarea_click_cb, LV_EVENT_CLICKED, NULL);

  // Progress Bar
  bar_calib = lv_bar_create(imu_setting_page);
  lv_obj_set_size(bar_calib, 200, 15);
  lv_obj_align(bar_calib, LV_ALIGN_BOTTOM_MID, 0, -85);
  lv_bar_set_range(bar_calib, 0, 100);
  lv_bar_set_value(bar_calib, 0, LV_ANIM_OFF);
  lv_obj_add_flag(bar_calib, LV_OBJ_FLAG_HIDDEN);  // 初始隐藏

  // Calibration Button
  calib_btn = lv_btn_create(imu_setting_page);
  lv_obj_set_size(calib_btn, 200, 40);
  lv_obj_align(calib_btn, LV_ALIGN_BOTTOM_MID, 0, -40);
  lv_obj_t *btn_label = lv_label_create(calib_btn);
  lv_label_set_text(btn_label, "Start Calibration");
  lv_obj_center(btn_label);
  lv_obj_add_event_cb(calib_btn, calib_btn_cb, LV_EVENT_CLICKED, NULL);

  imu_timer = lv_timer_create(imu_timer_cb, 100, NULL);

  lv_obj_add_event_cb(imu_setting_page, imu_setting_page_delete_cb,
                      LV_EVENT_DELETE, NULL);
}