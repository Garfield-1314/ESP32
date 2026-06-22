#include "device/inc/st7789.h"

#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_heap_caps.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "LCD_ST7789";

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle = NULL;
static int lcd_width = 0;
static int lcd_height = 0;
static int backlight_pin = -1;
static uint8_t s_current_brightness = 100;

static lcd_flush_ready_cb_t flush_ready_cb = NULL;
static void *flush_ready_user_ctx = NULL;

static bool lcd_panel_io_color_trans_done(esp_lcd_panel_io_handle_t panel_io,
                                          esp_lcd_panel_io_event_data_t *edata,
                                          void *user_ctx)
{
  if (flush_ready_cb) {
    return flush_ready_cb(flush_ready_user_ctx);
  }
  return false;
}

void lcd_st7789_register_flush_ready_cb(lcd_flush_ready_cb_t cb,
                                        void *user_ctx)
{
  flush_ready_cb = cb;
  flush_ready_user_ctx = user_ctx;
}

static lcd_config_t config = {
    .spi_host = LCD_SPI_HOST,
    .pin_mosi = LCD_PIN_SDA,
    .pin_clk = LCD_PIN_CLK,
    .pin_cs = LCD_PIN_CS,
    .pin_dc = LCD_PIN_DC,
    .pin_rst = LCD_PIN_RST,
    .pin_bl = LCD_PIN_BL,
    .lcd_width = LCD_WIDTH,
    .lcd_height = LCD_HEIGHT,
    .spi_freq_hz = 40 * 1000 * 1000,
};

esp_err_t lcd_st7789_init(void)
{
  esp_err_t ret = ESP_OK;
  lcd_width = config.lcd_width;
  lcd_height = config.lcd_height;
  backlight_pin = config.pin_bl;
  ESP_LOGI(TAG, "Init ST7789 LCD (%dx%d)", lcd_width, lcd_height);

  if (config.pin_bl >= 0) {
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .freq_hz = 5000,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_channel = {
        .gpio_num = config.pin_bl,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
  }

  spi_bus_config_t buscfg = {
      .miso_io_num = -1,
      .mosi_io_num = config.pin_mosi,
      .sclk_io_num = config.pin_clk,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = config.lcd_width * config.lcd_height * 2,
      .flags = SPICOMMON_BUSFLAG_MASTER | SPICOMMON_BUSFLAG_GPIO_PINS,
  };

  ret = spi_bus_initialize(config.spi_host, &buscfg, SPI_DMA_CH_AUTO);
  ESP_RETURN_ON_ERROR(ret, TAG, "SPI bus init failed");

  esp_lcd_panel_io_spi_config_t io_config = {
      .dc_gpio_num = config.pin_dc,
      .cs_gpio_num = config.pin_cs,
      .pclk_hz = config.spi_freq_hz,
      .lcd_cmd_bits = 8,
      .lcd_param_bits = 8,
      .spi_mode = 0,
      .trans_queue_depth = 10,
      .on_color_trans_done = lcd_panel_io_color_trans_done,
      .user_ctx = NULL,
      .flags = {
          .dc_low_on_data = 0,
          .octal_mode = 0,
          .lsb_first = 0,
          .cs_high_active = 0,
      },
  };

  ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)config.spi_host,
                                 &io_config, &io_handle);
  ESP_RETURN_ON_ERROR(ret, TAG, "LCD IO attach failed");

  esp_lcd_panel_dev_config_t panel_config = {
      .reset_gpio_num = config.pin_rst,
      .color_space = ESP_LCD_COLOR_SPACE_RGB,
      .bits_per_pixel = 16,
  };

  ret = esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle);
  ret = esp_lcd_panel_reset(panel_handle);
  ret = esp_lcd_panel_init(panel_handle);
  ret = esp_lcd_panel_swap_xy(panel_handle, true);
  ret = esp_lcd_panel_mirror(panel_handle, false, true);
  ret = esp_lcd_panel_disp_on_off(panel_handle, true);

  if (config.pin_bl >= 0) {
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 1023);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
  }

  ESP_LOGI(TAG, "LCD init complete");
  return ESP_OK;
}

esp_lcd_panel_handle_t lcd_st7789_get_panel_handle(void)
{
  return panel_handle;
}

void lcd_st7789_draw_rect(int x1, int y1, int x2, int y2, uint16_t color)
{
  if (panel_handle == NULL) { ESP_LOGE(TAG, "Panel not init"); return; }
  esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2, y2, (void *)&color);
}

