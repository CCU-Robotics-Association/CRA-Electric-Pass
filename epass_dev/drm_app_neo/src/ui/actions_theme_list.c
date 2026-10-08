// 主题列表专用 - 虚拟滚动实现

#include <src/core/lv_group.h>
#include <src/core/lv_obj_event.h>
#include <src/core/lv_obj_private.h>
#include <src/misc/lv_event.h>
#include <stdint.h>
#include <stdlib.h>

#include "ui.h"
#include "utils/log.h"
#include "theme/theme.h"
#include "ui/actions_theme_list.h"
#include "styles.h"
#include "ui/scr_transition.h"


ui_theme_list_t g_ui_theme_list;
extern objects_t objects;

// 防递归标志 - 防止虚拟滚动时焦点事件递归触发
static bool g_scroll_in_progress = false;

// 前向声明
static void update_slot_content(int slot_idx, int theme_idx);
static void theme_list_focus_cb(lv_event_t *e);
static void refocus_to_theme(int theme_idx);

// 根据主题索引重新设置焦点
static void refocus_to_theme(int theme_idx) {
    for (int i = 0; i < UI_THEME_LIST_VISIBLE_SLOTS; i++) {
        if (g_ui_theme_list.slots[i].theme_index == theme_idx) {
            lv_group_focus_obj(g_ui_theme_list.slots[i].theme_btn);
            return;
        }
    }
}

static void theme_btn_click_cb(lv_event_t *e){
    lv_obj_t* obj = lv_event_get_target(e);
    lv_obj_remove_state(obj, LV_STATE_PRESSED);

    // 从 user_data 获取主题索引
    int theme_idx = (int)(intptr_t)lv_event_get_user_data(e);
    theme_t *theme = g_ui_theme_list.theme;

    theme_request_set_theme(theme, theme_idx);
    ui_schedule_screen_transition(curr_screen_t_SCREEN_SPINNER);
}

// 创建单个槽位的 UI 对象
static void create_slot_ui(int slot_idx) {
    ui_theme_list_entry_objs_t *slot = &g_ui_theme_list.slots[slot_idx];

    // 创建外层容器
    lv_obj_t *obj = lv_obj_create(objects.theme_list_container);
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, LV_PCT(97), UI_THEME_LIST_ITEM_HEIGHT);
    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    slot->container = obj;

    // 我们这里要用EEZ 生成的
    // void create_user_widget_theme_entry(lv_obj_t *parent_obj, int startWidgetIndex);
    // 来创建主题列表项。但是它startWidgetIndex是相对于eez的objects的。
    // 我们希望添加到自己的信息表里。因此，这里有一个非常，非常，非常，非常Dirty的hacks

    // create_user_widget_theme_entry的写入方法是：
    // ((lv_obj_t **)&objects)[startWidgetIndex + 0] = obj;
    // 也就是*((lv_obj_t **)&objects) + startWidgetIndex) = obj;
    // 令 startWidgetIndex = (lv_obj_t**)&slot->theme_btn - (lv_obj_t **)&objects;
    // 这样实际的操作就是  *((lv_obj_t**)&slot->theme_btn) = obj，即slot->theme_btn = obj

    #warning "Dirty hacks happened here. If application crash during theme->ui sync, Please check alignness and such."
    int startWidgetIndex = (lv_obj_t**)&slot->theme_btn - (lv_obj_t **)&objects;
    create_user_widget_theme_entry(obj, startWidgetIndex);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
    add_style_list_entry(obj);

    slot->theme_index = -1;  // 初始未绑定主题
}

