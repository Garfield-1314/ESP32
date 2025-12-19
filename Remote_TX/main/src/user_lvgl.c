#include "include/user_lvgl.h"
#include "include/st7789.h"
#include "include/gt911.h"
#include "lvgl.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "USER_LVGL";

// LVGL Buffer
// Allocate buffer in internal RAM for better performance
// 320 * 240 * 2 / 10 = 15360 bytes. Internal RAM is fine.
#define LVGL_BUF_SIZE (LCD_WIDTH * LCD_HEIGHT / 10) 

static lv_display_t * disp_handle = NULL;
static lv_indev_t * indev_handle = NULL;
static gt911_dev_t tp_dev;

// Flush callback
static void lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    // The st7789 driver's draw_bitmap function calls esp_lcd_panel_draw_bitmap
    // esp_lcd_panel_draw_bitmap expects exclusive end coordinates (x2 + 1, y2 + 1)
    // We pass x2 + 1 and y2 + 1 because lcd_st7789_draw_bitmap passes them directly to esp_lcd_panel_draw_bitmap
    lcd_st7789_draw_bitmap(area->x1, area->y1, area->x2 + 1, area->y2 + 1, (uint16_t*)px_map);
}

// Flush ready callback (called from ISR)
static bool lvgl_flush_ready_callback(void *user_ctx) {
    lv_display_flush_ready(disp_handle);
    return false;
}

// Touch read callback
static void lvgl_touch_read_cb(lv_indev_t * indev, lv_indev_data_t * data) {
    gt911_read(&tp_dev);
    if (tp_dev.is_touched && tp_dev.touches > 0) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = tp_dev.points[0].x;
        data->point.y = tp_dev.points[0].y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

// Tick timer callback
static void lvgl_tick_task(void *arg) {
    lv_tick_inc(1);
}

// LVGL Task Handler
static void lvgl_task_handler(void *arg) {
    ESP_LOGI(TAG, "LVGL Task Started");
    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void user_lvgl_init(void) {
    ESP_LOGI(TAG, "Initializing User LVGL");

    // 1. Initialize Hardware
    // LCD
    ESP_ERROR_CHECK(lcd_st7789_init());
    lcd_st7789_register_flush_ready_cb(lvgl_flush_ready_callback, NULL);
    
    // Touch
    ESP_ERROR_CHECK(gt911_init_default(&tp_dev));

    // 2. Initialize LVGL
    lv_init();

    // 3. Create Display
    disp_handle = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    
    // Allocate buffer
    size_t buf_size_bytes = LVGL_BUF_SIZE * sizeof(uint16_t); // RGB565
    void *buf1 = heap_caps_malloc(buf_size_bytes, MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (buf1 == NULL) {
        ESP_LOGE(TAG, "Failed to allocate LVGL buffer");
        return;
    }
    
    lv_display_set_buffers(disp_handle, buf1, NULL, buf_size_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp_handle, lvgl_flush_cb);
    
    // 4. Create Input Device
    indev_handle = lv_indev_create();
    lv_indev_set_type(indev_handle, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev_handle, lvgl_touch_read_cb);

    // 5. Create Tick Timer
    const esp_timer_create_args_t tick_timer_args = {
        .callback = &lvgl_tick_task,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 1000)); // 1ms

    // 6. Create Task
    xTaskCreate(lvgl_task_handler, "lvgl_task", 4096, NULL, 5, NULL);
    
    // 7. Turn on backlight
    lcd_st7789_set_backlight(true);
    
    ESP_LOGI(TAG, "User LVGL Initialized");
}
