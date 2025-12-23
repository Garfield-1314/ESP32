#ifndef UI_H
#define UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

void create_ui(void);
void switch_page_cb(lv_event_t * e);

#ifdef __cplusplus
}
#endif

#endif /* UI_H */
