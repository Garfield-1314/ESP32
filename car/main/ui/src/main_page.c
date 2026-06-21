#include "ui/inc/main_page.h"

#include "Algorithm/inc/Mecanum.h"
#include "device/inc/motor.h"
#include "driver/inc/nvs.h"
#include "esp_log.h"
#include "ui/inc/app_page.h"
#include "ui/inc/ui.h"

// LVGL图片声明（来自openmv_logo.c 和 singtown_logo.c）
extern const lv_img_dsc_t resized_openmv_logo;
extern const lv_img_dsc_t singtown_logo;

static const char *TAG = "Model_NVS";

bool show_logo = true;

// 创建主页面
lv_obj_t *main_page;

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

static const char *mode_names[] = {
    "Mecanum",  // MOTOR_CONTROL_MODE_MECANUM = 0
    "Normal",   // MOTOR_CONTROL_MODE_DEBUG = 1
    "Balance"   // MOTOR_CONTROL_MODE_BALANCE = 2
};

static void update_mode_button_style(lv_obj_t *btn) {
  lv_color_t color;

  switch (motor_control_mode) {
    case MOTOR_CONTROL_MODE_MECANUM:
      color = lv_palette_main(LV_PALETTE_RED);
      break;
    case MOTOR_CONTROL_MODE_DEBUG:
      color = lv_palette_main(LV_PALETTE_GREEN);
      break;
    case MOTOR_CONTROL_MODE_BALANCE:
      color = lv_palette_main(LV_PALETTE_ORANGE);
      break;
    default:
      color = lv_palette_main(LV_PALETTE_GREY);
      break;
  }

  lv_obj_set_style_bg_color(btn, color, LV_PART_MAIN);
  lv_obj_set_style_bg_color(btn, color, LV_STATE_PRESSED);
  lv_obj_set_style_bg_color(btn, color, LV_STATE_FOCUSED);
  lv_obj_set_style_bg_color(btn, color, LV_STATE_DEFAULT);
}

static void clear_motor_state(void) {
  for (int i = 0; i < 4; i++) {
    motor_status.encoder_number[i] = 0;
    motor_status.speed_mm_s[i] = 0;
    motor_status.target_speed_mm_s[i] = 0;
    motor_status.distance_mm[i] = 0;
    motor_status.distance_m[i] = 0;
    motor_status.pid_output[i] = 0;
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

// 在标题下方显示Logo（openmv_logo在原来位置，singtown_logo替换原来"Smart Control System"文字）
static void show_logo_on_main_page(lv_obj_t *page) {
  // Singtown Logo - 放在原来" Smart Car\n Control\n System"文字位置
  lv_obj_t *img_singtown = lv_img_create(page);
  lv_img_set_src(img_singtown, &singtown_logo);
  lv_obj_set_style_img_recolor(img_singtown, lv_color_white(), 0);
  lv_obj_set_style_img_recolor_opa(img_singtown, LV_OPA_COVER, 0);
  lv_obj_set_pos(img_singtown, 5, 10);

  // OpenMV Logo - 上移一点
  lv_obj_t *img_openmv = lv_img_create(page);
  lv_img_set_src(img_openmv, &resized_openmv_logo);
  lv_obj_set_style_img_recolor(img_openmv, lv_color_white(), 0);
  lv_obj_set_style_img_recolor_opa(img_openmv, LV_OPA_COVER, 0);
  lv_obj_align(img_openmv, LV_ALIGN_TOP_MID, 0, 65);

  // "Smart Car" 文字 - 放在OpenMV Logo下方
  lv_obj_t *title = lv_label_create(page);
  lv_label_set_text(title, "Smart Car");
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align_to(title, img_openmv, LV_ALIGN_OUT_BOTTOM_MID, 0, 5);
}

static void mode_switch_cb(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target(e);

  // 按顺序切换: Mecanum(0) -> Normal(1) -> Balance(2) -> Mecanum(0) -> ...
  motor_control_mode = (motor_control_mode + 1) % 3;

  lv_label_set_text(lv_obj_get_child(obj, 0), mode_names[motor_control_mode]);
  update_mode_button_style(obj);

  model_nvs_save();
  clear_motor_state();
}

lv_obj_t *create_main_page(void) {
  model_nvs_init();
  main_page = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(main_page, lv_color_hex(0x003a57), LV_PART_MAIN);

  // Mode Switch Button (循环切换: Mecanum -> Normal -> Balance -> Mecanum -> ...)
  lv_obj_t *mode_btn = lv_btn_create(main_page);
  lv_obj_set_size(mode_btn, 80, 30);
  lv_obj_align(mode_btn, LV_ALIGN_TOP_RIGHT, -80, 10);

  lv_obj_t *mode_label = lv_label_create(mode_btn);
  lv_obj_center(mode_label);
  lv_label_set_text(mode_label, mode_names[motor_control_mode]);

  lv_obj_add_event_cb(mode_btn, mode_switch_cb, LV_EVENT_CLICKED, NULL);
  update_mode_button_style(mode_btn);

  // Motor Enable Button
  lv_obj_t *en_btn = lv_btn_create(main_page);
  lv_obj_set_size(en_btn, 60, 30);
  lv_obj_align(en_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
  lv_obj_add_flag(en_btn, LV_OBJ_FLAG_CHECKABLE);

  lv_obj_t *en_label = lv_label_create(en_btn);
  lv_obj_center(en_label);

  if (is_motor_enabled) {
    lv_obj_add_state(en_btn, LV_STATE_CHECKED);
    lv_label_set_text(en_label, "ON");
  } else {
    lv_obj_clear_state(en_btn, LV_STATE_CHECKED);
    lv_label_set_text(en_label, "OFF");
  }
  lv_obj_add_event_cb(en_btn, motor_enable_cb, LV_EVENT_VALUE_CHANGED, NULL);

  // 显示Singtown Logo（根据设置开关控制）
  if (show_logo) {
    show_logo_on_main_page(main_page);
  }

  // APP按钮
  lv_obj_t *nav_apps = lv_btn_create(main_page);
  lv_obj_set_size(nav_apps, 60, 40);
  lv_obj_set_pos(nav_apps, 130,
                 240 - 60);  // 屏幕底部中央 (320/2 - 30, 240 - 60)
  lv_obj_t *nav_apps_label = lv_label_create(nav_apps);
  lv_label_set_text(nav_apps_label, LV_SYMBOL_HOME);
  lv_obj_align(nav_apps_label, LV_ALIGN_CENTER, 0,
               0);  // 标签在按钮内居中可以保留align
  lv_obj_add_event_cb(nav_apps, switch_page_cb, LV_EVENT_CLICKED, "app_page");

  lv_obj_add_event_cb(main_page, main_page_delete_cb, LV_EVENT_DELETE, NULL);

  return main_page;
}

// 在页面退出时自动清除
void main_page_delete_cb(lv_event_t *e) { main_page = NULL; }

void model_nvs_init() {
  esp_err_t err = nvs_init_custom();
  if (err == ESP_OK) {
    model_nvs_load();
  } else {
    ESP_LOGE(TAG, "NVS Init Failed");
  }
}

void model_nvs_load() {
  nvs_read_i32("motor_mode", (int32_t *)&motor_control_mode);
  int32_t logo_val = 1;
  nvs_read_i32("show_logo", &logo_val);
  show_logo = (logo_val != 0);
}

void model_nvs_save() {
  nvs_write_i32("motor_mode", (int32_t)motor_control_mode);
  nvs_write_i32("show_logo", (int32_t)(show_logo ? 1 : 0));
}