// 更新槽位内容为指定主题
static void update_slot_content(int slot_idx, int theme_idx) {
    ui_theme_list_entry_objs_t *slot = &g_ui_theme_list.slots[slot_idx];
    theme_entry_t *entry = &g_ui_theme_list.theme->themes[theme_idx];

    // 更新内容
    lv_label_set_text(slot->theme_name_label, entry->theme_name);
    lv_label_set_text(slot->theme_description, entry->description);
    lv_image_set_src(slot->theme_icon, entry->icon_path);

    // SD标记可见性
    if(entry->source == THEME_SOURCE_NAND){
        lv_obj_add_flag(slot->sd_flag, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_remove_flag(slot->sd_flag, LV_OBJ_FLAG_HIDDEN);
    }

    // 移除旧的事件回调，添加新的
    lv_obj_remove_event_cb(slot->theme_btn, theme_btn_click_cb);
    lv_obj_add_event_cb(slot->theme_btn, theme_btn_click_cb, LV_EVENT_PRESSED, (void*)(intptr_t)theme_idx);

    slot->theme_index = theme_idx;
}

// 更新可见区域
static void update_visible_range(int new_start) {
    // 边界检查
    if (new_start < 0) new_start = 0;
    int max_start = g_ui_theme_list.total_count - UI_THEME_LIST_VISIBLE_SLOTS;
    if (max_start < 0) max_start = 0;
    if (new_start > max_start) new_start = max_start;

    int old_start = g_ui_theme_list.visible_start;
    if (new_start == old_start) return;

    g_ui_theme_list.visible_start = new_start;

    // 更新所有槽位
    for (int i = 0; i < UI_THEME_LIST_VISIBLE_SLOTS; i++) {
        int theme_idx = new_start + i;
        if (theme_idx < g_ui_theme_list.total_count) {
            update_slot_content(i, theme_idx);
            lv_obj_remove_flag(g_ui_theme_list.slots[i].container, LV_OBJ_FLAG_HIDDEN);
        } else {
            // 隐藏多余的槽位
            lv_obj_add_flag(g_ui_theme_list.slots[i].container, LV_OBJ_FLAG_HIDDEN);
            g_ui_theme_list.slots[i].theme_index = -1;
        }
    }

}

// 焦点变化回调 - encoder导航驱动的虚拟滚动
static void theme_list_focus_cb(lv_event_t *e) {
    // 防止递归调用（refocus_to_theme会触发新的FOCUSED事件）
    if (g_scroll_in_progress) return;

    lv_obj_t *focused = lv_event_get_target(e);

    // 找到当前焦点的slot索引
    int slot_idx = -1;
    for (int i = 0; i < UI_THEME_LIST_VISIBLE_SLOTS; i++) {
        if (g_ui_theme_list.slots[i].theme_btn == focused) {
            slot_idx = i;
            break;
        }
    }
    if (slot_idx < 0) return;

    int theme_idx = g_ui_theme_list.slots[slot_idx].theme_index;
    if (theme_idx < 0) return;

    // 边界检测：焦点移到顶部附近，向上滚动
    if (slot_idx <= 1 && g_ui_theme_list.visible_start > 0) {
        g_scroll_in_progress = true;
        int new_start = g_ui_theme_list.visible_start - 1;
        update_visible_range(new_start);
        refocus_to_theme(theme_idx);
        g_scroll_in_progress = false;
    }
    // 边界检测：焦点移到底部附近，向下滚动
    else if (slot_idx >= UI_THEME_LIST_VISIBLE_SLOTS - 2 &&
             g_ui_theme_list.visible_start + UI_THEME_LIST_VISIBLE_SLOTS < g_ui_theme_list.total_count) {
        g_scroll_in_progress = true;
        int new_start = g_ui_theme_list.visible_start + 1;
        update_visible_range(new_start);
        refocus_to_theme(theme_idx);
        g_scroll_in_progress = false;
    }
}

//自己添加的方法
void ui_theme_list_init(theme_t* theme){
    g_ui_theme_list.theme = theme;
    g_ui_theme_list.total_count = theme->theme_count;
    g_ui_theme_list.visible_start = 0;

    log_info("START theme->ui sync (virtual scroll mode)!! Total themes: %d", theme->theme_count);

    // 清空主题列表容器
    lv_obj_clean(objects.theme_list_container);

    for (int i = 0; i < UI_THEME_LIST_VISIBLE_SLOTS; i++) {
        create_slot_ui(i);
        if (i < theme->theme_count) {
            update_slot_content(i, i);
            lv_obj_remove_flag(g_ui_theme_list.slots[i].container, LV_OBJ_FLAG_HIDDEN);

            // 只为有效的槽位注册焦点回调
            lv_obj_add_event_cb(g_ui_theme_list.slots[i].theme_btn,
                                theme_list_focus_cb, LV_EVENT_FOCUSED, NULL);
        } else {
            lv_obj_add_flag(g_ui_theme_list.slots[i].container, LV_OBJ_FLAG_HIDDEN);
        }
    }

    log_info("theme->ui sync complete! Created %d slots for %d themes",
        UI_THEME_LIST_VISIBLE_SLOTS, theme->theme_count);
}

void add_theme_list_btn_to_group(){
    lv_group_remove_all_objs(groups.navigation);

    // 禁用焦点组的 wrap 循环，防止长按时焦点在边界槽位间反复跳动
    // 参考: lvgl/src/core/lv_group.c:480 - focus_next_core 中的 wrap 逻辑
    lv_group_set_wrap(groups.navigation, false);

    // 只添加当前可见的按钮到组
    for (int i = 0; i < UI_THEME_LIST_VISIBLE_SLOTS; i++) {
        if (g_ui_theme_list.slots[i].theme_index >= 0) {
            lv_group_add_obj(groups.navigation, g_ui_theme_list.slots[i].theme_btn);
        }
    }
    lv_group_add_obj(groups.navigation, objects.mainmenu_btn);
    lv_group_add_obj(groups.navigation, objects.refresh_theme_list_btn);

}

void ui_theme_list_focus_current_theme(){
    int current_op = g_ui_theme_list.theme->theme_index;

    // 确保当前主题在可见范围内
    if (current_op < g_ui_theme_list.visible_start ||
        current_op >= g_ui_theme_list.visible_start + UI_THEME_LIST_VISIBLE_SLOTS) {
        // 滚动到当前主题
        update_visible_range(current_op);
    }

    // 找到对应的槽位并聚焦
    for (int i = 0; i < UI_THEME_LIST_VISIBLE_SLOTS; i++) {
        if (g_ui_theme_list.slots[i].theme_index == current_op) {
            lv_group_focus_obj(g_ui_theme_list.slots[i].theme_btn);
            return;
        }
    }
}

// EEZ 回调
void action_refresh_theme_list(lv_event_t *e) {
    log_debug("action_refresh_theme_list");
    ui_schedule_screen_transition(curr_screen_t_SCREEN_SPINNER);
    theme_request_reload_assets(g_ui_theme_list.theme);

}
