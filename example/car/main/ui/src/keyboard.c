#include "ui/inc/keyboard.h"

#include "ui/inc/ui.h"

// 键盘相关全局变量
lv_obj_t *keyboard = NULL;
lv_obj_t *current_textarea = NULL;
lv_obj_t *keyboard_container = NULL;

// 键盘事件回调函数
static void keyboard_event_cb(lv_event_t *e) {
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_READY || code == LV_EVENT_CANCEL) {
    hide_keyboard();
  }
}

// 文本区域点击事件回调函数
void textarea_click_cb(lv_event_t *e) {
  // 消抖处理
  static uint32_t last_textarea_click_time = 0;
  if (!check_debounce(&last_textarea_click_time, 200)) {  // 200ms消抖
    return;
  }

  lv_obj_t *textarea = lv_event_get_target(e);
  show_keyboard(textarea);
}

// 关闭键盘按钮事件回调函数
void close_keyboard_cb(lv_event_t *e) {
  // 消抖处理
  static uint32_t last_close_time = 0;
  if (!check_debounce(&last_close_time, 200)) {  // 200ms消抖
    return;
  }

  hide_keyboard();
}

// 在全局变量部分添加
static lv_obj_t *keyboard_preview_ta = NULL;  // 键盘上方的预览文本框
static lv_obj_t *mode_btn = NULL;             // 切换模式按钮

// 切换键盘模式回调
static void toggle_keyboard_mode_cb(lv_event_t *e) {
  if (keyboard == NULL) return;

  lv_keyboard_mode_t mode = lv_keyboard_get_mode(keyboard);
  lv_obj_t *btn = lv_event_get_target(e);
  lv_obj_t *label = lv_obj_get_child(btn, 0);

  if (mode == LV_KEYBOARD_MODE_NUMBER) {
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    if (label) lv_label_set_text(label, "123");  // 切换到数字
  } else {
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
    if (label) lv_label_set_text(label, "ABC");  // 切换到字母
  }
}

// 修改ensure_keyboard_created函数
static void ensure_keyboard_created(void) {
  if (keyboard_container != NULL) return;

  // 创建键盘容器（覆盖整个屏幕）
  keyboard_container = lv_obj_create(lv_layer_top());  // 使用顶层图层
  lv_obj_set_size(keyboard_container, LV_PCT(100), LV_PCT(100));
  lv_obj_set_style_bg_color(keyboard_container, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(keyboard_container, LV_OPA_50, 0);
  lv_obj_set_style_border_width(keyboard_container, 0, 0);
  lv_obj_set_style_radius(keyboard_container, 0, 0);
  lv_obj_add_flag(keyboard_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_clear_flag(keyboard_container, LV_OBJ_FLAG_SCROLLABLE);

  // 创建预览文本框（在键盘上方）
  keyboard_preview_ta = lv_textarea_create(keyboard_container);
  lv_obj_set_size(keyboard_preview_ta, LV_PCT(90), 50);
  lv_obj_align(keyboard_preview_ta, LV_ALIGN_TOP_MID, 0, 20);
  lv_obj_set_style_bg_color(keyboard_preview_ta, lv_color_white(), 0);
  lv_obj_set_style_text_color(keyboard_preview_ta, lv_color_black(), 0);
  lv_obj_set_style_border_color(keyboard_preview_ta, lv_color_hex(0x007acc), 0);
  lv_obj_set_style_border_width(keyboard_preview_ta, 2, 0);
  lv_obj_set_style_radius(keyboard_preview_ta, 5, 0);
  lv_textarea_set_placeholder_text(keyboard_preview_ta, "Input preview...");
  // 不在这里设置单行模式，而是在show_keyboard中根据实际情况动态设置
  lv_textarea_set_align(keyboard_preview_ta, LV_TEXT_ALIGN_LEFT);

  // 创建键盘
  keyboard = lv_keyboard_create(keyboard_container);
  lv_obj_set_size(keyboard, LV_PCT(90), LV_PCT(50));
  lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_radius(keyboard, 10, 0);
  lv_obj_set_style_bg_color(keyboard, lv_color_hex(0x2a2a2a), 0);

  // 添加关闭按钮
  lv_obj_t *close_btn = lv_btn_create(keyboard_container);
  lv_obj_set_size(close_btn, 40, 40);
  lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_obj_t *close_label = lv_label_create(close_btn);
  lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
  lv_obj_center(close_label);
  lv_obj_add_event_cb(close_btn, close_keyboard_cb, LV_EVENT_CLICKED, NULL);

  // 添加切换模式按钮
  mode_btn = lv_btn_create(keyboard_container);
  lv_obj_set_size(mode_btn, 60, 40);
  lv_obj_align(mode_btn, LV_ALIGN_TOP_RIGHT, -50, 0);  // 放在关闭按钮左边
  lv_obj_t *mode_label = lv_label_create(mode_btn);
  lv_label_set_text(mode_label, "123");  // 默认显示切换到数字
  lv_obj_center(mode_label);
  lv_obj_add_event_cb(mode_btn, toggle_keyboard_mode_cb, LV_EVENT_CLICKED,
                      NULL);

  lv_obj_add_event_cb(keyboard, keyboard_event_cb, LV_EVENT_ALL, NULL);
}

// 修改show_keyboard函数
void show_keyboard(lv_obj_t *textarea) {
  ensure_keyboard_created();

  if (keyboard_container == NULL || keyboard == NULL) return;

  current_textarea = textarea;

  // 同步预览文本框的内容
  const char *text = lv_textarea_get_text(textarea);
  if (text != NULL) {
    lv_textarea_set_text(keyboard_preview_ta, text);
  } else {
    lv_textarea_set_text(keyboard_preview_ta, "");
  }

  // 同步原始文本框的单行/多行模式到预览文本框
  bool is_one_line = lv_textarea_get_one_line(textarea);
  lv_textarea_set_one_line(keyboard_preview_ta, is_one_line);

  // 保存当前光标位置
  uint32_t cursor_pos = lv_textarea_get_cursor_pos(textarea);

  // 设置预览文本框的光标位置与原始文本框一致
  lv_textarea_set_cursor_pos(keyboard_preview_ta, cursor_pos);

  // 将键盘绑定到预览文本框（而不是原始textarea）
  lv_keyboard_set_textarea(keyboard, keyboard_preview_ta);

  // 根据文本框允许输入的字符自动切换键盘模式
  const char *accepted_chars = lv_textarea_get_accepted_chars(textarea);
  if (accepted_chars != NULL) {
    // 简单的启发式判断：如果允许输入数字但不允许输入字母，则认为是数字键盘
    bool has_digit = (strchr(accepted_chars, '0') != NULL);
    bool has_alpha = false;
    for (const char *p = accepted_chars; *p; p++) {
      if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z')) {
        has_alpha = true;
        break;
      }
    }

    if (has_digit && !has_alpha) {
      lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_NUMBER);
    } else {
      lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
    }
  } else {
    lv_keyboard_set_mode(keyboard, LV_KEYBOARD_MODE_TEXT_LOWER);
  }

  // 更新切换按钮的标签
  if (mode_btn != NULL) {
    lv_obj_t *label = lv_obj_get_child(mode_btn, 0);
    if (label) {
      if (lv_keyboard_get_mode(keyboard) == LV_KEYBOARD_MODE_NUMBER) {
        lv_label_set_text(label, "ABC");
      } else {
        lv_label_set_text(label, "123");
      }
    }
  }

  // 添加键盘值改变事件回调，用于同步预览文本框
  lv_obj_add_event_cb(keyboard, keyboard_value_changed_cb,
                      LV_EVENT_VALUE_CHANGED, textarea);

  lv_obj_clear_flag(keyboard_container, LV_OBJ_FLAG_HIDDEN);
  lv_obj_move_foreground(keyboard_container);
}

