#include "ui/inc/ui.h"
#include "ui/inc/main_page.h"
#include "ui/inc/app_page.h"

void switch_page_cb(lv_event_t * e) {
    const char * page_name = (const char *)lv_event_get_user_data(e);
    lv_obj_t * target = NULL;

    if(strcmp(page_name, "main_page") == 0) {
        target = create_main_page();
    } else if(strcmp(page_name, "app_page") == 0) {
        target = create_app_page();
    }

    if(target) {
        lv_scr_load_anim(target, LV_SCR_LOAD_ANIM_NONE, 0, 0, true);
    }
}

void create_ui(void) {
    lv_obj_t * page = create_main_page();
    lv_scr_load(page);
}
