#include "ui/inc/motor_page.h"

#include <stdio.h>
#include <stdlib.h>

#include "Algorithm/inc/Mecanum.h"
#include "device/inc/motor.h"
#include "ui/inc/keyboard.h"
#include "ui/inc/ui.h"

lv_obj_t *motor_page;
static int current_motor = 0;
static lv_timer_t *timer;

static lv_obj_t *kp_ta;
static lv_obj_t *ki_ta;
static lv_obj_t *kd_ta;
static lv_obj_t *speed_label;
static lv_obj_t *target_speed_ta[4];

void motor_PID_display() {
  char buf[32];
  motor_PID_NVS_load();
  if (kp_ta) {
    snprintf(buf, sizeof(buf), "%.2f", motor_status.inc_kp[current_motor]);
    lv_textarea_set_text(kp_ta, buf);
  }
  if (ki_ta) {
    snprintf(buf, sizeof(buf), "%.2f", motor_status.inc_ki[current_motor]);
    lv_textarea_set_text(ki_ta, buf);
  }
  if (kd_ta) {
    snprintf(buf, sizeof(buf), "%.2f", motor_status.inc_kd[current_motor]);
    lv_textarea_set_text(kd_ta, buf);
  }
}

void motor_speed_display() {
  if (speed_label) {
    char buf[128];
    snprintf(buf, sizeof(buf), "Speed:\n  %.1f\n  %.1f\n  %.1f\n  %.1f\n",
             motor_status.speed_mm_s[0], motor_status.speed_mm_s[1],
             motor_status.speed_mm_s[2], motor_status.speed_mm_s[3]);
    lv_label_set_text(speed_label, buf);
  }
}

void motor_PID_change() { motor_PID_display(); }

static void speed_input_event_handler(lv_event_t *e) {
  lv_obj_t *ta = lv_event_get_target(e);
  int idx = (int)(intptr_t)lv_event_get_user_data(e);
  const char *text = lv_textarea_get_text(ta);
  motor_status.target_speed_mm_s[idx] = atof(text);
}

static void pid_input_event_handler(lv_event_t *e) {
  lv_obj_t *ta = lv_event_get_target(e);
  int type = (int)(intptr_t)lv_event_get_user_data(e);  // 0=Kp, 1=Ki, 2=Kd

  const char *text = lv_textarea_get_text(ta);
  float val = atof(text);

  if (type == 0) {
    motor_status.inc_kp[current_motor] = val;
    motor_status.pid_controller[current_motor].Kp = val;
  } else if (type == 1) {
    motor_status.inc_ki[current_motor] = val;
    motor_status.pid_controller[current_motor].Ki = val;
  } else {
    motor_status.inc_kd[current_motor] = val;
    motor_status.pid_controller[current_motor].Kd = val;
  }
  motor_PID_NVS_save();
}

static void motor_select_event_handler(lv_event_t *e) {
  lv_obj_t *dropdown = lv_event_get_current_target(e);
  current_motor = lv_dropdown_get_selected(dropdown);
  motor_PID_display();
}

static void timer_cb(lv_timer_t *t) { motor_speed_display(); }

void motor_page_delete_cb(lv_event_t *e) {
  if (timer) {
    lv_timer_del(timer);
    timer = NULL;
  }
  motor_page = NULL;
  kp_ta = NULL;
  ki_ta = NULL;
  kd_ta = NULL;
  speed_label = NULL;
  for (int i = 0; i < 4; i++) target_speed_ta[i] = NULL;
  hide_keyboard();
}

