#ifndef ENCODER_H
#define ENCODER_H

#include <stdint.h>

void encoder_init(int unit, int gpio_a, int gpio_b);
int encoder_get_count(int unit);
void encoder_clear_count(int unit);

#endif