// 修改hide_keyboard函数
void hide_keyboard(void) {
  if (keyboard_container != NULL) {
    // 移除键盘值改变事件回调
    lv_obj_remove_event_cb(keyboard, keyboard_value_changed_cb);

    lv_obj_add_flag(keyboard_container, LV_OBJ_FLAG_HIDDEN);
  }
  current_textarea = NULL;
}

// 添加键盘值改变事件回调函数
void keyboard_value_changed_cb(lv_event_t *e) {
  lv_obj_t *textarea = (lv_obj_t *)lv_event_get_user_data(e);

  if (textarea != NULL && keyboard_preview_ta != NULL) {
    // 从预览文本框获取当前光标位置和内容
    uint32_t cursor_pos = lv_textarea_get_cursor_pos(keyboard_preview_ta);
    const char *text = lv_textarea_get_text(keyboard_preview_ta);

    // 同步到原始文本框
    if (text != NULL) {
      lv_textarea_set_text(textarea, text);
      lv_event_send(textarea, LV_EVENT_VALUE_CHANGED, NULL);

      // 同步光标位置到原始文本框
      lv_textarea_set_cursor_pos(textarea, cursor_pos);
    }
  }
}

// 修改set_keyboard_textarea函数
void set_keyboard_textarea(lv_obj_t *textarea) {
  if (keyboard != NULL) {
    current_textarea = textarea;

    // 同步预览文本框
    if (keyboard_preview_ta != NULL) {
      const char *text = lv_textarea_get_text(textarea);
      if (text != NULL) {
        lv_textarea_set_text(keyboard_preview_ta, text);
      } else {
        lv_textarea_set_text(keyboard_preview_ta, "");
      }

      // 同步光标位置
      uint32_t cursor_pos = lv_textarea_get_cursor_pos(textarea);
      lv_textarea_set_cursor_pos(keyboard_preview_ta, cursor_pos);

      // 键盘绑定到预览文本框
      lv_keyboard_set_textarea(keyboard, keyboard_preview_ta);
    }

    // 更新事件回调
    lv_obj_remove_event_cb(keyboard, keyboard_value_changed_cb);
    lv_obj_add_event_cb(keyboard, keyboard_value_changed_cb,
                        LV_EVENT_VALUE_CHANGED, textarea);
  }
}