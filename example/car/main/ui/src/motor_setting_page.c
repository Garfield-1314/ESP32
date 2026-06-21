#include "ui/inc/motor_setting_page.h"

#include <stdio.h>
#include <stdlib.h>

#include "device/inc/motor.h"
#include "ui/inc/keyboard.h"
#include "ui/inc/ui.h"

lv_obj_t *motor_setting_page = NULL;

static lv_obj_t *ta_wheel_rad;
static lv_obj_t *ta_reduction;
static lv_obj_t *ta_wheel_base;
static lv_obj_t *ta_wheel_track;
static lv_obj_t *ta_enc_lines;
static lv_obj_t *ta_freq;
static lv_obj_t *ta_max_speed;

static lv_obj_t *sw_dir[4];
static lv_obj_t *sw_enc_dir[4];

static void motor_setting_page_delete_cb(lv_event_t *e) {
  motor_setting_page = NULL;
  ta_wheel_rad = NULL;
  ta_reduction = NULL;
  ta_wheel_base = NULL;
  ta_wheel_track = NULL;
  ta_enc_lines = NULL;
  ta_freq = NULL;
  ta_max_speed = NULL;
  for (int i = 0; i < 4; i++) {
    sw_dir[i] = NULL;
    sw_enc_dir[i] = NULL;
  }
}

static void save_btn_cb(lv_event_t *e) {
  if (ta_wheel_rad)
    motor_settings.wheel_Radius_mm = atof(lv_textarea_get_text(ta_wheel_rad));
  if (ta_reduction)
    motor_settings.motor_reduction_ratio =
        atof(lv_textarea_get_text(ta_reduction));
  if (ta_wheel_base)
    motor_settings.wheel_base_mm = atof(lv_textarea_get_text(ta_wheel_base));
  if (ta_wheel_track)
    motor_settings.wheel_track_mm = atof(lv_textarea_get_text(ta_wheel_track));
  if (ta_enc_lines)
    motor_settings.encoder_lines_per_revolution =
        atoi(lv_textarea_get_text(ta_enc_lines));
  if (ta_freq)
    motor_settings.frequency_hz = atof(lv_textarea_get_text(ta_freq));
  if (ta_max_speed)
    motor_settings.max_speed_limit = atof(lv_textarea_get_text(ta_max_speed));

  for (int i = 0; i < 4; i++) {
    if (sw_dir[i]) {
      int val = lv_obj_has_state(sw_dir[i], LV_STATE_CHECKED) ? 1 : 0;
      switch (i) {
        case 0:
          motor_settings.dir0 = val;
          break;
        case 1:
          motor_settings.dir1 = val;
          break;
        case 2:
          motor_settings.dir2 = val;
          break;
        case 3:
          motor_settings.dir3 = val;
          break;
      }
    }
    if (sw_enc_dir[i]) {
      int val = lv_obj_has_state(sw_enc_dir[i], LV_STATE_CHECKED) ? 1 : 0;
      switch (i) {
        case 0:
          motor_settings.encoder_dir0 = val;
          break;
        case 1:
          motor_settings.encoder_dir1 = val;
          break;
        case 2:
          motor_settings.encoder_dir2 = val;
          break;
        case 3:
          motor_settings.encoder_dir3 = val;
          break;
      }
    }
  }

  motor_setting_NVS_save();  // This will trigger calculation of dependent
                             // params and save
}

static lv_obj_t *create_setting_item(lv_obj_t *parent, const char *label_text,
                                     const char *value_text, int y_offset,
                                     const char *accepted_chars) {
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, label_text);
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, y_offset + 10);

  lv_obj_t *ta = lv_textarea_create(parent);
  lv_textarea_set_one_line(ta, true);
  lv_textarea_set_text(ta, value_text);
  lv_obj_set_width(ta, 100);
  lv_obj_align(ta, LV_ALIGN_TOP_RIGHT, -10, y_offset);
  if (accepted_chars) lv_textarea_set_accepted_chars(ta, accepted_chars);
  lv_obj_add_event_cb(ta, textarea_click_cb, LV_EVENT_CLICKED, NULL);
  return ta;
}

static lv_obj_t *create_switch_item(lv_obj_t *parent, const char *label_text,
                                    bool checked, int y_offset) {
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, label_text);
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, y_offset + 10);

  lv_obj_t *sw = lv_switch_create(parent);
  if (checked) lv_obj_add_state(sw, LV_STATE_CHECKED);
  lv_obj_align(sw, LV_ALIGN_TOP_RIGHT, -20, y_offset + 5);
  return sw;
}

