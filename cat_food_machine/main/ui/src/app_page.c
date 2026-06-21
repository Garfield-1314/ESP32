#include "ui/inc/app_page.h"

#include "ui/inc/setting_page.h"
#include "ui/inc/ui.h"

// 创建APP界面（开机默认显示）
lv_obj_t *app_page;

static void app_page_delete_cb(lv_event_t *e)
{
  app_page = NULL;
}

lv_obj_t *create_app_page(void)
{
  app_page = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(app_page, lv_color_hex(0x003a57), LV_PART_MAIN);

  // 标题
  lv_obj_t *title = lv_label_create(app_page);
  lv_label_set_text(title, "Cat Food Machine");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  // 设置功能按钮（圆形图标）
  lv_obj_t *settings_btn = lv_btn_create(app_page);
  lv_obj_set_size(settings_btn, 60, 60);
  lv_obj_align(settings_btn, LV_ALIGN_TOP_MID, 0, 50);
  lv_obj_t *settings_label = lv_label_create(settings_btn);
  lv_label_set_text(settings_label, LV_SYMBOL_SETTINGS);
  lv_obj_center(settings_label);
  lv_obj_add_event_cb(settings_btn, switch_page_cb, LV_EVENT_CLICKED,
                      "settings_page");

  lv_obj_add_event_cb(app_page, app_page_delete_cb, LV_EVENT_DELETE, NULL);

  return app_page;
}