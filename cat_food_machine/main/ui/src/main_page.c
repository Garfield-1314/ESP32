#include "ui/inc/main_page.h"

#include "ui/inc/setting_page.h"
#include "ui/inc/ui.h"

// 创建主页面
lv_obj_t *main_page;

lv_obj_t *create_main_page(void)
{
  main_page = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(main_page, lv_color_hex(0x003a57), LV_PART_MAIN);

  // 标题 - Cat Food Machine
  lv_obj_t *title = lv_label_create(main_page);
  lv_label_set_text(title, "Cat Food\nMachine");
  lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 80);

  // 状态指示灯 - Ready
  lv_obj_t *status_label = lv_label_create(main_page);
  lv_label_set_text(status_label, LV_SYMBOL_OK " System Ready");
  lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0);
  lv_obj_align(status_label, LV_ALIGN_TOP_MID, 0, 130);

  // APP按钮 - 进入设置页面
  lv_obj_t *nav_apps = lv_btn_create(main_page);
  lv_obj_set_size(nav_apps, 60, 40);
  lv_obj_set_pos(nav_apps, 130, 240 - 60);  // 屏幕底部中央
  lv_obj_t *nav_apps_label = lv_label_create(nav_apps);
  lv_label_set_text(nav_apps_label, LV_SYMBOL_SETTINGS);
  lv_obj_align(nav_apps_label, LV_ALIGN_CENTER, 0, 0);
  lv_obj_add_event_cb(nav_apps, switch_page_cb, LV_EVENT_CLICKED, "settings_page");

  lv_obj_add_event_cb(main_page, main_page_delete_cb, LV_EVENT_DELETE, NULL);

  return main_page;
}

void main_page_delete_cb(lv_event_t *e)
{
  main_page = NULL;
}