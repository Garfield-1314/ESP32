#include "ui/inc/mecanum_page.h"

#include <stdio.h>
#include <stdlib.h>

#include "Algorithm/inc/Mecanum.h"
#include "device/inc/motor.h"
#include "ui/inc/keyboard.h"
#include "ui/inc/ui.h"

lv_obj_t *mecanum_page;
static int current_axis = 0;  // 0:X, 1:Y, 2:Theta
static lv_timer_t *timer;

static lv_obj_t *kp_ta;
static lv_obj_t *ki_ta;
static lv_obj_t *kd_ta;
static lv_obj_t *info_label;
static lv_obj_t *target_pose_ta[3];

void mecanum_PID_display() {
  char buf[32];
  PID_Controller *pid = NULL;

  // Select PID controller
  if (current_axis == 0)
    pid = &mecanum_ctrl.pid_x;
  else if (current_axis == 1)
    pid = &mecanum_ctrl.pid_y;
  else
    pid = &mecanum_ctrl.pid_theta;

  if (kp_ta && pid) {
    snprintf(buf, sizeof(buf), "%.2f", pid->Kp);
    lv_textarea_set_text(kp_ta, buf);
  }
  if (ki_ta && pid) {
    snprintf(buf, sizeof(buf), "%.2f", pid->Ki);
    lv_textarea_set_text(ki_ta, buf);
  }
  if (kd_ta && pid) {
    snprintf(buf, sizeof(buf), "%.2f", pid->Kd);
    lv_textarea_set_text(kd_ta, buf);
  }
}

void mecanum_info_display() {
  if (info_label) {
    char buf[128];
    snprintf(buf, sizeof(buf),
             "Tar/Cur:\n X:%.1f/%.1f\n Y:%.1f/%.1f\n T:%.2f/%.2f",
             mecanum_ctrl.target_pose.x, mecanum_ctrl.current_pose.x,
             -mecanum_ctrl.target_pose.y, -mecanum_ctrl.current_pose.y,
             mecanum_ctrl.target_pose.theta, mecanum_ctrl.current_pose.theta);
    lv_label_set_text(info_label, buf);
  }
}

static void pose_input_event_handler(lv_event_t *e) {
  lv_obj_t *ta = lv_event_get_target(e);
  int idx = (int)(intptr_t)lv_event_get_user_data(e);
  const char *text = lv_textarea_get_text(ta);
  float val = atof(text);

  if (idx == 0)
    mecanum_ctrl.target_pose.x = val;
  else if (idx == 1)
    mecanum_ctrl.target_pose.y = -val;
  else if (idx == 2)
    mecanum_ctrl.target_pose.theta = val;
}

static void pid_input_event_handler(lv_event_t *e) {
  lv_obj_t *ta = lv_event_get_target(e);
  int type = (int)(intptr_t)lv_event_get_user_data(e);  // 0=Kp, 1=Ki, 2=Kd

  const char *text = lv_textarea_get_text(ta);
  float val = atof(text);

  PID_Controller *pid = NULL;
  if (current_axis == 0)
    pid = &mecanum_ctrl.pid_x;
  else if (current_axis == 1)
    pid = &mecanum_ctrl.pid_y;
  else
    pid = &mecanum_ctrl.pid_theta;

  if (pid) {
    if (type == 0)
      pid->Kp = val;
    else if (type == 1)
      pid->Ki = val;
    else
      pid->Kd = val;

    // Save to NVS
    Mecanum_PID_NVS_save();
  }
}

static void axis_select_event_handler(lv_event_t *e) {
  lv_obj_t *dropdown = lv_event_get_current_target(e);
  current_axis = lv_dropdown_get_selected(dropdown);
  mecanum_PID_display();
}

static void timer_cb(lv_timer_t *t) { mecanum_info_display(); }

void mecanum_page_delete_cb(lv_event_t *e) {
  if (timer) {
    lv_timer_del(timer);
    timer = NULL;
  }
  mecanum_page = NULL;
  kp_ta = NULL;
  ki_ta = NULL;
  kd_ta = NULL;
  info_label = NULL;
  for (int i = 0; i < 3; i++) target_pose_ta[i] = NULL;
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
                                 "0123456789.-");  // Allow digits, dot, minus

  // Add events
  lv_obj_add_event_cb(*ta_obj, textarea_click_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(*ta_obj, pid_input_event_handler, LV_EVENT_VALUE_CHANGED,
                      (void *)(intptr_t)type);
}

