#ifndef __NUM_SHOW_PAGE_H__
#define __NUM_SHOW_PAGE_H__

#include "lvgl.h"

extern lv_obj_t *num_show_page;

void create_num_show_page(void);
void show_num_on_page(int num1, int num2, int num3, int num4, int num5,
                      int num6);
void num_show_page_update(int num1, int num2, int num3, int num4, int num5,
                          int num6);
void num_show_page_set_init_done(void);

#endif /* __NUM_SHOW_H__ */
