#include "ui/inc/app_page.h"

#include "device/inc/motor.h"
#include "ui/inc/car_control_page.h"
#include "ui/inc/main_page.h"
#include "ui/inc/ui.h"
// 创建APP界面
lv_obj_t *app_page;

static void motor_enable_cb(lv_event_t *e) {
  lv_obj_t *btn = lv_event_get_target(e);
  bool state = lv_obj_has_state(btn, LV_STATE_CHECKED);
  motor_set_enable(state);
  if (state) {
    lv_label_set_text(lv_obj_get_child(btn, 0), "ON");
  } else {
    lv_label_set_text(lv_obj_get_child(btn, 0), "OFF");
  }
}

// 在页面退出时自动清除
static void app_page_delete_cb(lv_event_t *e) { app_page = NULL; }

lv_obj_t *create_app_page(void) {
  app_page = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(app_page, lv_color_hex(0x003a57), LV_PART_MAIN);
  lv_obj_t *title = lv_label_create(app_page);

  lv_label_set_text(title, "Apps");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  // Motor Enable Button
  lv_obj_t *en_btn = lv_btn_create(app_page);
  lv_obj_set_size(en_btn, 60, 30);
  lv_obj_align(en_btn, LV_ALIGN_TOP_RIGHT, -10, 5);
  lv_obj_add_flag(en_btn, LV_OBJ_FLAG_CHECKABLE);

  lv_obj_t *en_label = lv_label_create(en_btn);
  lv_obj_center(en_label);

  if (is_motor_enabled) {
    lv_obj_add_state(en_btn, LV_STATE_CHECKED);
    lv_label_set_text(en_label, "ON");
    // lv_obj_set_style_bg_color(en_btn, lv_palette_main(LV_PALETTE_GREEN), 0);
  } else {
    lv_obj_clear_state(en_btn, LV_STATE_CHECKED);
    lv_label_set_text(en_label, "OFF");
    // lv_obj_set_style_bg_color(en_btn, lv_palette_main(LV_PALETTE_RED), 0);
  }
  lv_obj_add_event_cb(en_btn, motor_enable_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // 返回按钮
  lv_obj_t *back_btn = lv_btn_create(app_page);

  lv_obj_set_size(back_btn, 70, 30);
  lv_obj_set_pos(back_btn, 0, 0);
  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
  lv_obj_center(back_label);
  lv_obj_add_event_cb(back_btn, switch_page_cb, LV_EVENT_CLICKED, "main_page");

  // 设置界面按钮
  lv_obj_t *settings = lv_btn_create(app_page);
  lv_obj_set_size(settings, 60, 60);
  lv_obj_align(settings, LV_ALIGN_TOP_MID, 80, 40);
  lv_obj_t *settings_label = lv_label_create(settings);
  lv_label_set_text(settings_label, LV_SYMBOL_SETTINGS);
  lv_obj_center(settings_label);
  lv_obj_add_event_cb(settings, switch_page_cb, LV_EVENT_CLICKED,
                      "settings_page");

  // PID界面按钮
  lv_obj_t *pid = lv_btn_create(app_page);
  lv_obj_set_size(pid, 60, 60);
  lv_obj_align(pid, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_t *pid_label = lv_label_create(pid);
  lv_label_set_text(pid_label, "PID");
  lv_obj_center(pid_label);
  lv_obj_add_event_cb(pid, switch_page_cb, LV_EVENT_CLICKED, "pid_page");

  // 数字显示界面按钮
  lv_obj_t *num_show = lv_btn_create(app_page);
  lv_obj_set_size(num_show, 60, 60);
  lv_obj_align(num_show, LV_ALIGN_TOP_MID, -80, 40);
  lv_obj_t *num_show_label = lv_label_create(num_show);
  lv_label_set_text(num_show_label, "Num");
  lv_obj_center(num_show_label);
  lv_obj_add_event_cb(num_show, switch_page_cb, LV_EVENT_CLICKED,
                      "num_show_page");

  // 电机设置界面按钮
  lv_obj_t *motor_setting = lv_btn_create(app_page);
  lv_obj_set_size(motor_setting, 60, 60);
  lv_obj_align(motor_setting, LV_ALIGN_TOP_MID, -80, 120);
  lv_obj_t *motor_setting_label = lv_label_create(motor_setting);
  lv_label_set_text(motor_setting_label, "Motor\nSetting");
  lv_obj_center(motor_setting_label);
  lv_obj_add_event_cb(motor_setting, switch_page_cb, LV_EVENT_CLICKED,
                      "motor_setting_page");

  // 电机调试界面按钮
  lv_obj_t *motor_debug = lv_btn_create(app_page);
  lv_obj_set_size(motor_debug, 60, 60);
  lv_obj_align(motor_debug, LV_ALIGN_TOP_MID, 0, 120);
  lv_obj_t *motor_debug_label = lv_label_create(motor_debug);
  lv_label_set_text(motor_debug_label, "Motor\nDebug");
  lv_obj_center(motor_debug_label);
  lv_obj_add_event_cb(motor_debug, switch_page_cb, LV_EVENT_CLICKED,
                      "motor_debug_page");

  // 麦轮控制界面按钮
  lv_obj_t *car_control = lv_btn_create(app_page);
  lv_obj_set_size(car_control, 60, 60);
  lv_obj_align(car_control, LV_ALIGN_TOP_MID, 80, 120);
  lv_obj_t *car_control_label = lv_label_create(car_control);
  lv_label_set_text(car_control_label, "Car\nCtrl");
  lv_obj_center(car_control_label);
  lv_obj_add_event_cb(car_control, switch_page_cb, LV_EVENT_CLICKED,
                      "car_control_page");

  // IMU设置界面按钮
  lv_obj_t *imu_setting = lv_btn_create(app_page);
  lv_obj_set_size(imu_setting, 60, 60);
  lv_obj_align(imu_setting, LV_ALIGN_TOP_MID, -80, 200);
  lv_obj_t *imu_setting_label = lv_label_create(imu_setting);
  lv_label_set_text(imu_setting_label, "IMU\nSetting");
  lv_obj_center(imu_setting_label);
  lv_obj_add_event_cb(imu_setting, switch_page_cb, LV_EVENT_CLICKED,
                      "imu_setting_page");

  // CMD终端界面按钮
  lv_obj_t *cmd_btn = lv_btn_create(app_page);
  lv_obj_set_size(cmd_btn, 60, 60);
  lv_obj_align(cmd_btn, LV_ALIGN_TOP_MID, 0, 200);
  lv_obj_t *cmd_label = lv_label_create(cmd_btn);
  lv_label_set_text(cmd_label, "CMD");
  lv_obj_center(cmd_label);
  lv_obj_add_event_cb(cmd_btn, switch_page_cb, LV_EVENT_CLICKED, "cmd_page");

  lv_obj_add_event_cb(app_page, app_page_delete_cb, LV_EVENT_DELETE, NULL);

  return app_page;
}
