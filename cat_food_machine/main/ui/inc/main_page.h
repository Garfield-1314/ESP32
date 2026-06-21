#ifndef __MAIN_PAGE_H
#define __MAIN_PAGE_H

#include "lvgl.h"

extern lv_obj_t *main_page;
lv_obj_t *create_main_page(void);
void main_page_delete_cb(lv_event_t *e);

#endif