void create_motor_setting_page(void) {
  motor_setting_page = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(motor_setting_page, lv_color_hex(0x003a57),
                            LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(motor_setting_page);

  lv_label_set_text(title, "Motor Settings");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  // Create a container for scrolling
  lv_obj_t *cont = lv_obj_create(motor_setting_page);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(80));
  lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_opa(cont, 0, 0);
  lv_obj_set_style_border_width(cont, 0, 0);

  // 返回按钮
  lv_obj_t *back_btn = lv_btn_create(motor_setting_page);
  lv_obj_set_size(back_btn, 70, 30);
  lv_obj_set_pos(back_btn, 0, 0);
  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
  lv_obj_center(back_label);
  lv_obj_add_event_cb(back_btn, switch_page_cb, LV_EVENT_CLICKED, "app_page");

  // Save Button
  lv_obj_t *save_btn = lv_btn_create(motor_setting_page);
  lv_obj_set_size(save_btn, 70, 30);
  lv_obj_align(save_btn, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_t *save_label = lv_label_create(save_btn);
  lv_label_set_text(save_label, "Save");
  lv_obj_center(save_label);
  lv_obj_add_event_cb(save_btn, save_btn_cb, LV_EVENT_CLICKED, NULL);

  char buf[32];
  int y = 0;
  int step = 60;

  snprintf(buf, sizeof(buf), "%.2f", motor_settings.frequency_hz);
  ta_freq =
      create_setting_item(cont, "Freq (Restart)(Hz)", buf, y, "0123456789.");
  y += step;

  snprintf(buf, sizeof(buf), "%.2f", motor_settings.max_speed_limit);
  ta_max_speed =
      create_setting_item(cont, "Max Speed(mm/s)", buf, y, "0123456789.");
  y += step;

  // snprintf(buf, sizeof(buf), "%.2f", motor_settings.wheel_diameter_mm);
  snprintf(buf, sizeof(buf), "%.2f", motor_settings.wheel_Radius_mm);
  ta_wheel_rad =
      create_setting_item(cont, "Wheel Rad(mm)", buf, y, "0123456789.");
  y += step;

  snprintf(buf, sizeof(buf), "%.2f", motor_settings.motor_reduction_ratio);
  ta_reduction =
      create_setting_item(cont, "Reduct Ratio", buf, y, "0123456789.");
  y += step;

  snprintf(buf, sizeof(buf), "%ld",
           motor_settings.encoder_lines_per_revolution);
  ta_enc_lines =
      create_setting_item(cont, "Enc Lines/Rev", buf, y, "0123456789");
  y += step;

  snprintf(buf, sizeof(buf), "%.2f", motor_settings.wheel_base_mm);
  ta_wheel_base =
      create_setting_item(cont, "Wheel Base(mm)", buf, y, "0123456789.");
  y += step;

  snprintf(buf, sizeof(buf), "%.2f", motor_settings.wheel_track_mm);
  ta_wheel_track =
      create_setting_item(cont, "Wheel Track(mm)", buf, y, "0123456789.");
  y += step;

  char label_buf[32];
  for (int i = 0; i < 4; i++) {
    snprintf(label_buf, sizeof(label_buf), "Dir %d", i);
    bool checked = false;
    switch (i) {
      case 0:
        checked = motor_settings.dir0;
        break;
      case 1:
        checked = motor_settings.dir1;
        break;
      case 2:
        checked = motor_settings.dir2;
        break;
      case 3:
        checked = motor_settings.dir3;
        break;
    }
    sw_dir[i] = create_switch_item(cont, label_buf, checked, y);
    y += step;
  }

  for (int i = 0; i < 4; i++) {
    snprintf(label_buf, sizeof(label_buf), "Enc Dir %d", i);
    bool checked = false;
    switch (i) {
      case 0:
        checked = motor_settings.encoder_dir0;
        break;
      case 1:
        checked = motor_settings.encoder_dir1;
        break;
      case 2:
        checked = motor_settings.encoder_dir2;
        break;
      case 3:
        checked = motor_settings.encoder_dir3;
        break;
    }
    sw_enc_dir[i] = create_switch_item(cont, label_buf, checked, y);
    y += step;
  }

  lv_obj_add_event_cb(motor_setting_page, motor_setting_page_delete_cb,
                      LV_EVENT_DELETE, NULL);
}
