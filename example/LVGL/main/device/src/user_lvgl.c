#include "device/inc/user_lvgl.h"

#include "device/inc/gt911.h"
#include "device/inc/st7789.h"
#include "esp_timer.h"
#include "lvgl.h"

static gt911_dev_t gt911_dev;

static uint32_t last_touch_time = 0;
static bool last_touch_state = false;
static lv_point_t last_touch_point = {0, 0};
#define TOUCH_DEBOUNCE_MS 50
#define TOUCH_POSITION_THRESHOLD 10

static void lvgl_touch_read_cb(lv_indev_drv_t *indev_drv,
                               lv_indev_data_t *data)
{
  uint32_t current_time = lv_tick_get();

  gt911_read(&gt911_dev);
  bool is_touched = gt911_is_touched(&gt911_dev);
  if (is_touched) {
    tp_point_t p = gt911_get_point(&gt911_dev, 0);
    lv_point_t current_point;

    int16_t x_calc = 320 - p.y;
    int16_t y_calc = p.x;

    if (x_calc < 0) x_calc = 0;
    if (x_calc > 319) x_calc = 319;
    if (y_calc < 0) y_calc = 0;
    if (y_calc > 239) y_calc = 239;

    current_point.x = x_calc;
    current_point.y = y_calc;

    if (!last_touch_state) {
      if (current_time - last_touch_time > TOUCH_DEBOUNCE_MS) {
        data->point = current_point;
        data->state = LV_INDEV_STATE_PR;
        last_touch_state = true;
        last_touch_point = current_point;
        last_touch_time = current_time;
      } else {
        data->state = LV_INDEV_STATE_REL;
      }
    } else {
      data->point = current_point;
      data->state = LV_INDEV_STATE_PR;
      last_touch_point = current_point;
      last_touch_time = current_time;
    }
  } else {
    data->state = LV_INDEV_STATE_REL;
    if (last_touch_state) {
      last_touch_state = false;
      last_touch_time = current_time;
    }
  }
}

static lv_color_t lvgl_buf1[LCD_WIDTH * (LCD_HEIGHT / 4)]
    __attribute__((aligned(32)));
static lv_color_t lvgl_buf2[LCD_WIDTH * (LCD_HEIGHT / 4)]
    __attribute__((aligned(32)));

static bool lcd_flush_ready_callback(void *user_ctx)
{
  lv_disp_drv_t *disp_drv = (lv_disp_drv_t *)user_ctx;
  lv_disp_flush_ready(disp_drv);
  return false;
}

void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area,
                   lv_color_t *color_p)
{
  if (area == NULL || color_p == NULL) {
    lv_disp_flush_ready(disp_drv);
    return;
  }
  if (area->x2 < area->x1 || area->y2 < area->y1) {
    lv_disp_flush_ready(disp_drv);
    return;
  }
  int32_t w = area->x2 - area->x1 + 1;
  int32_t h = area->y2 - area->y1 + 1;
  if (w <= 0 || h <= 0) {
    lv_disp_flush_ready(disp_drv);
    return;
  }

  if (lcd_st7789_draw_image(area->x1, area->y1, w, h,
                            (const uint16_t *)color_p) != ESP_OK) {
    lv_disp_flush_ready(disp_drv);
  }
}

void user_lvgl_init(void)
{
  lcd_st7789_init();
  ST7789_test_pattern();
  lv_init();

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  lcd_st7789_register_flush_ready_cb(lcd_flush_ready_callback, &disp_drv);

  disp_drv.hor_res = LCD_WIDTH;
  disp_drv.ver_res = LCD_HEIGHT;
  disp_drv.flush_cb = lvgl_flush_cb;

  static lv_disp_draw_buf_t disp_buf;
  lv_disp_draw_buf_init(&disp_buf, lvgl_buf1, lvgl_buf2,
                        LCD_WIDTH * (LCD_HEIGHT / 4));
  disp_drv.draw_buf = &disp_buf;
  lv_disp_drv_register(&disp_drv);

  gt911_init_default(&gt911_dev);
  gt911_set_resolution(&gt911_dev, LCD_HEIGHT, LCD_WIDTH);
  gt911_set_rotation(&gt911_dev, ROTATION_INVERTED);

  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = lvgl_touch_read_cb;
  lv_indev_drv_register(&indev_drv);
}