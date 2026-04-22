#include "ui/inc/main_page.h"
#include "ui/inc/app_page.h"
#include "ui/inc/ui.h"

// 创建主页面
lv_obj_t *main_page;

lv_obj_t * create_main_page(void) {
    main_page = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(main_page, lv_color_hex(0x003a57), LV_PART_MAIN);

    // 标题
    lv_obj_t *title = lv_label_create(main_page);
    lv_label_set_text(title, "Remote TX");
    lv_obj_set_width(title, 240); // 稍微减小宽度以避开按钮
    lv_obj_set_style_text_align(title, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_18, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_pos(title, 20, 20);


    // 关机按钮
    lv_obj_t *btn_shutdown = lv_btn_create(main_page);
    lv_obj_set_size(btn_shutdown, 40, 40);
    lv_obj_set_pos(btn_shutdown, 270, 10); // 右上角
    // lv_obj_set_style_bg_color(btn_shutdown, lv_color_hex(0xCC0000), 0); // 红色背景

    // show_logo_on_main_page(main_page); // 添加Logo

    // APP按钮
    lv_obj_t *nav_apps = lv_btn_create(main_page);
    lv_obj_set_size(nav_apps, 60, 40);
    lv_obj_set_pos(nav_apps, 130, 240 - 60);  // 屏幕底部中央 (320/2 - 30, 240 - 60)
    lv_obj_t *nav_apps_label = lv_label_create(nav_apps);
    lv_label_set_text(nav_apps_label, LV_SYMBOL_HOME);
    lv_obj_align(nav_apps_label, LV_ALIGN_CENTER, 0, 0);  // 标签在按钮内居中可以保留align
    lv_obj_add_event_cb(nav_apps, switch_page_cb, LV_EVENT_CLICKED, "app_page");

    lv_obj_add_event_cb(main_page, main_page_delete_cb, LV_EVENT_DELETE, NULL);
    
    return main_page;
}

// 在页面退出时自动清除
void main_page_delete_cb(lv_event_t *e) {
    main_page = NULL;
}