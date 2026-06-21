#ifndef PWM_H
#define PWM_H

#include <stdint.h>

void pwm_init(int gpio_num, int channel, int freq_hz);
void pwm_set_duty(int channel, int duty);

#endif
