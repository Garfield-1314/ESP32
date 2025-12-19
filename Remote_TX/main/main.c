#include "include.h"


static void user_component_init(void){
    tusb_serial_init();
    user_lvgl_init();
}

void app_main(void) {
    user_component_init();
}