static void create_pid_input(lv_obj_t *parent, int y_offset,
                             const char *label_text, lv_obj_t **ta_obj,
                             int type) {
  // Label
  lv_obj_t *label = lv_label_create(parent);
  lv_obj_set_pos(label, 20, y_offset + 10);
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_label_set_text(label, label_text);

  // TextArea
  *ta_obj = lv_textarea_create(parent);
  lv_obj_set_size(*ta_obj, 100, 40);
  lv_obj_set_pos(*ta_obj, 50, y_offset);
  lv_textarea_set_one_line(*ta_obj, true);
  lv_textarea_set_align(*ta_obj, LV_TEXT_ALIGN_CENTER);
  lv_textarea_set_accepted_chars(*ta_obj,
                                 "0123456789.");  // Allow digits and dot

  // Add events
  lv_obj_add_event_cb(*ta_obj, textarea_click_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(*ta_obj, pid_input_event_handler, LV_EVENT_VALUE_CHANGED,
                      (void *)(intptr_t)type);
}

static void mode_switch_cb() {
  motor_control_mode = MOTOR_CONTROL_MODE_DEBUG;
  // Clear motor states when switching mode
  for (int i = 0; i < 4; i++) {
    motor_status.encoder_number[i] = 0;
    motor_status.speed_mm_s[i] = 0;
    motor_status.target_speed_mm_s[i] = 0;
    motor_status.distance_mm[i] = 0;
    motor_status.distance_m[i] = 0;
    motor_status.pid_output[i] = 0;

    // Reset PID internal state
    motor_status.pid_controller[i].prevError = 0;
    motor_status.pid_controller[i].prevPrevError = 0;
    motor_status.pid_controller[i].integral = 0;
    motor_status.pid_controller[i].output = 0;
  }

  mecanum_ctrl.current_pose.x = 0;
  mecanum_ctrl.current_pose.y = 0;
  mecanum_ctrl.current_pose.theta = 0;

  mecanum_ctrl.target_pose.x = 0;
  mecanum_ctrl.target_pose.y = 0;
  mecanum_ctrl.target_pose.theta = 0;

  mecanum_ctrl.current_vel.vx = 0;
  mecanum_ctrl.current_vel.vy = 0;
  mecanum_ctrl.current_vel.omega = 0;

  mecanum_ctrl.target_vel.vx = 0;
  mecanum_ctrl.target_vel.vy = 0;
  mecanum_ctrl.target_vel.omega = 0;
}

void create_motor_page(void) {
  motor_page = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(motor_page, lv_color_hex(0x003a57), LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(motor_page);
  lv_label_set_text(title, "Motor Control");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  lv_obj_t *back_btn = lv_btn_create(motor_page);
  lv_obj_set_size(back_btn, 70, 30);
  lv_obj_set_pos(back_btn, 0, 0);
  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
  lv_obj_center(back_label);
  lv_obj_add_event_cb(back_btn, switch_page_cb, LV_EVENT_CLICKED, "pid_page");

  lv_obj_add_event_cb(motor_page, motor_page_delete_cb, LV_EVENT_DELETE, NULL);

  // Dropdown for motor selection
  lv_obj_t *dd = lv_dropdown_create(motor_page);
  lv_dropdown_set_options(dd, "Motor 1\nMotor 2\nMotor 3\nMotor 4");
  lv_obj_set_width(dd, 100);
  lv_obj_align(dd, LV_ALIGN_TOP_RIGHT, -10, 40);
  lv_obj_add_event_cb(dd, motor_select_event_handler, LV_EVENT_VALUE_CHANGED,
                      NULL);
  lv_dropdown_set_selected(dd, current_motor);

  // Create PID Inputs
  create_pid_input(motor_page, 40, "Kp", &kp_ta, 0);
  create_pid_input(motor_page, 80, "Ki", &ki_ta, 1);
  create_pid_input(motor_page, 120, "Kd", &kd_ta, 2);

  // Speed Label
  speed_label = lv_label_create(motor_page);
  lv_obj_set_style_text_color(speed_label, lv_color_white(), 0);
  lv_obj_align(speed_label, LV_ALIGN_TOP_MID, 100, 80);

  // Speed inputs
  const char *labels[] = {"M1", "M2", "M3", "M4"};
  for (int i = 0; i < 4; i++) {
    lv_obj_t *cont = lv_obj_create(motor_page);
    lv_obj_set_size(cont, 78, 65);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_align(cont, LV_ALIGN_BOTTOM_LEFT, 5 + i * 78, 0);

    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, labels[i]);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, -15);

    target_speed_ta[i] = lv_textarea_create(cont);
    lv_obj_set_size(target_speed_ta[i], 70, 35);
    lv_obj_align(target_speed_ta[i], LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_textarea_set_one_line(target_speed_ta[i], true);
    lv_textarea_set_align(target_speed_ta[i], LV_TEXT_ALIGN_CENTER);
    lv_textarea_set_accepted_chars(target_speed_ta[i], "0123456789.-");

    char buf[16];
    snprintf(buf, sizeof(buf), "%.0f", motor_status.target_speed_mm_s[i]);
    lv_textarea_set_text(target_speed_ta[i], buf);

    lv_obj_add_event_cb(target_speed_ta[i], textarea_click_cb, LV_EVENT_CLICKED,
                        NULL);
    lv_obj_add_event_cb(target_speed_ta[i], speed_input_event_handler,
                        LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)i);
  }

  timer = lv_timer_create(timer_cb, 200, NULL);
  mode_switch_cb();
  motor_PID_display();
  motor_speed_display();
}
