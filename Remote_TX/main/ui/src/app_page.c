#include "ui/inc/app_page.h"
#include "ui/inc/main_page.h"
#include "ui/inc/ui.h"

//创建APP界面
lv_obj_t *app_page;

// 在页面退出时自动清除
static void app_page_delete_cb(lv_event_t *e) {
    app_page = NULL;
}

lv_obj_t * create_app_page(void) {
    app_page = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(app_page, lv_color_hex(0x003a57), LV_PART_MAIN);
    lv_obj_t *title = lv_label_create(app_page);

    lv_label_set_text(title, "Apps");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    // 返回按钮
    lv_obj_t *back_btn = lv_btn_create(app_page);
    lv_obj_set_size(back_btn, 70, 30);
    lv_obj_set_pos(back_btn, 0, 0);
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, switch_page_cb, LV_EVENT_CLICKED, "main_page");


    // 设置按钮
    lv_obj_t *settings = lv_btn_create(app_page);
    lv_obj_set_size(settings, 60, 60);
    lv_obj_align(settings, LV_ALIGN_TOP_MID, 105, 40);
    lv_obj_t *settings_label = lv_label_create(settings);
    lv_label_set_text(settings_label, LV_SYMBOL_SETTINGS);
    lv_obj_center(settings_label);
    // lv_obj_add_event_cb(settings, switch_page_cb, LV_EVENT_CLICKED, "settings");

    lv_obj_add_event_cb(app_page, app_page_delete_cb, LV_EVENT_DELETE, NULL);

    return app_page;
}
