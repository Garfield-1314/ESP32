#include "device/inc/buzzer.h"

#include "driver/gpio.h"

#define BUZZER_GPIO 36

void buzzer_init(void) {
  gpio_config_t io_conf = {.intr_type = GPIO_INTR_DISABLE,
                           .mode = GPIO_MODE_OUTPUT,
                           .pin_bit_mask = (1ULL << BUZZER_GPIO),
                           .pull_down_en = 0,
                           .pull_up_en = 0};
  gpio_config(&io_conf);
  gpio_set_level(BUZZER_GPIO, 0);  // Default off
}

void buzzer_on(void) { gpio_set_level(BUZZER_GPIO, 1); }

void buzzer_off(void) { gpio_set_level(BUZZER_GPIO, 0); }
