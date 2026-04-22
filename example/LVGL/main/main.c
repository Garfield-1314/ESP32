#include "include.h"

gt911_dev_t gt911_dev;

static void user_component_init(void){
    tusb_serial_init();
    vTaskDelay(pdMS_TO_TICKS(1000));
    user_lvgl_init();
    create_ui();
}

void app_main(void) {
    user_component_init();
    
    for(;;){
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
