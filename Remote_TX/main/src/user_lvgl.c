#include "include/user_lvgl.h"
#include "include/st7789.h"
#include "include/gt911.h"
#include "lvgl.h"

// 定义LVGL缓冲区 (32字节对齐以适配Cache/DMA)
static lv_color_t lvgl_buf1[LCD_WIDTH * (LCD_HEIGHT / 4)] __attribute__((aligned(32)));
static lv_color_t lvgl_buf2[LCD_WIDTH * (LCD_HEIGHT / 4)] __attribute__((aligned(32)));

// LCD传输完成回调
static bool lcd_flush_ready_callback(void *user_ctx)
{
    lv_display_t *disp = (lv_display_t *)user_ctx;
    lv_display_flush_ready(disp);
    return false; // 不需要唤醒高优先级任务
}

// LVGL显示刷新回调函数
void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    // 检查LVGL传递的区域坐标是否有效
    if (area == NULL || px_map == NULL) {
        lv_display_flush_ready(disp);
        return;
    }
    
    // 检查区域坐标是否合法
    if (area->x2 < area->x1 || area->y2 < area->y1) {
        lv_display_flush_ready(disp);
        return;
    }
    
    // 计算区域宽高
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    
    // 基本有效性检查
    if (w <= 0 || h <= 0) {
        lv_display_flush_ready(disp);
        return;
    }
    
    // 直接调用LCD驱动
    // 注意：我们移除了驱动层的静态缓冲区和字节交换，以支持DMA异步传输
    // 请确保在lv_conf.h中启用了 LV_COLOR_16_SWAP，否则颜色可能会反转
    lcd_st7789_draw_image(area->x1, area->y1, w, h, (const uint16_t *)px_map);
    
    // 注意：不要在这里调用 lv_display_flush_ready(disp);
    // 它会在DMA传输完成回调 lcd_flush_ready_callback 中被调用
}


// GT911触摸芯片实例
static gt911_dev_t gt911_dev;

// 触摸消抖相关变量
static uint32_t last_touch_time = 0;
static bool last_touch_state = false;
static lv_point_t last_touch_point = {0, 0};
#define TOUCH_DEBOUNCE_MS 50  // 触摸消抖时间（毫秒）
#define TOUCH_POSITION_THRESHOLD 10  // 位置变化阈值（像素）


// LVGL触摸输入回调（全局静态函数）
static void lvgl_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data) {
    uint32_t current_time = lv_tick_get();
    
    gt911_read(&gt911_dev);
    bool is_touched = gt911_is_touched(&gt911_dev);
    
    if (is_touched) {
        tp_point_t p = gt911_get_point(&gt911_dev, 0);
        lv_point_t current_point;
        current_point.x = p.y + (LCD_WIDTH - LCD_HEIGHT);
        current_point.y = LCD_HEIGHT - (p.x - (LCD_WIDTH - LCD_HEIGHT));
        
      
        // 触摸消抖逻辑
        if (!last_touch_state) {
            // 从未触摸到触摸状态
            if (current_time - last_touch_time > TOUCH_DEBOUNCE_MS) {
                data->point = current_point;
                data->state = LV_INDEV_STATE_PRESSED;
                last_touch_state = true;
                last_touch_point = current_point;
                last_touch_time = current_time;
            } else {
                // 在消抖时间内，保持释放状态
                data->state = LV_INDEV_STATE_RELEASED;
            }
        } else {
            // 持续触摸状态，更新坐标
            data->point = current_point;
            data->state = LV_INDEV_STATE_PRESSED;
            last_touch_point = current_point;
            last_touch_time = current_time;
        }
    } else {
        // 触摸释放
        data->state = LV_INDEV_STATE_RELEASED;
        if (last_touch_state) {
            last_touch_state = false;
            last_touch_time = current_time;
        }
    }
}


void user_lvgl_init(void) {
 // 初始化LCD硬件
    lcd_st7789_init();

    // 初始化LVGL核心
    lv_init();
    
    // 创建显示对象
    lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    
    // 注册LCD传输完成回调
    lcd_st7789_register_flush_ready_cb(lcd_flush_ready_callback, disp);

    // 设置刷新回调
    lv_display_set_flush_cb(disp, lvgl_flush_cb);
    
    // 设置显示缓冲区
    lv_display_set_buffers(disp, lvgl_buf1, lvgl_buf2, sizeof(lvgl_buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 初始化GT911触摸芯片
    gt911_init_default(&gt911_dev);
    gt911_set_resolution(&gt911_dev, LCD_WIDTH, LCD_HEIGHT);
    gt911_set_rotation(&gt911_dev, ROTATION_NORMAL);

    // 初始化LVGL输入设备驱动
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, lvgl_touch_read_cb);
}
