#include "ui/inc/ui.h"

#include <string.h>

#include "Algorithm/inc/Mecanum.h"
#include "Algorithm/inc/pid.h"
#include "device/inc/motor.h"
#include "ui/inc/app_page.h"
#include "ui/inc/car_control_page.h"
#include "ui/inc/cmd_page.h"
#include "ui/inc/imu_setting_page.h"
#include "ui/inc/main_page.h"
#include "ui/inc/mecanum_page.h"
#include "ui/inc/motor_debug.h"
#include "ui/inc/motor_page.h"
#include "ui/inc/motor_setting_page.h"
#include "ui/inc/num_show_page.h"
#include "ui/inc/pid_page.h"
#include "ui/inc/setting_page.h"

// 通用消抖函数
bool check_debounce(uint32_t *last_time, uint32_t debounce_ms) {
  uint32_t current_time = lv_tick_get();

  if (current_time - *last_time < debounce_ms) {
    return false;  // 在消抖时间内，忽略
  }

  *last_time = current_time;
  return true;  // 允许执行
}

static void motor_mecanum_status_clear(void) {
  // 1. Motor Status Clear
  for (int i = 0; i < 4; i++) {
    motor_status.encoder_number[i] = 0;
    motor_status.speed_mm_s[i] = 0;
    motor_status.target_speed_mm_s[i] = 0;
    motor_status.distance_mm[i] = 0;
    motor_status.distance_m[i] = 0;
    motor_status.pid_output[i] = 0;

    PID_Reset_State(&motor_status.pid_controller[i]);
  }

  // 2. Mecanum Status Clear
  memset(&mecanum_ctrl.current_pose, 0, sizeof(Mecanum_Pose_t));
  memset(&mecanum_ctrl.target_pose, 0, sizeof(Mecanum_Pose_t));
  memset(&mecanum_ctrl.current_vel, 0, sizeof(Mecanum_Velocity_t));
  memset(&mecanum_ctrl.target_vel, 0, sizeof(Mecanum_Velocity_t));

  PID_Reset_State(&mecanum_ctrl.pid_x);
  PID_Reset_State(&mecanum_ctrl.pid_y);
  PID_Reset_State(&mecanum_ctrl.pid_theta);
}

void switch_page_cb(lv_event_t *e) {
  const char *page_name = (const char *)lv_event_get_user_data(e);
  lv_obj_t *target = NULL;

  // 清零所有状态 (Motor + Mecanum)，保留PID参数 (Kp,Ki,Kd)
  // 每次切换页面时都执行清理，确保只有进入特定页面后才启用对应状态

  motor_mecanum_status_clear();

  if (strcmp(page_name, "main_page") == 0) {
    target = create_main_page();
  } else if (strcmp(page_name, "app_page") == 0) {
    target = create_app_page();
  } else if (strcmp(page_name, "settings_page") == 0) {
    create_setting_page();
    target = setting_page;
  } else if (strcmp(page_name, "pid_page") == 0) {
    create_pid_page();
    target = pid_page;
  } else if (strcmp(page_name, "motor_page") == 0) {
    create_motor_page();
    target = motor_page;
  } else if (strcmp(page_name, "mecanum_page") == 0) {
    create_mecanum_page();
    target = mecanum_page;
  } else if (strcmp(page_name, "num_show_page") == 0) {
    create_num_show_page();
    target = num_show_page;
  } else if (strcmp(page_name, "motor_setting_page") == 0) {
    create_motor_setting_page();
    target = motor_setting_page;
  } else if (strcmp(page_name, "motor_debug_page") == 0) {
    create_motor_debug_page();
    target = motor_debug_page;
  } else if (strcmp(page_name, "car_control_page") == 0) {
    car_control_page_create();
    target = car_control_page;
  } else if (strcmp(page_name, "imu_setting_page") == 0) {
    create_imu_setting_page();
    target = imu_setting_page;
  } else if (strcmp(page_name, "cmd_page") == 0) {
    target = create_cmd_page();
  }

  if (target) {
    lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
  }
}

void create_ui(void) {
  lv_obj_t *page = create_main_page();
  lv_scr_load(page);
}
