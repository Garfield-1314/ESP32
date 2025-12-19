#include "include.h"

gt911_dev_t gt911_dev;

static void user_component_init(void){
    tusb_serial_init();
    vTaskDelay(pdMS_TO_TICKS(1000));
    lcd_st7789_init();
    gt911_init_default(&gt911_dev);
    user_lvgl_init();
    ui_init();
}

void app_main(void) {
    user_component_init();
    
    for(;;){
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
