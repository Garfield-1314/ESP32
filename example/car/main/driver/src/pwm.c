#include "driver/inc/pwm.h"

#include "driver/ledc.h"

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_DUTY_RES LEDC_TIMER_13_BIT  // Set duty resolution to 13 bits
// 13 bit resolution means duty cycle is 0-8191

void pwm_init(int gpio_num, int channel, int freq_hz) {
  // Prepare and then apply the LEDC PWM timer configuration
  ledc_timer_config_t ledc_timer = {.speed_mode = LEDC_MODE,
                                    .timer_num = LEDC_TIMER,
                                    .duty_resolution = LEDC_DUTY_RES,
                                    .freq_hz = freq_hz,  // Set output frequency
                                    .clk_cfg = LEDC_AUTO_CLK};
  ledc_timer_config(&ledc_timer);

  // Prepare and then apply the LEDC PWM channel configuration
  ledc_channel_config_t ledc_channel = {.speed_mode = LEDC_MODE,
                                        .channel = (ledc_channel_t)channel,
                                        .timer_sel = LEDC_TIMER,
                                        .intr_type = LEDC_INTR_DISABLE,
                                        .gpio_num = gpio_num,
                                        .duty = 0,  // Set duty to 0%
                                        .hpoint = 0};
  ledc_channel_config(&ledc_channel);
}

void pwm_set_duty(int channel, int duty) {
  ledc_set_duty(LEDC_MODE, (ledc_channel_t)channel, duty);
  ledc_update_duty(LEDC_MODE, (ledc_channel_t)channel);
}
