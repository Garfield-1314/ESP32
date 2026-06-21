#include "ui/inc/cmd_page.h"
#include "ui/inc/ui.h"
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

lv_obj_t *cmd_page;
static lv_obj_t *cmd_text_area;

/* Message queue to decouple UART task from LVGL task */
static QueueHandle_t cmd_msg_queue = NULL;
#define CMD_MSG_QUEUE_LEN 32
#define CMD_MSG_MAX_LEN   256

/* Called from lvgl_event_task (core 1) to drain the queue */
void cmd_page_process_queue(void) {
    if (!cmd_msg_queue) return;

    char buf[CMD_MSG_MAX_LEN];
    while (xQueueReceive(cmd_msg_queue, buf, 0) == pdTRUE) {
        if (cmd_page && cmd_text_area) {
            /* Limit total length to avoid memory issues */
            const char* current_text = lv_textarea_get_text(cmd_text_area);
            if (strlen(current_text) > 2000) {
                lv_textarea_set_text(cmd_text_area, "");
            }
            lv_textarea_add_text(cmd_text_area, buf);
            lv_obj_scroll_to_y(cmd_text_area, LV_COORD_MAX, LV_ANIM_OFF);
        }
    }
}

static void clear_btn_cb(lv_event_t *e) {
    if (cmd_text_area) {
        lv_textarea_set_text(cmd_text_area, "");
    }
}

static void cmd_page_delete_cb(lv_event_t *e) {
    cmd_page = NULL;
    cmd_text_area = NULL;
    if (cmd_msg_queue) {
        vQueueDelete(cmd_msg_queue);
        cmd_msg_queue = NULL;
    }
}

lv_obj_t *create_cmd_page(void) {
    /* Create the message queue once */
    if (cmd_msg_queue == NULL) {
        cmd_msg_queue = xQueueCreate(CMD_MSG_QUEUE_LEN, CMD_MSG_MAX_LEN);
    }

    cmd_page = lv_obj_create(NULL);
    lv_obj_add_event_cb(cmd_page, cmd_page_delete_cb, LV_EVENT_DELETE, NULL);
    lv_obj_set_style_bg_color(cmd_page, lv_color_hex(0x000000), LV_PART_MAIN);

    // Title
    lv_obj_t *title = lv_label_create(cmd_page);
    lv_label_set_text(title, "Terminal");
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 5);

    // Back Button
    lv_obj_t *back_btn = lv_btn_create(cmd_page);
    lv_obj_set_size(back_btn, 70, 30);
    lv_obj_align(back_btn, LV_ALIGN_TOP_LEFT, 5, 5);
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, switch_page_cb, LV_EVENT_CLICKED, "app_page");

    // Clear Button
    lv_obj_t *clear_btn = lv_btn_create(cmd_page);
    lv_obj_set_size(clear_btn, 60, 30);
    lv_obj_align(clear_btn, LV_ALIGN_TOP_RIGHT, -5, 5);
    lv_obj_t *clear_label = lv_label_create(clear_btn);
    lv_label_set_text(clear_label, "Clear");
    lv_obj_center(clear_label);
    lv_obj_add_event_cb(clear_btn, clear_btn_cb, LV_EVENT_CLICKED, NULL);

    // Terminal Text Area
    cmd_text_area = lv_textarea_create(cmd_page);
    lv_obj_set_size(cmd_text_area, 310, 190);
    lv_obj_align(cmd_text_area, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_textarea_set_cursor_click_pos(cmd_text_area, false);
    lv_textarea_set_text(cmd_text_area, "");
    lv_obj_set_style_bg_color(cmd_text_area, lv_color_hex(0x1e1e1e), 0);
    lv_obj_set_style_text_color(cmd_text_area, lv_color_hex(0x00ff00), 0); // Terminal green
    lv_obj_set_style_text_font(cmd_text_area, &lv_font_montserrat_12, 0);
    
    // Make it read-only
    lv_obj_clear_flag(cmd_text_area, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(cmd_text_area, LV_OBJ_FLAG_SCROLLABLE);

    return cmd_page;
}

void cmd_page_add_data(const char *data, size_t len) {
    if (!cmd_msg_queue) return;

    /* Split large payloads into lines and enqueue each */
    size_t start = 0;
    for (size_t i = 0; i < len; i++) {
        if (data[i] == '\n' || data[i] == '\r') {
            if (i > start) {
                size_t line_len = i - start;
                if (line_len > CMD_MSG_MAX_LEN - 1) {
                    line_len = CMD_MSG_MAX_LEN - 1;
                }
                char buf[CMD_MSG_MAX_LEN];
                memcpy(buf, data + start, line_len);
                buf[line_len] = '\0';
                xQueueSend(cmd_msg_queue, buf, 0);  /* non-blocking, drops if full */
            }
            start = i + 1;
        }
    }
    /* Last fragment without trailing newline */
    if (start < len) {
        size_t line_len = len - start;
        if (line_len > CMD_MSG_MAX_LEN - 1) {
            line_len = CMD_MSG_MAX_LEN - 1;
        }
        char buf[CMD_MSG_MAX_LEN];
        memcpy(buf, data + start, line_len);
        buf[line_len] = '\0';
        xQueueSend(cmd_msg_queue, buf, 0);
    }
}