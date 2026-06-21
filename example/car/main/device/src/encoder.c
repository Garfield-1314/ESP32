#include "device/inc/encoder.h"

#include "driver/pulse_cnt.h"
#include "esp_log.h"

static pcnt_unit_handle_t pcnt_units[4] = {NULL};

void encoder_init(int unit, int gpio_a, int gpio_b) {
  if (unit < 0 || unit >= 4) return;

  pcnt_unit_config_t unit_config = {
      .high_limit = 32767,
      .low_limit = -32768,
  };
  ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_units[unit]));

  pcnt_glitch_filter_config_t filter_config = {
      .max_glitch_ns = 1000,
  };
  ESP_ERROR_CHECK(
      pcnt_unit_set_glitch_filter(pcnt_units[unit], &filter_config));

  pcnt_chan_config_t chan_a_config = {
      .edge_gpio_num = gpio_a,
      .level_gpio_num = gpio_b,
  };
  pcnt_channel_handle_t pcnt_chan_a = NULL;
  ESP_ERROR_CHECK(
      pcnt_new_channel(pcnt_units[unit], &chan_a_config, &pcnt_chan_a));

  pcnt_chan_config_t chan_b_config = {
      .edge_gpio_num = gpio_b,
      .level_gpio_num = gpio_a,
  };
  pcnt_channel_handle_t pcnt_chan_b = NULL;
  ESP_ERROR_CHECK(
      pcnt_new_channel(pcnt_units[unit], &chan_b_config, &pcnt_chan_b));

  ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
      pcnt_chan_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE,
      PCNT_CHANNEL_EDGE_ACTION_INCREASE));
  ESP_ERROR_CHECK(
      pcnt_channel_set_level_action(pcnt_chan_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                    PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

  ESP_ERROR_CHECK(pcnt_channel_set_edge_action(
      pcnt_chan_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE,
      PCNT_CHANNEL_EDGE_ACTION_DECREASE));
  ESP_ERROR_CHECK(
      pcnt_channel_set_level_action(pcnt_chan_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                    PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

  ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_units[unit]));
  ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_units[unit]));
  ESP_ERROR_CHECK(pcnt_unit_start(pcnt_units[unit]));
}

int encoder_get_count(int unit) {
  if (unit < 0 || unit >= 4 || pcnt_units[unit] == NULL) return 0;
  int count = 0;
  pcnt_unit_get_count(pcnt_units[unit], &count);
  return count;
}

void encoder_clear_count(int unit) {
  if (unit < 0 || unit >= 4 || pcnt_units[unit] == NULL) return;
  pcnt_unit_clear_count(pcnt_units[unit]);
}
