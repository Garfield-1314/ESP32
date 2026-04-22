#include "driver/inc/user_lvgl.h"
#include "driver/inc/st7789.h"
#include "driver/inc/gt911.h"
#include "lvgl.h"
#include "esp_timer.h"

static const char *TAG = "lvgl";

// GT911触摸芯片实例
static gt911_dev_t gt911_dev;

// 触摸消抖相关变量
static uint32_t last_touch_time = 0;
static bool last_touch_state = false;
static lv_point_t last_touch_point = {0, 0};
#define TOUCH_DEBOUNCE_MS 50  // 触摸消抖时间（毫秒）
#define TOUCH_POSITION_THRESHOLD 10  // 位置变化阈值（像素）

// LVGL触摸输入回调（全局静态函数）
static void lvgl_touch_read_cb(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    uint32_t current_time = lv_tick_get();
    
    gt911_read(&gt911_dev);
    bool is_touched = gt911_is_touched(&gt911_dev);
    
    if (is_touched) {
        tp_point_t p = gt911_get_point(&gt911_dev, 0);
        lv_point_t current_point;
        current_point.x = p.y + (LCD_WIDTH - LCD_HEIGHT);
        current_point.y = LCD_HEIGHT - (p.x - (LCD_WIDTH - LCD_HEIGHT));
        
        // 计算位置变化
        // int32_t dx = current_point.x - last_touch_point.x;
        // int32_t dy = current_point.y - last_touch_point.y;
        // int32_t distance = (dx * dx + dy * dy);  // 距离平方
        
        // 触摸消抖逻辑
        if (!last_touch_state) {
            // 从未触摸到触摸状态
            if (current_time - last_touch_time > TOUCH_DEBOUNCE_MS) {
                data->point = current_point;
                data->state = LV_INDEV_STATE_PR;
                last_touch_state = true;
                last_touch_point = current_point;
                last_touch_time = current_time;
            } else {
                // 在消抖时间内，保持释放状态
                data->state = LV_INDEV_STATE_REL;
            }
        } else {
            // 持续触摸状态，更新坐标
            data->point = current_point;
            data->state = LV_INDEV_STATE_PR;
            last_touch_point = current_point;
            last_touch_time = current_time;
        }
    } else {
        // 触摸释放
        data->state = LV_INDEV_STATE_REL;
        if (last_touch_state) {
            last_touch_state = false;
            last_touch_time = current_time;
        }
    }
}


// 定义LVGL缓冲区 (32字节对齐以适配Cache/DMA)
static lv_color_t lvgl_buf1[LCD_WIDTH * (LCD_HEIGHT / 4)] __attribute__((aligned(32)));
static lv_color_t lvgl_buf2[LCD_WIDTH * (LCD_HEIGHT / 4)] __attribute__((aligned(32)));

// LCD传输完成回调
static bool lcd_flush_ready_callback(void *user_ctx)
{
    lv_disp_drv_t *disp_drv = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_drv);
    return false; // 不需要唤醒高优先级任务
}

// LVGL显示刷新回调函数
void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    // 检查LVGL传递的区域坐标是否有效
    if (area == NULL || color_p == NULL) {
        lv_disp_flush_ready(disp_drv);
        return;
    }
    
    // 检查区域坐标是否合法
    if (area->x2 < area->x1 || area->y2 < area->y1) {
        lv_disp_flush_ready(disp_drv);
        return;
    }
    
    // 计算区域宽高
    int32_t w = area->x2 - area->x1 + 1;
    int32_t h = area->y2 - area->y1 + 1;
    
    // 基本有效性检查
    if (w <= 0 || h <= 0) {
        lv_disp_flush_ready(disp_drv);
        return;
    }
    
    // 直接调用LCD驱动
    // 注意：我们移除了驱动层的静态缓冲区和字节交换，以支持DMA异步传输
    // 请确保在lv_conf.h中启用了 LV_COLOR_16_SWAP，否则颜色可能会反转
    lcd_st7789_draw_image(area->x1, area->y1, w, h, (const uint16_t *)color_p);
    
    // 注意：不要在这里调用 lv_disp_flush_ready(disp_drv);
    // 它会在DMA传输完成回调 lcd_flush_ready_callback 中被调用
}

void user_lvgl_init(void) {

    // 初始化LCD硬件
    lcd_st7789_init();

    // 初始化LVGL核心
    lv_init();
    
    // 初始化显示驱动
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    
    // 注册LCD传输完成回调
    lcd_st7789_register_flush_ready_cb(lcd_flush_ready_callback, &disp_drv);

    // 设置屏幕分辨率
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    
    // 设置刷新回调
    disp_drv.flush_cb = lvgl_flush_cb;
    
    // 设置显示缓冲区
    static lv_disp_draw_buf_t disp_buf;
    // 修复缓冲区大小参数，应与实际分配的大小一致
    lv_disp_draw_buf_init(&disp_buf, lvgl_buf1, lvgl_buf2, LCD_WIDTH * (LCD_HEIGHT / 4));
    disp_drv.draw_buf = &disp_buf;
    
    // 注册显示驱动
    lv_disp_drv_register(&disp_drv);

    // 初始化GT911触摸芯片
    gt911_init_default(&gt911_dev);
    gt911_set_resolution(&gt911_dev, LCD_WIDTH, LCD_HEIGHT);
    gt911_set_rotation(&gt911_dev, ROTATION_NORMAL);

    // 初始化LVGL输入设备驱动
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_read_cb;
    lv_indev_drv_register(&indev_drv);
}