void lcd_st7789_fill_screen(uint16_t color)
{
  if (panel_handle == NULL) { ESP_LOGE(TAG, "Panel not init"); return; }

  const int CHUNK_ROWS = 10;
  int chunk_size = lcd_width * CHUNK_ROWS;
  uint16_t *color_data = malloc(chunk_size * sizeof(uint16_t));
  if (color_data == NULL) {
    color_data = malloc(lcd_width * sizeof(uint16_t));
    if (color_data == NULL) { ESP_LOGE(TAG, "malloc fail"); return; }
    for (int i = 0; i < lcd_width; i++) color_data[i] = color;
    for (int y = 0; y < lcd_height; y++)
      esp_lcd_panel_draw_bitmap(panel_handle, 0, y, lcd_width, y + 1, color_data);
  } else {
    for (int i = 0; i < chunk_size; i++) color_data[i] = color;
    for (int y = 0; y < lcd_height; y += CHUNK_ROWS) {
      int rows = (y + CHUNK_ROWS > lcd_height) ? (lcd_height - y) : CHUNK_ROWS;
      esp_lcd_panel_draw_bitmap(panel_handle, 0, y, lcd_width, y + rows, color_data);
    }
  }
  free(color_data);
}

void lcd_st7789_draw_bitmap(int x1, int y1, int x2, int y2,
                            const uint16_t *color_data)
{
  if (panel_handle == NULL || color_data == NULL) return;
  esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2, y2, color_data);
}

esp_err_t lcd_st7789_draw_image(int x, int y, int w, int h,
                                const uint16_t *image)
{
  if (panel_handle == NULL) return ESP_ERR_INVALID_STATE;
  if (image == NULL) return ESP_ERR_INVALID_ARG;
  if (w <= 0 || h <= 0) return ESP_OK;
  if (x < 0 || y < 0 || x + w > lcd_width || y + h > lcd_height) {
    if (x >= lcd_width || y >= lcd_height || x + w <= 0 || y + h <= 0) return ESP_OK;
  }
  return esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + w, y + h, image);
}

void lcd_st7789_draw_pixel(int x, int y, uint16_t color)
{
  if (panel_handle == NULL) return;
  if (x >= lcd_width || y >= lcd_height || x < 0 || y < 0) return;
  esp_lcd_panel_draw_bitmap(panel_handle, x, y, x + 1, y + 1, (void *)&color);
}

void lcd_st7789_fill_rect(int x, int y, int w, int h, uint16_t color)
{
  if (panel_handle == NULL) return;
  if (x >= lcd_width || y >= lcd_height || x < 0 || y < 0) return;
  if (x + w > lcd_width) w = lcd_width - x;
  if (y + h > lcd_height) h = lcd_height - y;

  size_t buffer_size = w * sizeof(uint16_t);
  uint16_t *line_buffer = malloc(buffer_size);
  if (line_buffer == NULL) { ESP_LOGE(TAG, "malloc fail"); return; }
  for (int i = 0; i < w; i++) line_buffer[i] = color;
  for (int row = 0; row < h; row++)
    esp_lcd_panel_draw_bitmap(panel_handle, x, y + row, x + w, y + row + 1, line_buffer);
  free(line_buffer);
}

uint8_t lcd_st7789_get_brightness(void) { return s_current_brightness; }

void lcd_st7789_set_brightness(uint8_t percent)
{
  if (backlight_pin < 0) return;
  if (percent > 100) percent = 100;
  s_current_brightness = percent;
  ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, (uint32_t)percent * 1023 / 100);
  ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}

void lcd_st7789_set_backlight(bool on) { lcd_st7789_set_brightness(on ? 100 : 0); }

int lcd_st7789_get_width(void) { return lcd_width; }
int lcd_st7789_get_height(void) { return lcd_height; }

void ST7789_test_pattern(void)
{
  lcd_st7789_fill_screen(0xF800); vTaskDelay(pdMS_TO_TICKS(1000));
  lcd_st7789_fill_screen(0x07E0); vTaskDelay(pdMS_TO_TICKS(1000));
  lcd_st7789_fill_screen(0x001F); vTaskDelay(pdMS_TO_TICKS(1000));
  lcd_st7789_fill_screen(0xFFFF); vTaskDelay(pdMS_TO_TICKS(1000));
  lcd_st7789_fill_screen(0x0000); vTaskDelay(pdMS_TO_TICKS(1000));
}