#include "ui/inc/app_page.h"

#include <string.h>
#include "ui/inc/ui.h"

lv_obj_t *app_page;

/* ========== 演示按钮回调 ========== */

/* 按钮1：闹铃样式演示 */
static void btn_demo1_cb(lv_event_t *e)
{
    (void)e;
    static uint8_t toggle = 0;
    toggle = !toggle;
    lv_obj_t *label = lv_obj_get_child(lv_event_get_target(e), 0);
    if (label) {
        lv_label_set_text(label, toggle ? "ON" : "OFF");
    }
}

/* 按钮2：切换测试颜色 */
static void btn_demo2_cb(lv_event_t *e)
{
    (void)e;
    static uint8_t hue = 0;
    hue = (hue + 40) % 360;
    lv_obj_t *btn = lv_event_get_target(e);
    lv_color_t c = lv_color_hsv_to_rgb(hue, 80, 80);
    lv_obj_set_style_bg_color(btn, c, 0);
}

/* 页面删除回调 */
static void app_page_delete_cb(lv_event_t *e)
{
    app_page = NULL;
}

lv_obj_t *create_app_page(void)
{
    app_page = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(app_page, lv_color_hex(0x003a57), LV_PART_MAIN);

    /* 标题 */
    lv_obj_t *title = lv_label_create(app_page);
    lv_label_set_text(title, "LVGL Example");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* 副标题 */
    lv_obj_t *subtitle = lv_label_create(app_page);
    lv_label_set_text(subtitle, "ST7789 + GT911 + TinyUSB");
    lv_obj_set_style_text_font(subtitle, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(subtitle, lv_color_hex(0x88CCFF), 0);
    lv_obj_align(subtitle, LV_ALIGN_TOP_MID, 0, 30);

    /* 大图标占位 */
    lv_obj_t *icon_label = lv_label_create(app_page);
    lv_label_set_text(icon_label, LV_SYMBOL_SETTINGS " " LV_SYMBOL_WIFI " " LV_SYMBOL_PLAY);
    lv_obj_set_style_text_font(icon_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(icon_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(icon_label, LV_ALIGN_CENTER, 0, -30);

    /* 演示按钮1 */
    lv_obj_t *btn1 = lv_btn_create(app_page);
    lv_obj_set_size(btn1, 100, 40);
    lv_obj_align(btn1, LV_ALIGN_CENTER, -60, 30);
    lv_obj_t *btn1_lbl = lv_label_create(btn1);
    lv_label_set_text(btn1_lbl, "OFF");
    lv_obj_center(btn1_lbl);
    lv_obj_add_event_cb(btn1, btn_demo1_cb, LV_EVENT_CLICKED, NULL);

    /* 演示按钮2 */
    lv_obj_t *btn2 = lv_btn_create(app_page);
    lv_obj_set_size(btn2, 100, 40);
    lv_obj_align(btn2, LV_ALIGN_CENTER, 60, 30);
    lv_obj_t *btn2_lbl = lv_label_create(btn2);
    lv_label_set_text(btn2_lbl, "Color");
    lv_obj_center(btn2_lbl);
    lv_obj_add_event_cb(btn2, btn_demo2_cb, LV_EVENT_CLICKED, NULL);

    /* 状态文本 */
    lv_obj_t *status = lv_label_create(app_page);
    lv_label_set_text(status, "Touch the buttons to test!");
    lv_obj_set_style_text_font(status, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(status, lv_color_hex(0xAAAAAA), 0);
    lv_obj_align(status, LV_ALIGN_BOTTOM_MID, 0, -15);

    /* 底部信息 */
    lv_obj_t *info = lv_label_create(app_page);
    lv_label_set_text(info, "320x240 | ST7789 | LVGL");
    lv_obj_set_style_text_font(info, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(info, lv_color_hex(0x666666), 0);
    lv_obj_align(info, LV_ALIGN_BOTTOM_MID, 0, -3);

    lv_obj_add_event_cb(app_page, app_page_delete_cb, LV_EVENT_DELETE, NULL);

    return app_page;
}