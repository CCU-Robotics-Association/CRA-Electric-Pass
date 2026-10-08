// 主题列表专用
#pragma once

#include "config.h"
#include "theme/theme.h"
#include "lvgl.h"

// 虚拟滚动：可见区域 + 上下缓冲
// 容器高度 280px，每项 80px，可见约 4 项，加上缓冲共 8 项

typedef struct {
    lv_obj_t *container;  // 外层容器对象
    // 请注意：以下顺序和EEZ保持一致！
    lv_obj_t *theme_btn;
    lv_obj_t *theme_icon;
    lv_obj_t *theme_description;
    lv_obj_t *theme_name_label;
    lv_obj_t *sd_flag;
    int theme_index;   // 该槽位当前显示的主题索引，-1 表示未使用
} ui_theme_list_entry_objs_t;

typedef struct {
    theme_t* theme;
    ui_theme_list_entry_objs_t slots[UI_THEME_LIST_VISIBLE_SLOTS];  // 固定槽位
    int total_count;        // 主题总数
    int visible_start;      // 当前可见区域起始索引
} ui_theme_list_t;


// UI层就先全局变量漫天飞吧....
extern ui_theme_list_t g_ui_theme_list;

// 自己添加的方法
void ui_theme_list_init(theme_t* theme);
void add_theme_list_btn_to_group();
void ui_theme_list_focus_current_theme();
// EEZ回调不需要添加。
