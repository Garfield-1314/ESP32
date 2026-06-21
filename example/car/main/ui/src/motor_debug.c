#include "ui/inc/motor_debug.h"

#include <stdio.h>

#include "Algorithm/inc/Mecanum.h"
#include "device/inc/motor.h"
#include "ui/inc/ui.h"

lv_obj_t *motor_debug_page = NULL;
static lv_timer_t *update_timer = NULL;
static lv_obj_t *lbl_speed[4];
static lv_obj_t *lbl_target[4];
static lv_obj_t *lbl_dist[4];
static lv_obj_t *lbl_pwm[4];

static void update_task(lv_timer_t *t) {
  if (!motor_debug_page) return;

  char buf[64];
  for (int i = 0; i < 4; i++) {
    if (lbl_speed[i]) {
      snprintf(buf, sizeof(buf), "Spd: %.1f", motor_status.speed_mm_s[i]);
      lv_label_set_text(lbl_speed[i], buf);
    }
    if (lbl_target[i]) {
      snprintf(buf, sizeof(buf), "Tgt: %.1f",
               motor_status.target_speed_mm_s[i]);
      lv_label_set_text(lbl_target[i], buf);
    }
    if (lbl_dist[i]) {
      snprintf(buf, sizeof(buf), "Dst: %.2fm", motor_status.distance_m[i]);
      lv_label_set_text(lbl_dist[i], buf);
    }
    if (lbl_pwm[i]) {
      snprintf(buf, sizeof(buf), "PWM: %.0f", motor_status.pid_output[i]);
      lv_label_set_text(lbl_pwm[i], buf);
    }
  }
}

static void motor_debug_page_delete_cb(lv_event_t *e) {
  if (update_timer) {
    lv_timer_del(update_timer);
    update_timer = NULL;
  }
  motor_debug_page = NULL;
  for (int i = 0; i < 4; i++) {
    lbl_speed[i] = NULL;
    lbl_target[i] = NULL;
    lbl_dist[i] = NULL;
    lbl_pwm[i] = NULL;
  }
}

static lv_obj_t *mode_switch_btn = NULL;
static lv_obj_t *mode_label = NULL;

static void mode_switch_cb(lv_event_t *e) {
  lv_obj_t *sw = lv_event_get_target(e);
  bool state = lv_obj_has_state(sw, LV_STATE_CHECKED);

  if (state) {
    motor_control_mode = MOTOR_CONTROL_MODE_DEBUG;
    lv_label_set_text(mode_label, "Debug");
  } else {
    motor_control_mode = MOTOR_CONTROL_MODE_MECANUM;
    lv_label_set_text(mode_label, "Mecanum");
  }

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

  for (int i = 0; i < 3; i++) {
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
}

void create_motor_debug_page(void) {
  motor_debug_page = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(motor_debug_page, lv_color_hex(0x003a57),
                            LV_PART_MAIN);

  lv_obj_t *title = lv_label_create(motor_debug_page);

  lv_label_set_text(title, "Motor Debug");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  // Mode Switch Button (Toggle Switch)
  mode_switch_btn = lv_switch_create(motor_debug_page);
  lv_obj_set_size(mode_switch_btn, 50, 25);
  lv_obj_align(mode_switch_btn, LV_ALIGN_TOP_RIGHT, -10, 10);

  mode_label = lv_label_create(motor_debug_page);
  lv_obj_set_style_text_font(mode_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(mode_label, lv_palette_main(LV_PALETTE_RED), 0);
  lv_obj_align_to(mode_label, mode_switch_btn, LV_ALIGN_OUT_LEFT_MID, -5, 0);

  if (motor_control_mode == MOTOR_CONTROL_MODE_DEBUG) {
    lv_obj_add_state(mode_switch_btn, LV_STATE_CHECKED);
    lv_label_set_text(mode_label, "Debug");
  } else {
    lv_obj_clear_state(mode_switch_btn, LV_STATE_CHECKED);
    lv_label_set_text(mode_label, "Mecanum");
  }

  lv_obj_add_event_cb(mode_switch_btn, mode_switch_cb, LV_EVENT_VALUE_CHANGED,
                      NULL);

  // Container for list
  lv_obj_t *cont = lv_obj_create(motor_debug_page);
  lv_obj_set_size(cont, LV_PCT(100), LV_PCT(85));
  lv_obj_align(cont, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_bg_opa(cont, 0, 0);
  lv_obj_set_style_border_width(cont, 0, 0);
  lv_obj_set_style_pad_all(cont, 2, 0);
  lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START,
                        LV_FLEX_ALIGN_START);

  for (int i = 0; i < 4; i++) {
    lv_obj_t *card = lv_obj_create(cont);
    lv_obj_set_size(card, LV_PCT(48), 110);
    lv_obj_set_style_bg_color(card, lv_color_hex(0x202020), 0);
    lv_obj_set_style_radius(card, 5, 0);
    lv_obj_set_style_pad_all(card, 5, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title_m = lv_label_create(card);
    lv_label_set_text_fmt(title_m, "Motor %d", i);
    lv_obj_set_style_text_color(title_m, lv_color_hex(0xFFA500), 0);
    lv_obj_align(title_m, LV_ALIGN_TOP_MID, 0, -2);
    lv_obj_set_style_text_font(title_m, &lv_font_montserrat_14, 0);

    lbl_target[i] = lv_label_create(card);
    lv_obj_align(lbl_target[i], LV_ALIGN_TOP_LEFT, 0, 18);
    lv_obj_set_style_text_font(lbl_target[i], &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_target[i], lv_color_white(), 0);

    lbl_speed[i] = lv_label_create(card);
    lv_obj_align(lbl_speed[i], LV_ALIGN_TOP_LEFT, 0, 36);
    lv_obj_set_style_text_font(lbl_speed[i], &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_speed[i], lv_color_white(), 0);

    lbl_dist[i] = lv_label_create(card);
    lv_obj_align(lbl_dist[i], LV_ALIGN_TOP_LEFT, 0, 54);
    lv_obj_set_style_text_font(lbl_dist[i], &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_dist[i], lv_color_white(), 0);

    lbl_pwm[i] = lv_label_create(card);
    lv_obj_align(lbl_pwm[i], LV_ALIGN_TOP_LEFT, 0, 72);
    lv_obj_set_style_text_font(lbl_pwm[i], &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lbl_pwm[i], lv_color_white(), 0);

    // Initial text
    lv_label_set_text(lbl_target[i], "Tgt: 0.0");
    lv_label_set_text(lbl_speed[i], "Spd: 0.0");
    lv_label_set_text(lbl_dist[i], "Dst: 0.00m");
    lv_label_set_text(lbl_pwm[i], "PWM: 0");
  }

  // 返回按钮
  lv_obj_t *back_btn = lv_btn_create(motor_debug_page);
  lv_obj_set_size(back_btn, 70, 30);
  lv_obj_set_pos(back_btn, 0, 0);
  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
  lv_obj_center(back_label);
  lv_obj_add_event_cb(back_btn, switch_page_cb, LV_EVENT_CLICKED, "app_page");

  lv_obj_add_event_cb(motor_debug_page, motor_debug_page_delete_cb,
                      LV_EVENT_DELETE, NULL);

  update_timer = lv_timer_create(update_task, 200, NULL);
}
