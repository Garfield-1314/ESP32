#include "ui/inc/car_control_page.h"

#include <stdio.h>

#include "Algorithm/inc/Mecanum.h"
#include "device/inc/motor.h"
#include "ui/inc/ui.h"

lv_obj_t *car_control_page;
static lv_timer_t *update_timer;
static lv_obj_t *target_pos_label;
static lv_obj_t *current_pos_label;

// Define movement constants
#define MOVEMENT_TARGET_DELTA 100.0f  // mm
#define ANGLE_TARGET_DELTA 0.1f       // rad

static void update_info_cb(lv_timer_t *timer) {
  if (!car_control_page) return;

  char buf[64];

  // Update target pose (Left)
  snprintf(buf, sizeof(buf), "Target:\nX: %.1f\nY: %.1f\nT: %.2f",
           mecanum_ctrl.target_pose.x, -mecanum_ctrl.target_pose.y,
           mecanum_ctrl.target_pose.theta * 180.0f / 3.1415926f);
  lv_label_set_text(target_pos_label, buf);

  // Update current pose (Right)
  snprintf(buf, sizeof(buf), "Current:\nX: %.1f\nY: %.1f\nT: %.2f",
           mecanum_ctrl.current_pose.x, -mecanum_ctrl.current_pose.y,
           mecanum_ctrl.current_pose.theta * 180.0f / 3.1415926f);
  lv_label_set_text(current_pos_label, buf);
}

static void car_control_page_delete_cb(lv_event_t *e) {
  // Stop the car when exiting the page
  mecanum_ctrl.target_vel.vx = 0;
  mecanum_ctrl.target_vel.vy = 0;
  mecanum_ctrl.target_vel.omega = 0;
  mecanum_ctrl.target_pose = mecanum_ctrl.current_pose;

  if (update_timer) {
    lv_timer_del(update_timer);
    update_timer = NULL;
  }
  car_control_page = NULL;
}

float cur_x = 0;
float cur_y = 0;
float cur_theta = 0;
static void btn_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);
  intptr_t btn_id = (intptr_t)lv_event_get_user_data(e);

  if (code == LV_EVENT_PRESSED) {
    cur_x = mecanum_ctrl.target_pose.x;
    cur_y = mecanum_ctrl.target_pose.y;
    cur_theta = mecanum_ctrl.target_pose.theta;

    switch (btn_id) {
      case 0:
        mecanum_ctrl.target_pose.x = cur_x + MOVEMENT_TARGET_DELTA;
        break;  // Forward
      case 1:
        mecanum_ctrl.target_pose.x = cur_x - MOVEMENT_TARGET_DELTA;
        break;  // Backward
      case 2:
        mecanum_ctrl.target_pose.y = cur_y - MOVEMENT_TARGET_DELTA;
        break;  // Left
      case 3:
        mecanum_ctrl.target_pose.y = cur_y + MOVEMENT_TARGET_DELTA;
        break;  // Right
      case 4:
        mecanum_ctrl.target_pose.theta = cur_theta + ANGLE_TARGET_DELTA;
        break;  // Rotate Left
      case 5:
        mecanum_ctrl.target_pose.theta = cur_theta - ANGLE_TARGET_DELTA;
        break;  // Rotate Right
    }
  }
}

static lv_obj_t *create_control_btn(lv_obj_t *parent, const char *symbol, int w,
                                    int h, intptr_t id) {
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_size(btn, w, h);
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, symbol);
  lv_obj_center(label);
  lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, (void *)id);
  return btn;
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

void car_control_page_create(void) {
  car_control_page = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(car_control_page, lv_color_hex(0x001f3d),
                            LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(car_control_page);
  lv_label_set_text(title, "Car Control");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  // Back button
  lv_obj_t *back_btn = lv_btn_create(car_control_page);
  lv_obj_set_size(back_btn, 70, 30);
  lv_obj_set_pos(back_btn, 0, 0);
  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
  lv_obj_center(back_label);
  lv_obj_add_event_cb(back_btn, switch_page_cb, LV_EVENT_CLICKED, "app_page");

  lv_obj_add_event_cb(car_control_page, car_control_page_delete_cb,
                      LV_EVENT_DELETE, NULL);

  // Info Labels
  target_pos_label = lv_label_create(car_control_page);
  lv_obj_set_style_text_font(target_pos_label, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(target_pos_label, lv_color_white(), 0);
  lv_obj_align(target_pos_label, LV_ALIGN_TOP_LEFT, 5, 40);

  current_pos_label = lv_label_create(car_control_page);
  lv_obj_set_style_text_font(current_pos_label, &lv_font_montserrat_10, 0);
  lv_obj_set_style_text_color(current_pos_label, lv_color_white(), 0);
  lv_obj_set_style_text_align(current_pos_label, LV_TEXT_ALIGN_RIGHT, 0);
  lv_obj_align(current_pos_label, LV_ALIGN_TOP_RIGHT, -5, 40);

  // Create timer for updating info
  update_info_cb(NULL);  // Update once immediately
  update_timer = lv_timer_create(update_info_cb, 100, NULL);

  int btn_size = 50;

  // Up (0)
  lv_obj_t *btn_up =
      create_control_btn(car_control_page, LV_SYMBOL_UP, btn_size, btn_size, 0);
  lv_obj_align(btn_up, LV_ALIGN_CENTER, 0, -btn_size + 10);

  // Down (1)
  lv_obj_t *btn_down = create_control_btn(car_control_page, LV_SYMBOL_DOWN,
                                          btn_size, btn_size, 1);
  lv_obj_align(btn_down, LV_ALIGN_CENTER, 0, btn_size + 20);

  // Left (2)
  lv_obj_t *btn_left = create_control_btn(car_control_page, LV_SYMBOL_LEFT,
                                          btn_size, btn_size, 2);
  lv_obj_align(btn_left, LV_ALIGN_CENTER, -btn_size - 5, 15);

  // Right (3)
  lv_obj_t *btn_right = create_control_btn(car_control_page, LV_SYMBOL_RIGHT,
                                           btn_size, btn_size, 3);
  lv_obj_align(btn_right, LV_ALIGN_CENTER, btn_size + 5, 15);

  // Rotate Left (4)
  lv_obj_t *btn_rot_l = create_control_btn(car_control_page, LV_SYMBOL_REFRESH,
                                           btn_size, btn_size, 4);
  lv_obj_align(btn_rot_l, LV_ALIGN_BOTTOM_LEFT, 20, -20);

  // Rotate Right (5)
  lv_obj_t *btn_rot_r = create_control_btn(car_control_page, LV_SYMBOL_REFRESH,
                                           btn_size, btn_size, 5);
  lv_obj_align(btn_rot_r, LV_ALIGN_BOTTOM_RIGHT, -20, -20);

  mode_switch_cb();
}

void car_control_event_handler(lv_event_t *e) {
  // Function implementation goes here
}