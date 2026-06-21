#include "ui/inc/pid_page.h"

#include "ui/inc/ui.h"

lv_obj_t *pid_page = NULL;

static void pid_page_delete_cb(lv_event_t *e) { pid_page = NULL; }

void create_pid_page(void) {
  pid_page = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(pid_page, lv_color_hex(0x003a57), LV_PART_MAIN);
  lv_obj_t *title = lv_label_create(pid_page);

  lv_label_set_text(title, "PID Settings");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  // 返回按钮
  lv_obj_t *back_btn = lv_btn_create(pid_page);
  lv_obj_set_size(back_btn, 70, 30);
  lv_obj_set_pos(back_btn, 0, 0);
  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
  lv_obj_center(back_label);
  lv_obj_add_event_cb(back_btn, switch_page_cb, LV_EVENT_CLICKED, "app_page");

  lv_obj_add_event_cb(pid_page, pid_page_delete_cb, LV_EVENT_DELETE, NULL);

  // 电机界面按钮
  lv_obj_t *motor = lv_btn_create(pid_page);
  lv_obj_set_size(motor, 60, 60);
  lv_obj_align(motor, LV_ALIGN_TOP_MID, 0, 40);
  lv_obj_t *motor_label = lv_label_create(motor);
  lv_label_set_text(motor_label, "Motor");
  lv_obj_center(motor_label);
  lv_obj_add_event_cb(motor, switch_page_cb, LV_EVENT_CLICKED, "motor_page");

  // 麦轮界面按钮
  lv_obj_t *Mecanum = lv_btn_create(pid_page);
  lv_obj_set_size(Mecanum, 60, 60);
  lv_obj_align(Mecanum, LV_ALIGN_TOP_MID, -80, 40);
  lv_obj_t *Mecanum_label = lv_label_create(Mecanum);
  lv_label_set_text(Mecanum_label, "MecW");
  lv_obj_center(Mecanum_label);
  lv_obj_add_event_cb(Mecanum, switch_page_cb, LV_EVENT_CLICKED,
                      "mecanum_page");
}
