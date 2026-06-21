#ifndef GPIO_OUT_H
#define GPIO_OUT_H

#include <stdint.h>

void gpio_out_init(int gpio_num, int level);
void gpio_out_set(int gpio_num, int level);

#endif
