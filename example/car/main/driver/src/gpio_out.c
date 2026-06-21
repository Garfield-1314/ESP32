#include "driver/inc/gpio_out.h"

#include "driver/gpio.h"

void gpio_out_init(int gpio_num, int level) {
  gpio_reset_pin(gpio_num);
  gpio_set_direction(gpio_num, GPIO_MODE_OUTPUT);
  gpio_set_level(gpio_num, level);
}

void gpio_out_set(int gpio_num, int level) { gpio_set_level(gpio_num, level); }
