#ifndef CMD_PAGE_H
#define CMD_PAGE_H

#include "lvgl.h"

extern lv_obj_t *cmd_page;

lv_obj_t *create_cmd_page(void);
void cmd_page_add_data(const char *data, size_t len);
void cmd_page_process_queue(void);

#endif // CMD_PAGE_H
