#include "ui/inc/num_show_page.h"

#include "driver/uart.h"
#include "stdio.h"
#include "string.h"
#include "ui/inc/ui.h"
LV_FONT_DECLARE(gcxl_large_font_80);

lv_obj_t *num_show_page = NULL;
static lv_obj_t *num_label = NULL;
static lv_obj_t *status_label = NULL;

// START 按钮事件回调：向 OpenMV 发送 "start" 指令
static void start_btn_cb(lv_event_t *e) {
  const char *msg = "start\n";
  uart_write_bytes(UART_NUM_0, msg, strlen(msg));
  vTaskDelay(pdMS_TO_TICKS(10));
  uart_write_bytes(UART_NUM_0, msg, strlen(msg));
}

void num_show_page_delete_cb(lv_event_t *e) {
  num_show_page = NULL;
  num_label = NULL;
}

void create_num_show_page(void) {
  num_show_page = lv_obj_create(NULL);
  lv_obj_set_style_bg_color(num_show_page, lv_color_hex(0x003a57),
                            LV_PART_MAIN);
  lv_obj_t *title = lv_label_create(num_show_page);

  lv_label_set_text(title, "Number Display");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

  // 返回按钮
  lv_obj_t *back_btn = lv_btn_create(num_show_page);
  lv_obj_set_size(back_btn, 70, 30);
  lv_obj_set_pos(back_btn, 0, 0);
  lv_obj_t *back_label = lv_label_create(back_btn);
  lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
  lv_obj_center(back_label);
  lv_obj_add_event_cb(back_btn, switch_page_cb, LV_EVENT_CLICKED, "app_page");

  // START 按钮 (右上角, 红色)
  lv_obj_t *start_btn = lv_btn_create(num_show_page);
  lv_obj_set_size(start_btn, 70, 30);
  lv_obj_align(start_btn, LV_ALIGN_TOP_RIGHT, -5, 5);
  lv_obj_set_style_bg_color(start_btn, lv_color_hex(0xFF0000), LV_PART_MAIN);
  lv_obj_set_style_bg_color(start_btn, lv_color_hex(0xCC0000), LV_STATE_PRESSED);
  lv_obj_t *start_label = lv_label_create(start_btn);
  lv_label_set_text(start_label, "START");
  lv_obj_set_style_text_color(start_label, lv_color_white(), 0);
  lv_obj_center(start_label);
  lv_obj_add_event_cb(start_btn, start_btn_cb, LV_EVENT_CLICKED, NULL);

  // 状态指示标签 (START 按钮下方, 橙色)
  status_label = lv_label_create(num_show_page);
  lv_label_set_text(status_label, "wait OpenMV init...");
  lv_obj_set_style_text_color(status_label, lv_color_hex(0xFFA500), 0);
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
  lv_obj_align(status_label, LV_ALIGN_TOP_RIGHT, -5, 42);

  show_num_on_page(0, 0, 0, 0, 0, 0);

  lv_obj_add_event_cb(num_show_page, num_show_page_delete_cb, LV_EVENT_DELETE,
                      NULL);
}

void show_num_on_page(int num1, int num2, int num3, int num4, int num5,
                      int num6) {
  if (num_show_page == NULL) {
    return;
  }

  char buf[64];
  snprintf(buf, sizeof(buf), "%d %d %d \n   +\n%d %d %d", num1, num2, num3,
           num4, num5, num6);

  if (num_label == NULL) {
    num_label = lv_label_create(num_show_page);
    lv_obj_set_style_text_font(num_label, &gcxl_large_font_80, 0);
    lv_obj_set_style_text_color(num_label, lv_color_white(), 0);
    lv_obj_align(num_label, LV_ALIGN_TOP_MID, 0, 50);
  }

  lv_label_set_text(num_label, buf);
}

void num_show_page_update(int num1, int num2, int num3, int num4, int num5,
                          int num6) {
  if (num_show_page == NULL) {
    return;
  }

  char buf[64];
  snprintf(buf, sizeof(buf), "%d %d %d \n   +\n%d %d %d", num1, num2, num3,
           num4, num5, num6);

  if (num_label == NULL) {
    /* If no label exists (maybe page created earlier without showing), create
     * it */
    num_label = lv_label_create(num_show_page);
    lv_obj_set_style_text_font(num_label, &gcxl_large_font_80, 0);
    lv_obj_set_style_text_color(num_label, lv_color_white(), 0);
    lv_obj_align(num_label, LV_ALIGN_TOP_MID, 0, 50);
  }

  /* Replace displayed numbers */
  lv_label_set_text(num_label, buf);
}

void num_show_page_set_init_done(void) {
  if (status_label != NULL) {
    lv_label_set_text(status_label, "OpenMV Ready");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x00FF00), 0);  // 绿色
  }
}
