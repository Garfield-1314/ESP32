#ifndef __MOTOR_PAGE_H__
#define __MOTOR_PAGE_H__

#include "lvgl.h"

extern lv_obj_t *motor_page;
void create_motor_page(void);
void motor_page_delete_cb(lv_event_t *e);

#endif  // __MOTOR_PAGE_H__