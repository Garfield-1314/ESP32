#ifndef __ST7789_H
#define __ST7789_H

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LCD_WIDTH  320
#define LCD_HEIGHT 240

// LCD引脚定义
#define LCD_SPI_HOST   SPI3_HOST
#define LCD_PIN_SDO   -1
#define LCD_PIN_SDA   36
#define LCD_PIN_CLK   35
#define LCD_PIN_CS    37
#define LCD_PIN_DC    38
#define LCD_PIN_RST   47
#define LCD_PIN_BL    48

// LCD引脚配置结构
typedef struct {
  int spi_host;      // SPI主机 (SPI2_HOST 或 SPI3_HOST)
  int pin_mosi;      // MOSI引脚
  int pin_clk;       // 时钟引脚
  int pin_cs;        // 片选引脚
  int pin_dc;        // 数据/命令引脚
  int pin_rst;       // 复位引脚
  int pin_bl;        // 背光引脚
  int lcd_width;     // 屏幕宽度
  int lcd_height;    // 屏幕高度
  int spi_freq_hz;   // SPI频率
} lcd_config_t;

/**
 * @brief Initialize ST7789 LCD display
 */
esp_err_t lcd_st7789_init(void);

/**
 * @brief Get LCD panel handle
 */
esp_lcd_panel_handle_t lcd_st7789_get_panel_handle(void);

/**
 * @brief Draw a rectangle
 */
void lcd_st7789_draw_rect(int x1, int y1, int x2, int y2, uint16_t color);

/**
 * @brief Fill the entire screen
 */
void lcd_st7789_fill_screen(uint16_t color);

/**
 * @brief Draw a bitmap
 */
void lcd_st7789_draw_bitmap(int x1, int y1, int x2, int y2,
                            const uint16_t *color_data);

/**
 * @brief Draw an image
 */
esp_err_t lcd_st7789_draw_image(int x, int y, int w, int h,
                                const uint16_t *image);

/**
 * @brief Draw a single pixel
 */
void lcd_st7789_draw_pixel(int x, int y, uint16_t color);

/**
 * @brief Fill a rectangular area
 */
void lcd_st7789_fill_rect(int x, int y, int w, int h, uint16_t color);

/**
 * @brief Get current backlight brightness
 * @return brightness percentage (0-100)
 */
uint8_t lcd_st7789_get_brightness(void);

/**
 * @brief Set backlight brightness via PWM
 * @param percent brightness percentage (0 = off, 100 = brightest)
 */
void lcd_st7789_set_brightness(uint8_t percent);

/**
 * @brief Set backlight on/off (compatibility)
 */
void lcd_st7789_set_backlight(bool on);

/**
 * @brief Get screen width
 */
int lcd_st7789_get_width(void);

/**
 * @brief Get screen height
 */
int lcd_st7789_get_height(void);

/**
 * @brief LCD flush ready callback type
 */
typedef bool (*lcd_flush_ready_cb_t)(void *user_ctx);

/**
 * @brief Register LCD flush ready callback
 */
void lcd_st7789_register_flush_ready_cb(lcd_flush_ready_cb_t cb,
                                        void *user_ctx);

void ST7789_test_pattern(void);

#ifdef __cplusplus
}
#endif

#endif  // LCD_ST7789_H