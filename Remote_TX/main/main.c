#include "include.h"


static void user_component_init(void){
    tusb_serial_init();
    vTaskDelay(pdMS_TO_TICKS(1000));
    lcd_st7789_init();
    gt911_init();
    user_lvgl_init();
    ui_init();
}

void app_main(void) {
    user_component_init();
    
    for(;;){
        printf("App Main Initialized\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
