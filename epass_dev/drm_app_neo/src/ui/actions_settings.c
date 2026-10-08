//  设置页面 专用
#include "ui.h"
#include "ui/actions_settings.h"
#include "utils/settings.h"
#include "utils/log.h"
#include "ui/theme.h"
#include "ui/actions_confirm.h"

extern objects_t objects;
extern settings_t g_settings;

// =========================================
// 自己添加的方法 START
// =========================================
static void style_dropdown_selected_item(lv_obj_t *dropdown) {
    lv_obj_t *list = lv_dropdown_get_list(dropdown);
    if(list == NULL) {
        return;
    }

    lv_color_t background = lv_color_hex(0xFFFFFF);
    lv_color_t foreground = lv_color_hex(0x000000);

    lv_obj_set_style_bg_color(list, background,
                              LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER,
                            LV_PART_SELECTED | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(list, foreground,
                                LV_PART_SELECTED | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(list, background,
                              LV_PART_SELECTED | LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(list, LV_OPA_COVER,
                            LV_PART_SELECTED | LV_STATE_PRESSED);
    lv_obj_set_style_text_color(list, foreground,
                                LV_PART_SELECTED | LV_STATE_PRESSED);
}

void ui_settings_apply_dropdown_theme() {
    style_dropdown_selected_item(objects.sleep_timeout_dropdown);
    style_dropdown_selected_item(objects.theme_color_dropdown);
    style_dropdown_selected_item(objects.swmode_dropdown);
    style_dropdown_selected_item(objects.swint_dropdown);
    style_dropdown_selected_item(objects.usbmode_dropdown);
}

void ui_settings_load_ctrl_word(){
    settings_lock(&g_settings);
    if(g_settings.ctrl_word.lowbat_trip){
        lv_obj_add_state(objects.lowbat_trip, LV_STATE_CHECKED);
    }
    else{
        lv_obj_remove_state(objects.lowbat_trip, LV_STATE_CHECKED);
    }
    if(g_settings.ctrl_word.no_intro_block){
        lv_obj_add_state(objects.no_intro_block, LV_STATE_CHECKED);
    }
    else{
        lv_obj_remove_state(objects.no_intro_block, LV_STATE_CHECKED);
    }
    if(g_settings.ctrl_word.no_overlay_block){
        lv_obj_add_state(objects.no_overlay_block, LV_STATE_CHECKED);
    }
    else{
        lv_obj_remove_state(objects.no_overlay_block, LV_STATE_CHECKED);
    }
    settings_unlock(&g_settings);
}


// =========================================
// EEZ 回调 START
// =========================================

void action_clear_cache(lv_event_t * e){
    log_debug("action_clear_cache");
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_remove_state(obj, LV_STATE_PRESSED);
    ui_confirm(UI_CONFIRM_TYPE_CLEAR_CACHE);
}

void action_settings_ctrl_changed(lv_event_t * e){
    log_debug("action_settings_ctrl_changed");
    settings_lock(&g_settings);
    g_settings.ctrl_word.lowbat_trip = lv_obj_has_state(objects.lowbat_trip, LV_STATE_CHECKED);
    g_settings.ctrl_word.no_intro_block = lv_obj_has_state(objects.no_intro_block, LV_STATE_CHECKED);
    g_settings.ctrl_word.no_overlay_block = lv_obj_has_state(objects.no_overlay_block, LV_STATE_CHECKED);
    settings_unlock(&g_settings);
    settings_update(&g_settings);
    return;
}

sw_mode_t get_var_sw_mode(){
    return g_settings.switch_mode;
}
void set_var_sw_mode(sw_mode_t value){
    settings_lock(&g_settings);
    g_settings.switch_mode = value;
    settings_unlock(&g_settings);
    settings_update(&g_settings);
    return;
}

sw_interval_t get_var_sw_interval(){
    return g_settings.switch_interval;
}
void set_var_sw_interval(sw_interval_t value){
    settings_lock(&g_settings);
    g_settings.switch_interval = value;
    settings_unlock(&g_settings);
    settings_update(&g_settings);
    return;
}

sleep_timeout_t get_var_sleep_timeout(){
    return g_settings.sleep_timeout;
}
void set_var_sleep_timeout(sleep_timeout_t value){
    settings_lock(&g_settings);
    g_settings.sleep_timeout = value;
    settings_unlock(&g_settings);
    settings_update(&g_settings);
}

theme_color_t get_var_theme_color(){
    return g_settings.theme_color;
}
void set_var_theme_color(theme_color_t value){
    settings_lock(&g_settings);
    g_settings.theme_color = value;
    settings_unlock(&g_settings);
    settings_update(&g_settings);
    ui_theme_apply(value);
}

int32_t get_var_brightness(){
    return g_settings.brightness;
}
void set_var_brightness(int32_t value){
    settings_lock(&g_settings);
    g_settings.brightness = value;
    settings_unlock(&g_settings);
    settings_update(&g_settings);
    return;
}

usb_mode_t get_var_usb_mode(){
    return g_settings.usb_mode;
}
void set_var_usb_mode(usb_mode_t value){
    settings_lock(&g_settings);
    g_settings.usb_mode = value;
    settings_unlock(&g_settings);
    settings_update(&g_settings);
    settings_set_usb_mode(value);
    return;
}
extern int g_running;
extern int g_exitcode;
void action_call_srgn_config(lv_event_t * e){
    log_debug("action_call_srgn_config");
    g_exitcode = EXITCODE_SRGN_CONFIG;
    g_running = 0;
}
