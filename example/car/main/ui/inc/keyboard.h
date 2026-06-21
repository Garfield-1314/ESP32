#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "lvgl.h"

extern lv_obj_t *keyboard;
// 键盘相关函数
void show_keyboard(lv_obj_t *textarea);
void hide_keyboard(void);
void set_keyboard_textarea(lv_obj_t *textarea);
void keyboard_value_changed_cb(lv_event_t *e);
void textarea_click_cb(lv_event_t *e);

#endif
