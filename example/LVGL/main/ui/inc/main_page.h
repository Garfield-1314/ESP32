#ifndef __MAIN_PAGE_H
#define __MAIN_PAGE_H

#include "lvgl.h"
// 创建主页面
extern lv_obj_t *main_page;
lv_obj_t * create_main_page(void);

// 主页面删除回调
void main_page_delete_cb(lv_event_t *e);


#endif