static void mode_switch_cb() {
  motor_control_mode = MOTOR_CONTROL_MODE_MECANUM;
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

void create_mecanum_page(void) {
  mecanum_page = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(mecanum_page, lv_color_hex(0x003a57), LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(mecanum_page);
  lv_label_set_text(title, "Mecanum PID");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  lv_obj_t *back_btn = lv_btn_create(mecanum_page);
  lv_obj_set_size(back_btn, 70, 30);
  lv_obj_set_pos(back_btn, 0, 0);
  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
  lv_obj_center(back_label);
  lv_obj_add_event_cb(back_btn, switch_page_cb, LV_EVENT_CLICKED, "pid_page");

  lv_obj_add_event_cb(mecanum_page, mecanum_page_delete_cb, LV_EVENT_DELETE,
                      NULL);

  // Dropdown for axis selection
  lv_obj_t *dd = lv_dropdown_create(mecanum_page);
  lv_dropdown_set_options(dd, "X\nY\nTheta");
  lv_obj_set_width(dd, 100);
  lv_obj_align(dd, LV_ALIGN_TOP_RIGHT, -10, 40);
  lv_obj_add_event_cb(dd, axis_select_event_handler, LV_EVENT_VALUE_CHANGED,
                      NULL);
  lv_dropdown_set_selected(dd, current_axis);

  // Create PID Inputs
  create_pid_input(mecanum_page, 40, "Kp", &kp_ta, 0);
  create_pid_input(mecanum_page, 80, "Ki", &ki_ta, 1);
  create_pid_input(mecanum_page, 120, "Kd", &kd_ta, 2);

  // Info Label
  info_label = lv_label_create(mecanum_page);
  lv_obj_set_style_text_color(info_label, lv_color_white(), 0);
  lv_obj_align(info_label, LV_ALIGN_TOP_MID, 100, 80);

  // Target Pose Inputs
  const char *pose_labels[] = {"X", "Y", "T"};
  for (int i = 0; i < 3; i++) {
    lv_obj_t *cont = lv_obj_create(mecanum_page);
    lv_obj_set_size(cont, 85, 65);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_align(cont, LV_ALIGN_BOTTOM_LEFT, 15 + i * 90, 0);

    lv_obj_t *label = lv_label_create(cont);
    lv_label_set_text(label, pose_labels[i]);
    lv_obj_set_style_text_color(label, lv_color_white(), 0);
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, -15);

    target_pose_ta[i] = lv_textarea_create(cont);
    lv_obj_set_size(target_pose_ta[i], 80, 35);
    lv_obj_align(target_pose_ta[i], LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_textarea_set_one_line(target_pose_ta[i], true);
    lv_textarea_set_align(target_pose_ta[i], LV_TEXT_ALIGN_CENTER);
    lv_textarea_set_accepted_chars(target_pose_ta[i], "0123456789.-");

    char buf[16];
    float val = 0;
    if (i == 0)
      val = mecanum_ctrl.target_pose.x;
    else if (i == 1)
      val = mecanum_ctrl.target_pose.y;
    else
      val = mecanum_ctrl.target_pose.theta;

    snprintf(buf, sizeof(buf), (i == 2) ? "%.2f" : "%.1f", val);
    lv_textarea_set_text(target_pose_ta[i], buf);

    lv_obj_add_event_cb(target_pose_ta[i], textarea_click_cb, LV_EVENT_CLICKED,
                        NULL);
    lv_obj_add_event_cb(target_pose_ta[i], pose_input_event_handler,
                        LV_EVENT_VALUE_CHANGED, (void *)(intptr_t)i);
  }

  timer = lv_timer_create(timer_cb, 200, NULL);

  // Load initial data
  Mecanum_PID_NVS_load();  // Ensure we have latest
  mecanum_PID_display();
  mecanum_info_display();
  mode_switch_cb();
}
