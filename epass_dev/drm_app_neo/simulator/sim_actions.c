#include <stdio.h>
#include <stdint.h>

#include "actions.h"
#include "screens.h"
#include "ui.h"
#include "vars.h"
#include <lvgl/src/misc/cache/instance/lv_image_cache.h>

typedef enum {
    SIM_CONFIRM_GENERIC,
    SIM_CONFIRM_CLEAR_CACHE
} sim_confirm_action_t;

static sim_confirm_action_t g_sim_confirm_action = SIM_CONFIRM_GENERIC;

static void release_event_target(lv_event_t *event)
{
    if (event == NULL) {
        return;
    }

    lv_obj_t *target = lv_event_get_target_obj(event);
    if (target != NULL) {
        lv_obj_remove_state(target, LV_STATE_PRESSED);
    }
}

static void show_screen(lv_event_t *event, enum ScreensEnum screen_id)
{
    release_event_target(event);
    loadScreen(screen_id);
}

static void show_simulated_warning(lv_event_t *event, const char *title,
                                   const char *description)
{
    set_var_warning_title(title);
    set_var_warning_desc(description);
    show_screen(event, SCREEN_ID_WARNING);
}

static void style_dropdown_selected_item(lv_obj_t *dropdown)
{
    lv_obj_t *list = lv_dropdown_get_list(dropdown);
    if (list == NULL) {
        return;
    }

    const lv_color_t background = lv_color_hex(0xFFFFFF);
    const lv_color_t foreground = lv_color_hex(0x000000);

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

static void apply_settings_dropdown_theme(void)
{
    style_dropdown_selected_item(objects.sleep_timeout_dropdown);
    style_dropdown_selected_item(objects.theme_color_dropdown);
    style_dropdown_selected_item(objects.swmode_dropdown);
    style_dropdown_selected_item(objects.swint_dropdown);
    style_dropdown_selected_item(objects.usbmode_dropdown);
}

void action_show_theme_list(lv_event_t *event)
{
    show_screen(event, SCREEN_ID_THEME_LIST);
}

void action_show_menu(lv_event_t *event)
{
    show_screen(event, SCREEN_ID_MAINMENU);
}

void action_show_sysinfo(lv_event_t *event)
{
    show_screen(event, SCREEN_ID_SYSINFO);
}

void action_show_settings(lv_event_t *event)
{
    show_screen(event, SCREEN_ID_SETTINGS);
}

void action_show_files(lv_event_t *event)
{
    show_screen(event, SCREEN_ID_FILEMANAGER);
}

void action_show_apps(lv_event_t *event)
{
    show_screen(event, SCREEN_ID_APPLIST);
}

void action_show_dispimg(lv_event_t *event)
{
    show_screen(event, SCREEN_ID_DISPLAYIMG);
}

void action_shutdown(lv_event_t *event)
{
    g_sim_confirm_action = SIM_CONFIRM_GENERIC;
    set_var_confirm_title("模拟关机：确认继续？");
    show_screen(event, SCREEN_ID_CONFIRM);
}

void action_format_sd_card(lv_event_t *event)
{
    g_sim_confirm_action = SIM_CONFIRM_GENERIC;
    set_var_confirm_title("模拟格式化 SD 卡：确认继续？");
    show_screen(event, SCREEN_ID_CONFIRM);
}

void action_restart_app(lv_event_t *event)
{
    show_simulated_warning(event, "模拟重启",
                           "桌面模拟器未重启进程，界面与设备文件均未改变。");
}

void action_confirm_proceed(lv_event_t *event)
{
    if (g_sim_confirm_action == SIM_CONFIRM_CLEAR_CACHE) {
        lv_image_cache_drop(NULL);
        show_simulated_warning(
            event, "缓存已清理",
            "已释放模拟器图片缓存；未访问实体设备文件。");
        g_sim_confirm_action = SIM_CONFIRM_GENERIC;
        return;
    }

    show_simulated_warning(event, "模拟操作已完成",
                           "这是安全的桌面演示，没有操作实体设备。");
}

void action_confirm_cancel(lv_event_t *event)
{
    if (g_sim_confirm_action == SIM_CONFIRM_CLEAR_CACHE) {
        g_sim_confirm_action = SIM_CONFIRM_GENERIC;
        show_screen(event, SCREEN_ID_SETTINGS);
        return;
    }

    show_screen(event, SCREEN_ID_MAINMENU);
}

void action_call_srgn_config(lv_event_t *event)
{
    show_simulated_warning(event, "区域配置模拟",
                           "实体设备的区域配置程序不会在桌面端启动。");
}

void action_clear_cache(lv_event_t *event)
{
    release_event_target(event);
    g_sim_confirm_action = SIM_CONFIRM_CLEAR_CACHE;
    set_var_confirm_title("确定清除应用缓存吗？");
    show_screen(event, SCREEN_ID_CONFIRM);
}

void action_refresh_theme_list(lv_event_t *event)
{
    release_event_target(event);
    puts("[simulator] theme list refreshed");
}

void action_settings_ctrl_changed(lv_event_t *event)
{
    (void)event;
    printf("[simulator] settings: brightness=%ld, switch_mode=%d, "
           "interval=%d, sleep_timeout=%d, theme=%d, usb_mode=%d\n",
           (long)get_var_brightness(), (int)get_var_sw_mode(),
           (int)get_var_sw_interval(), (int)get_var_sleep_timeout(),
           (int)get_var_theme_color(), (int)get_var_usb_mode());
}

void action_screen_loaded_cb(lv_event_t *event)
{
    if ((intptr_t)lv_event_get_user_data(event) == SCREEN_ID_SETTINGS) {
        apply_settings_dropdown_theme();
    }
    puts("[simulator] screen loaded");
}

void action_displayimg_key(lv_event_t *event)
{
    (void)event;
}
