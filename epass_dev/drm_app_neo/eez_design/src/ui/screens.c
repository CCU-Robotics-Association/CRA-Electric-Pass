#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

groups_t groups;
static bool groups_created = false;

objects_t objects;
lv_obj_t *tick_value_change_obj;
uint32_t active_theme_index = 0;

static void event_handler_cb_mainmenu_mainmenu(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_SCREEN_LOAD_START) {
        // group: navigation
        lv_group_remove_all_objs(groups.navigation);
        lv_group_add_obj(groups.navigation, objects.theme_list_btn);
        lv_group_add_obj(groups.navigation, objects.dispimg_btn);
        lv_group_add_obj(groups.navigation, objects.apps_btn);
        lv_group_add_obj(groups.navigation, objects.file_btn);
        lv_group_add_obj(groups.navigation, objects.sett_btn);
        lv_group_add_obj(groups.navigation, objects.dev_btn);
        lv_group_add_obj(groups.navigation, objects.brightness_scroller);
        lv_group_add_obj(groups.navigation, objects.restart_app_btn);
        lv_group_add_obj(groups.navigation, objects.shutdown_btn);
    }
}

static void event_handler_cb_mainmenu_brightness_scroller(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_slider_get_value(ta);
            set_var_brightness(value);
        }
    }
}

static void event_handler_cb_theme_list_theme_list(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_SCREEN_LOAD_START) {
        // group: navigation
        lv_group_remove_all_objs(groups.navigation);
        lv_group_add_obj(groups.navigation, objects.refresh_theme_list_btn);
        lv_group_add_obj(groups.navigation, objects.mainmenu_btn);
    }
}

static void event_handler_cb_sysinfo_sysinfo(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_SCREEN_LOAD_START) {
        // group: navigation
        lv_group_remove_all_objs(groups.navigation);
        lv_group_add_obj(groups.navigation, objects.back_btn);
        lv_group_add_obj(groups.navigation, objects.obj8);
    }
}

static void event_handler_cb_sysinfo_obj5(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_arc_get_value(ta);
            set_var_nand_percent(value);
        }
    }
}

static void event_handler_cb_sysinfo_obj6(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_arc_get_value(ta);
            set_var_sd_percent(value);
        }
    }
}

static void event_handler_cb_spinner_spinner(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_SCREEN_LOAD_START) {
        // group: navigation
        lv_group_remove_all_objs(groups.navigation);
    }
}

static void event_handler_cb_displayimg_displayimg(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_SCREEN_LOAD_START) {
        // group: navigation
        lv_group_remove_all_objs(groups.navigation);
    }
}

static void event_handler_cb_filemanager_filemanager(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_SCREEN_LOAD_START) {
        // group: navigation
        lv_group_remove_all_objs(groups.navigation);
    }
}

static void event_handler_cb_settings_settings(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_SCREEN_LOAD_START) {
        // group: navigation
        lv_group_remove_all_objs(groups.navigation);
        lv_group_add_obj(groups.navigation, objects.dummybtn);
        lv_group_add_obj(groups.navigation, objects.sleep_timeout_dropdown);
        lv_group_add_obj(groups.navigation, objects.theme_color_dropdown);
        lv_group_add_obj(groups.navigation, objects.swmode_dropdown);
        lv_group_add_obj(groups.navigation, objects.swint_dropdown);
        lv_group_add_obj(groups.navigation, objects.usbmode_dropdown);
        lv_group_add_obj(groups.navigation, objects.obj14);
        lv_group_add_obj(groups.navigation, objects.back_btn_1);
    }
}

static void event_handler_cb_settings_sleep_timeout_dropdown(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_dropdown_get_selected(ta);
            set_var_sleep_timeout(value);
        }
    }
}

static void event_handler_cb_settings_theme_color_dropdown(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_dropdown_get_selected(ta);
            set_var_theme_color(value);
        }
    }
}

static void event_handler_cb_settings_swmode_dropdown(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_dropdown_get_selected(ta);
            set_var_sw_mode(value);
        }
    }
}

static void event_handler_cb_settings_swint_dropdown(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_dropdown_get_selected(ta);
            set_var_sw_interval(value);
        }
    }
}

static void event_handler_cb_settings_usbmode_dropdown(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_VALUE_CHANGED) {
        lv_obj_t *ta = lv_event_get_target(e);
        if (tick_value_change_obj != ta) {
            int32_t value = lv_dropdown_get_selected(ta);
            set_var_usb_mode(value);
        }
    }
}

static void event_handler_cb_warning_warning(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_SCREEN_LOAD_START) {
        // group: navigation
        lv_group_remove_all_objs(groups.navigation);
    }
}

static void event_handler_cb_confirm_confirm(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_SCREEN_LOAD_START) {
        // group: navigation
        lv_group_remove_all_objs(groups.navigation);
        lv_group_add_obj(groups.navigation, objects.back_btn_2);
        lv_group_add_obj(groups.navigation, objects.obj20);
    }
}

static void event_handler_cb_applist_applist(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    if (event == LV_EVENT_SCREEN_LOAD_START) {
        // group: navigation
        lv_group_remove_all_objs(groups.navigation);
        lv_group_add_obj(groups.navigation, objects.applist_back_btn);
    }
}

void create_screen_mainmenu() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.mainmenu = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 640);
    lv_obj_add_event_cb(obj, action_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, (void *)0);
    lv_obj_add_event_cb(obj, event_handler_cb_mainmenu_mainmenu, LV_EVENT_ALL, 0);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 14, 8);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_cra_logo);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 51, 11);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "主菜单");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 25, 285);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_fa_label(obj);
            lv_obj_set_style_transform_scale_x(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_transform_scale_y(obj, 100, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 157, 376);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            add_style_label_small(obj);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 23, 372);
            lv_obj_set_size(obj, 312, LV_SIZE_CONTENT);
            add_style_label_small(obj);
            lv_obj_set_style_text_line_space(obj, -1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "致以 卜开丽\n　　本电子通行证献以长春大学机器人协会成员\n　　纪念大家一同奋进前行的时光\n　　祝愿各位未来勇敢立破，不等炬火\nEdited by XCrane/2026~2027届协会刘潇阳");
        }
        {
            // theme_list_btn
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.theme_list_btn = obj;
            lv_obj_set_pos(obj, 25, 48);
            lv_obj_set_size(obj, 97, 112);
            lv_obj_add_event_cb(obj, action_show_theme_list, LV_EVENT_PRESSED, (void *)0);
            add_style_main_btn(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 2, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_fa_label(obj);
                    lv_obj_set_style_transform_scale_x(obj, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_transform_scale_y(obj, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 1, 37);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "主题");
                }
            }
        }
        {
            // dispimg_btn
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.dispimg_btn = obj;
            lv_obj_set_pos(obj, 130, 48);
            lv_obj_set_size(obj, 97, 112);
            lv_obj_add_event_cb(obj, action_show_dispimg, LV_EVENT_PRESSED, (void *)0);
            add_style_main_btn(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 2, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_fa_label(obj);
                    lv_obj_set_style_transform_scale_x(obj, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_transform_scale_y(obj, 240, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 1, 37);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Cat");
                }
            }
        }
        {
            // apps_btn
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.apps_btn = obj;
            lv_obj_set_pos(obj, 235, 48);
            lv_obj_set_size(obj, 97, 112);
            lv_obj_add_event_cb(obj, action_show_apps, LV_EVENT_PRESSED, (void *)0);
            add_style_main_btn(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 4, 1);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_fa_label(obj);
                    lv_obj_set_style_transform_scale_x(obj, 250, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_transform_scale_y(obj, 250, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 1, 37);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "应用");
                }
            }
        }
        {
            // file_btn
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.file_btn = obj;
            lv_obj_set_pos(obj, 25, 166);
            lv_obj_set_size(obj, 97, 112);
            lv_obj_add_event_cb(obj, action_show_files, LV_EVENT_PRESSED, (void *)0);
            add_style_main_btn(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 10, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_fa_label(obj);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 1, 37);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "文件");
                }
            }
        }
        {
            // sett_btn
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.sett_btn = obj;
            lv_obj_set_pos(obj, 130, 166);
            lv_obj_set_size(obj, 97, 112);
            lv_obj_add_event_cb(obj, action_show_settings, LV_EVENT_PRESSED, (void *)0);
            add_style_main_btn(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 4, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_fa_label(obj);
                    lv_obj_set_style_transform_scale_x(obj, 250, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_transform_scale_y(obj, 250, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 1, 37);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "设置");
                }
            }
        }
        {
            // dev_btn
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.dev_btn = obj;
            lv_obj_set_pos(obj, 235, 166);
            lv_obj_set_size(obj, 97, 112);
            lv_obj_add_event_cb(obj, action_show_sysinfo, LV_EVENT_PRESSED, (void *)0);
            add_style_main_btn(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 10, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_fa_label(obj);
                    lv_label_set_text(obj, "");
                }
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 1, 37);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "设备");
                }
            }
        }
        {
            // brightness_scroller
            lv_obj_t *obj = lv_slider_create(parent_obj);
            objects.brightness_scroller = obj;
            lv_obj_set_pos(obj, 59, 292);
            lv_obj_set_size(obj, 273, 13);
            lv_slider_set_range(obj, 1, 9);
            lv_obj_add_event_cb(obj, event_handler_cb_mainmenu_brightness_scroller, LV_EVENT_ALL, 0);
        }
        {
            // restart_app_btn
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.restart_app_btn = obj;
            lv_obj_set_pos(obj, 26, 316);
            lv_obj_set_size(obj, 148, 52);
            lv_obj_add_event_cb(obj, action_restart_app, LV_EVENT_PRESSED, (void *)0);
            add_style_main_small_btn(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "重启程序");
                }
            }
        }
        {
            // shutdown_btn
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.shutdown_btn = obj;
            lv_obj_set_pos(obj, 183, 317);
            lv_obj_set_size(obj, 149, 52);
            lv_obj_add_event_cb(obj, action_shutdown, LV_EVENT_PRESSED, (void *)0);
            add_style_main_small_btn(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "关机");
                }
            }
        }
    }
    
    tick_screen_mainmenu();
}

void tick_screen_mainmenu() {
    {
        const char *new_val = get_var_epass_version();
        const char *cur_val = lv_label_get_text(objects.obj0);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj0;
            lv_label_set_text(objects.obj0, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        int32_t new_val = get_var_brightness();
        int32_t cur_val = lv_slider_get_value(objects.brightness_scroller);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.brightness_scroller;
            lv_slider_set_value(objects.brightness_scroller, new_val, LV_ANIM_ON);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_theme_list() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.theme_list = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 640);
    lv_obj_add_event_cb(obj, action_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, (void *)1);
    lv_obj_add_event_cb(obj, event_handler_cb_theme_list_theme_list, LV_EVENT_ALL, 0);
    {
        lv_obj_t *parent_obj = obj;
        {
            // theme_list_container
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.theme_list_container = obj;
            lv_obj_set_pos(obj, 14, 40);
            lv_obj_set_size(obj, 332, 280);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_ON);
            lv_obj_set_scroll_dir(obj, LV_DIR_VER);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.obj1 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_PCT(97), 80);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    create_user_widget_theme_entry(obj, 22);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    add_style_list_entry(obj);
                }
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.obj2 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_PCT(97), 80);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    create_user_widget_theme_entry(obj, 28);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    add_style_list_entry(obj);
                }
            }
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 51, 11);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "主题列表");
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 14, 8);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_cra_logo);
        }
        {
            // refresh_theme_list_btn
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.refresh_theme_list_btn = obj;
            lv_obj_set_pos(obj, 17, 327);
            lv_obj_set_size(obj, 159, 51);
            lv_obj_add_event_cb(obj, action_refresh_theme_list, LV_EVENT_PRESSED, (void *)1);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_FOCUSED);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj3 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "刷新列表");
                }
            }
        }
        {
            // mainmenu_btn
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.mainmenu_btn = obj;
            lv_obj_set_pos(obj, 187, 327);
            lv_obj_set_size(obj, 157, 51);
            lv_obj_add_event_cb(obj, action_show_menu, LV_EVENT_PRESSED, (void *)1);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_FOCUSED);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj4 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "主菜单");
                }
            }
        }
    }
    
    tick_screen_theme_list();
}

void tick_screen_theme_list() {
    tick_user_widget_theme_entry(22);
    tick_user_widget_theme_entry(28);
}

void create_screen_sysinfo() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.sysinfo = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 640);
    lv_obj_add_event_cb(obj, action_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, (void *)2);
    lv_obj_add_event_cb(obj, event_handler_cb_sysinfo_sysinfo, LV_EVENT_ALL, 0);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 52, 16);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "设备信息");
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 12, 13);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_cra_logo);
        }
        {
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.obj5 = obj;
            lv_obj_set_pos(obj, 40, 50);
            lv_obj_set_size(obj, 125, 125);
            lv_obj_add_event_cb(obj, event_handler_cb_sysinfo_obj5, LV_EVENT_ALL, 0);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0xffffffff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_KNOB | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_arc_create(parent_obj);
            objects.obj6 = obj;
            lv_obj_set_pos(obj, 195, 50);
            lv_obj_set_size(obj, 127, 125);
            lv_obj_add_event_cb(obj, event_handler_cb_sysinfo_obj6, LV_EVENT_ALL, 0);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0xffffffff), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_KNOB | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 52, 162);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "内部存储");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 229, 160);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "SD卡");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj9 = obj;
            lv_obj_set_pos(obj, 35, 191);
            lv_obj_set_size(obj, 134, 26);
            add_style_label_small(obj);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj10 = obj;
            lv_obj_set_pos(obj, 190, 190);
            lv_obj_set_size(obj, 136, 25);
            add_style_label_small(obj);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj11 = obj;
            lv_obj_set_pos(obj, 12, 210);
            lv_obj_set_size(obj, 335, 370);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_ON);
            lv_obj_set_scroll_dir(obj, LV_DIR_VER);
            add_style_label_small(obj);
            lv_label_set_text(obj, "");
        }
        {
            // back_btn
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.back_btn = obj;
            lv_obj_set_pos(obj, 27, 581);
            lv_obj_set_size(obj, 149, 51);
            lv_obj_add_event_cb(obj, action_show_menu, LV_EVENT_PRESSED, (void *)2);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_FOCUSED);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj7 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "返回");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.obj8 = obj;
            lv_obj_set_pos(obj, 186, 580);
            lv_obj_set_size(obj, 147, 52);
            lv_obj_add_event_cb(obj, action_format_sd_card, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff3f1a1e), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "格式化SD卡");
                }
            }
        }
    }
    
    tick_screen_sysinfo();
}

void tick_screen_sysinfo() {
    {
        int32_t new_val = get_var_nand_percent();
        int32_t cur_val = lv_arc_get_value(objects.obj5);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj5;
            lv_arc_set_value(objects.obj5, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        int32_t new_val = get_var_sd_percent();
        int32_t cur_val = lv_arc_get_value(objects.obj6);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.obj6;
            lv_arc_set_value(objects.obj6, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_nand_label();
        const char *cur_val = lv_label_get_text(objects.obj9);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj9;
            lv_label_set_text(objects.obj9, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_sd_label();
        const char *cur_val = lv_label_get_text(objects.obj10);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj10;
            lv_label_set_text(objects.obj10, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_sysinfo();
        const char *cur_val = lv_label_get_text(objects.obj11);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj11;
            lv_label_set_text(objects.obj11, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_spinner() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.spinner = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 640);
    lv_obj_add_event_cb(obj, action_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, (void *)3);
    lv_obj_add_event_cb(obj, event_handler_cb_spinner_spinner, LV_EVENT_ALL, 0);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 212, 23);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_small(obj);
            lv_label_set_text(obj, "Loading...");
        }
        {
            lv_obj_t *obj = lv_spinner_create(parent_obj);
            objects.obj12 = obj;
            lv_obj_set_pos(obj, 159, 9);
            lv_obj_set_size(obj, 43, 44);
            lv_obj_set_style_arc_width(obj, 6, LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_color(obj, lv_color_hex(0xffcdd0d3), LV_PART_INDICATOR | LV_STATE_DEFAULT);
            lv_obj_set_style_arc_width(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj13 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(50), LV_PCT(10));
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
            add_style_label_small(obj);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffa4a4a4), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sourcesans_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_line_space(obj, -3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "CRA Connection\n====> CCU Robotics\n[OK] System Ready\n[OK] UI Loaded");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 0, 620);
            lv_obj_set_size(obj, LV_PCT(100), 20);
            lv_label_set_long_mode(obj, LV_LABEL_LONG_CLIP);
            lv_label_set_text(obj, "ZmxhZ1NSR057VjNyeV9FRVpfU29mdFdhcmVfNF9VSV9ENHMxOW59DQrml6LnhLbooqvkvaDnnIvliLDkuobmiJHlsLHnoo7noo7lv7XkuKTlj6XvvIjvvIkNCuWFtuWunuaIkeacrOadpeWPquaYr+aDs+WBmui/meS4quW8hOedgOeOqeeahO+8jOayoeaDs+WIsOS8muiOt+W+l+i/meS5iOWkp+eahOWPjeWTje+8jOe+pOmDveWIsDIwMDDlpJrkurrkuoYuLi4uDQrmiJHov57nnYDlvIDkuobkuIDkuKrmnIjnmoTov4fovb3mqKHlvI/vvIzlp5HkuJTmiorov5nkuKoyLjDnmoRVSeW6lOeUqOWGmeWHuuadpeS6hu+8jOS9huaYr+aIkeaOpeS4i+adpeWunuWcqOayoeWKm+awlOS6hu+8jOi/mOW+l+aDs+WKnuazleaKiui/meS4quaciOiDoeWQg+a1t+WWneeahOiCpeWHj+Wbnuadpe+8iO+8iQ0K5oSf6LCi5L2g5p2l55yL55yL5Luj56CB4oCU4oCUDQrllYog5pyA5ZCO5piv5LiA5Liq5bCP56S854mp77yM5oiR6K6w5b6X5bel56iL6YeM6Z2i5piv5pyJc3lzdGVt56ym5Y+355qE77yM5LiN55+l6YGT5L2g5pyJ5rKh5pyJ5YW06Laj546p546p77yf/bin/sh");
        }
    }
    
    tick_screen_spinner();
}

void tick_screen_spinner() {
}

void create_screen_displayimg() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.displayimg = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 640);
    lv_obj_add_event_cb(obj, action_displayimg_key, LV_EVENT_KEY, (void *)0);
    lv_obj_add_event_cb(obj, action_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, (void *)4);
    lv_obj_add_event_cb(obj, event_handler_cb_displayimg_displayimg, LV_EVENT_ALL, 0);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 52, 16);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "Cat");
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 12, 13);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_cra_logo);
        }
        {
            // dispimg_container
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.dispimg_container = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 360, 640);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_displayimg();
}

void tick_screen_displayimg() {
}

void create_screen_filemanager() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.filemanager = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 640);
    lv_obj_add_event_cb(obj, action_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, (void *)5);
    lv_obj_add_event_cb(obj, event_handler_cb_filemanager_filemanager, LV_EVENT_ALL, 0);
    {
        lv_obj_t *parent_obj = obj;
        {
            // file_container
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.file_container = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 360, 640);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_filemanager();
}

void tick_screen_filemanager() {
}

void create_screen_settings() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.settings = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 640);
    lv_obj_add_event_cb(obj, action_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, (void *)6);
    lv_obj_add_event_cb(obj, event_handler_cb_settings_settings, LV_EVENT_ALL, 0);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 51, 11);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "设备参数");
        }
        {
            // dummybtn
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.dummybtn = obj;
            lv_obj_set_pos(obj, 257, 8);
            lv_obj_set_size(obj, 82, 32);
            lv_obj_add_event_cb(obj, action_clear_cache, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_FOCUSED);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_small(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "清除缓存");
                }
            }
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 14, 8);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_cra_logo);
        }
        {
            // lowbat_trip
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.lowbat_trip = obj;
            lv_obj_set_pos(obj, 279, 49);
            lv_obj_set_size(obj, 60, 29);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_event_cb(obj, action_settings_ctrl_changed, LV_EVENT_VALUE_CHANGED, (void *)0);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 22, 49);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            add_style_label_large(obj);
            lv_label_set_text(obj, "低电量自动关机");
        }
        {
            // no_intro_block
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.no_intro_block = obj;
            lv_obj_set_pos(obj, 279, 93);
            lv_obj_set_size(obj, 60, 29);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_event_cb(obj, action_settings_ctrl_changed, LV_EVENT_VALUE_CHANGED, (void *)0);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 22, 93);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            add_style_label_large(obj);
            lv_label_set_text(obj, "(切换时)跳过入场动画");
        }
        {
            // no_overlay_block
            lv_obj_t *obj = lv_switch_create(parent_obj);
            objects.no_overlay_block = obj;
            lv_obj_set_pos(obj, 279, 136);
            lv_obj_set_size(obj, 60, 29);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            lv_obj_add_event_cb(obj, action_settings_ctrl_changed, LV_EVENT_VALUE_CHANGED, (void *)0);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 22, 136);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_HIDDEN);
            add_style_label_large(obj);
            lv_label_set_text(obj, "(切换时)不显示信息层");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 22, 63);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "屏幕休眠");
        }
        {
            // sleep_timeout_dropdown
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.sleep_timeout_dropdown = obj;
            lv_obj_set_pos(obj, 195, 56);
            lv_obj_set_size(obj, 144, LV_SIZE_CONTENT);
            lv_dropdown_set_options(obj, "30秒\n1分钟\n3分钟\n从不");
            lv_obj_add_event_cb(obj, event_handler_cb_settings_sleep_timeout_dropdown, LV_EVENT_ALL, 0);
            lv_obj_set_style_text_font(obj, &ui_font_sourcesans_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sourcesans_reg_14, LV_PART_SELECTED | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 22, 112);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "主题配色");
        }
        {
            // theme_color_dropdown
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.theme_color_dropdown = obj;
            lv_obj_set_pos(obj, 195, 105);
            lv_obj_set_size(obj, 144, LV_SIZE_CONTENT);
            lv_dropdown_set_options(obj, "中性白\n靛紫\n青金石蓝\n勃艮第红\n克莱因蓝\n蒂芙尼蓝\n祖母绿\n中国红\n石墨黑\n天蓝\n琥珀黄\n柠檬黄\n香槟金");
            lv_obj_add_event_cb(obj, event_handler_cb_settings_theme_color_dropdown, LV_EVENT_ALL, 0);
            lv_obj_set_style_text_font(obj, &ui_font_sourcesans_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sourcesans_reg_14, LV_PART_SELECTED | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 22, 175);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "切换模式");
        }
        {
            // swmode_dropdown
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.swmode_dropdown = obj;
            lv_obj_set_pos(obj, 195, 168);
            lv_obj_set_size(obj, 144, LV_SIZE_CONTENT);
            lv_dropdown_set_options(obj, "顺序播放\n随机播放\n手动切换");
            lv_obj_add_event_cb(obj, event_handler_cb_settings_swmode_dropdown, LV_EVENT_ALL, 0);
            lv_obj_set_style_text_font(obj, &ui_font_sourcesans_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sourcesans_reg_14, LV_PART_SELECTED | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 22, 224);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "自动切换间隔");
        }
        {
            // swint_dropdown
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.swint_dropdown = obj;
            lv_obj_set_pos(obj, 195, 217);
            lv_obj_set_size(obj, 144, LV_SIZE_CONTENT);
            lv_dropdown_set_options(obj, "1分钟\n3分钟\n5分钟\n10分钟\n30分钟");
            lv_obj_add_event_cb(obj, event_handler_cb_settings_swint_dropdown, LV_EVENT_ALL, 0);
            lv_obj_set_style_text_font(obj, &ui_font_sourcesans_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sourcesans_reg_14, LV_PART_SELECTED | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 22, 273);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "USB模式");
        }
        {
            // usbmode_dropdown
            lv_obj_t *obj = lv_dropdown_create(parent_obj);
            objects.usbmode_dropdown = obj;
            lv_obj_set_pos(obj, 195, 266);
            lv_obj_set_size(obj, 144, LV_SIZE_CONTENT);
            lv_dropdown_set_options(obj, "文件(MTP)\nShell(串口)\n网络(rndis)\n仅充电\n管理器APP");
            lv_obj_add_event_cb(obj, event_handler_cb_settings_usbmode_dropdown, LV_EVENT_ALL, 0);
            lv_obj_set_style_text_font(obj, &ui_font_sourcesans_reg_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &ui_font_sourcesans_reg_14, LV_PART_SELECTED | LV_STATE_DEFAULT);
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.obj14 = obj;
            lv_obj_set_pos(obj, 23, 513);
            lv_obj_set_size(obj, 316, 52);
            lv_obj_add_event_cb(obj, action_call_srgn_config, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff3f1a1e), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff3f1a1e), LV_PART_MAIN | LV_STATE_FOCUSED);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "srgn_config入口");
                }
            }
        }
        {
            // back_btn_1
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.back_btn_1 = obj;
            lv_obj_set_pos(obj, 23, 574);
            lv_obj_set_size(obj, 316, 51);
            lv_obj_add_event_cb(obj, action_show_menu, LV_EVENT_PRESSED, (void *)6);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_FOCUSED);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj15 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "返回");
                }
            }
        }
    }
    
    tick_screen_settings();
}

void tick_screen_settings() {
    {
        if (!(lv_obj_get_state(objects.sleep_timeout_dropdown) & LV_STATE_EDITED)) {
            int32_t new_val = get_var_sleep_timeout();
            int32_t cur_val = lv_dropdown_get_selected(objects.sleep_timeout_dropdown);
            if (new_val != cur_val) {
                tick_value_change_obj = objects.sleep_timeout_dropdown;
                lv_dropdown_set_selected(objects.sleep_timeout_dropdown, new_val);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (!(lv_obj_get_state(objects.theme_color_dropdown) & LV_STATE_EDITED)) {
            int32_t new_val = get_var_theme_color();
            int32_t cur_val = lv_dropdown_get_selected(objects.theme_color_dropdown);
            if (new_val != cur_val) {
                tick_value_change_obj = objects.theme_color_dropdown;
                lv_dropdown_set_selected(objects.theme_color_dropdown, new_val);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (!(lv_obj_get_state(objects.swmode_dropdown) & LV_STATE_EDITED)) {
            int32_t new_val = get_var_sw_mode();
            int32_t cur_val = lv_dropdown_get_selected(objects.swmode_dropdown);
            if (new_val != cur_val) {
                tick_value_change_obj = objects.swmode_dropdown;
                lv_dropdown_set_selected(objects.swmode_dropdown, new_val);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (!(lv_obj_get_state(objects.swint_dropdown) & LV_STATE_EDITED)) {
            int32_t new_val = get_var_sw_interval();
            int32_t cur_val = lv_dropdown_get_selected(objects.swint_dropdown);
            if (new_val != cur_val) {
                tick_value_change_obj = objects.swint_dropdown;
                lv_dropdown_set_selected(objects.swint_dropdown, new_val);
                tick_value_change_obj = NULL;
            }
        }
    }
    {
        if (!(lv_obj_get_state(objects.usbmode_dropdown) & LV_STATE_EDITED)) {
            int32_t new_val = get_var_usb_mode();
            int32_t cur_val = lv_dropdown_get_selected(objects.usbmode_dropdown);
            if (new_val != cur_val) {
                tick_value_change_obj = objects.usbmode_dropdown;
                lv_dropdown_set_selected(objects.usbmode_dropdown, new_val);
                tick_value_change_obj = NULL;
            }
        }
    }
}

void create_screen_warning() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.warning = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 640);
    lv_obj_add_event_cb(obj, action_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, (void *)7);
    lv_obj_add_event_cb(obj, event_handler_cb_warning_warning, LV_EVENT_ALL, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffb93030), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj16 = obj;
            lv_obj_set_pos(obj, 9, 1);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_fa_label(obj);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj17 = obj;
            lv_obj_set_pos(obj, 76, 4);
            lv_obj_set_size(obj, 275, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj18 = obj;
            lv_obj_set_pos(obj, 76, 32);
            lv_obj_set_size(obj, 275, 34);
            add_style_label_small(obj);
            lv_label_set_text(obj, "");
        }
    }
    
    tick_screen_warning();
}

void tick_screen_warning() {
    {
        const char *new_val = get_var_warning_icon();
        const char *cur_val = lv_label_get_text(objects.obj16);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj16;
            lv_label_set_text(objects.obj16, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_warning_title();
        const char *cur_val = lv_label_get_text(objects.obj17);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj17;
            lv_label_set_text(objects.obj17, new_val);
            tick_value_change_obj = NULL;
        }
    }
    {
        const char *new_val = get_var_warning_desc();
        const char *cur_val = lv_label_get_text(objects.obj18);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj18;
            lv_label_set_text(objects.obj18, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_confirm() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.confirm = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 640);
    lv_obj_add_event_cb(obj, action_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, (void *)8);
    lv_obj_add_event_cb(obj, event_handler_cb_confirm_confirm, LV_EVENT_ALL, 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd6b65b), LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 14, 4);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_fa_label(obj);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj21 = obj;
            lv_obj_set_pos(obj, 83, 37);
            lv_obj_set_size(obj, 262, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 83, 4);
            lv_obj_set_size(obj, 196, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "=二次确认=");
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj19 = obj;
            lv_obj_set_pos(obj, 261, -2);
            lv_obj_set_size(obj, 99, 68);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffb9b1b1), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "=>Secound\nConfirm");
        }
        {
            // back_btn_2
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.back_btn_2 = obj;
            lv_obj_set_pos(obj, 28, 70);
            lv_obj_set_size(obj, 149, 51);
            lv_obj_add_event_cb(obj, action_confirm_cancel, LV_EVENT_PRESSED, (void *)2);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff6c6666), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "取消");
                }
            }
        }
        {
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.obj20 = obj;
            lv_obj_set_pos(obj, 187, 69);
            lv_obj_set_size(obj, 147, 52);
            lv_obj_add_event_cb(obj, action_confirm_proceed, LV_EVENT_PRESSED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffb10a0a), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "确定");
                }
            }
        }
    }
    
    tick_screen_confirm();
}

void tick_screen_confirm() {
    {
        const char *new_val = get_var_confirm_title();
        const char *cur_val = lv_label_get_text(objects.obj21);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj21;
            lv_label_set_text(objects.obj21, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_applist() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.applist = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 360, 640);
    lv_obj_add_event_cb(obj, action_screen_loaded_cb, LV_EVENT_SCREEN_LOADED, (void *)9);
    lv_obj_add_event_cb(obj, event_handler_cb_applist_applist, LV_EVENT_ALL, 0);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            lv_obj_set_pos(obj, 51, 11);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "应用列表");
        }
        {
            lv_obj_t *obj = lv_image_create(parent_obj);
            lv_obj_set_pos(obj, 14, 8);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_image_set_src(obj, &img_cra_logo);
        }
        {
            // app_container
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.app_container = obj;
            lv_obj_set_pos(obj, 0, 40);
            lv_obj_set_size(obj, 360, 520);
            lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_ON);
            lv_obj_set_scroll_dir(obj, LV_DIR_VER);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.obj22 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_PCT(97), 80);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    create_user_widget_app_entry(obj, 68);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    add_style_list_entry(obj);
                }
                {
                    lv_obj_t *obj = lv_obj_create(parent_obj);
                    objects.obj23 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_PCT(97), 80);
                    lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    create_user_widget_app_entry(obj, 75);
                    lv_obj_clear_flag(obj, LV_OBJ_FLAG_CLICKABLE);
                    add_style_list_entry(obj);
                }
            }
        }
        {
            // applist_back_btn
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.applist_back_btn = obj;
            lv_obj_set_pos(obj, 23, 574);
            lv_obj_set_size(obj, 316, 51);
            lv_obj_add_event_cb(obj, action_show_menu, LV_EVENT_PRESSED, (void *)9);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffffff), LV_PART_MAIN | LV_STATE_FOCUSED);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_FOCUSED);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.obj24 = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_label_large(obj);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xff000000), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "返回");
                }
            }
        }
        {
            // applist_no_app_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.applist_no_app_label = obj;
            lv_obj_set_pos(obj, 67, 203);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            add_style_label_large(obj);
            lv_label_set_text(obj, "设备内没有应用程序\n请将程序安装到\n/app");
        }
    }
    
    tick_screen_applist();
}

void tick_screen_applist() {
    tick_user_widget_app_entry(68);
    tick_user_widget_app_entry(75);
    {
        bool new_val = get_var_applist_show_warning();
        bool cur_val = lv_obj_has_flag(objects.applist_no_app_label, LV_OBJ_FLAG_HIDDEN);
        if (new_val != cur_val) {
            tick_value_change_obj = objects.applist_no_app_label;
            if (new_val) lv_obj_add_flag(objects.applist_no_app_label, LV_OBJ_FLAG_HIDDEN);
            else lv_obj_clear_flag(objects.applist_no_app_label, LV_OBJ_FLAG_HIDDEN);
            tick_value_change_obj = NULL;
        }
    }
}

void create_user_widget_theme_entry(lv_obj_t *parent_obj, int startWidgetIndex) {
    (void)startWidgetIndex;
    lv_obj_t *obj = parent_obj;
    {
        lv_obj_t *parent_obj = obj;
        {
            // theme_btn
            lv_obj_t *obj = lv_button_create(parent_obj);
            ((lv_obj_t **)&objects)[startWidgetIndex + 0] = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
            add_style_theme_entry_button(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // theme_icon
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    ((lv_obj_t **)&objects)[startWidgetIndex + 1] = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 64, 64);
                    lv_image_set_src(obj, &img_cra_logo);
                    lv_image_set_inner_align(obj, LV_IMAGE_ALIGN_STRETCH);
                }
                {
                    // theme_description
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    ((lv_obj_t **)&objects)[startWidgetIndex + 2] = obj;
                    lv_obj_set_pos(obj, 68, 32);
                    lv_obj_set_size(obj, 213, 32);
                    add_style_label_small(obj);
                    lv_obj_set_style_text_line_space(obj, -1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Null\n作者: XCrane");
                }
                {
                    // theme_name
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    ((lv_obj_t **)&objects)[startWidgetIndex + 3] = obj;
                    lv_obj_set_pos(obj, 68, 0);
                    lv_obj_set_size(obj, 232, 32);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
                    add_style_label_large(obj);
                    lv_label_set_text(obj, "测试用例");
                }
                {
                    // sd_flag_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    ((lv_obj_t **)&objects)[startWidgetIndex + 4] = obj;
                    lv_obj_set_pos(obj, 281, 47);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_sd_flag(obj);
                    lv_label_set_text(obj, "SD");
                }
            }
        }
    }
}

void tick_user_widget_theme_entry(int startWidgetIndex) {
    (void)startWidgetIndex;
}

void create_user_widget_app_entry(lv_obj_t *parent_obj, int startWidgetIndex) {
    (void)startWidgetIndex;
    lv_obj_t *obj = parent_obj;
    {
        lv_obj_t *parent_obj = obj;
        {
            // appbtn
            lv_obj_t *obj = lv_button_create(parent_obj);
            ((lv_obj_t **)&objects)[startWidgetIndex + 0] = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
            add_style_theme_entry_button(obj);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // applogo
                    lv_obj_t *obj = lv_image_create(parent_obj);
                    ((lv_obj_t **)&objects)[startWidgetIndex + 1] = obj;
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, 64, 64);
                    lv_image_set_src(obj, &img_cra_logo);
                    lv_image_set_inner_align(obj, LV_IMAGE_ALIGN_STRETCH);
                }
                {
                    // appdesc
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    ((lv_obj_t **)&objects)[startWidgetIndex + 2] = obj;
                    lv_obj_set_pos(obj, 68, 32);
                    lv_obj_set_size(obj, 245, 32);
                    add_style_label_small(obj);
                    lv_obj_set_style_text_line_space(obj, -1, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "Null\n作者: XCrane");
                }
                {
                    // appname
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    ((lv_obj_t **)&objects)[startWidgetIndex + 3] = obj;
                    lv_obj_set_pos(obj, 68, 0);
                    lv_obj_set_size(obj, 266, 32);
                    lv_label_set_long_mode(obj, LV_LABEL_LONG_SCROLL_CIRCULAR);
                    add_style_label_large(obj);
                    lv_label_set_text(obj, "测试用例");
                }
                {
                    // bgfg_flag
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    ((lv_obj_t **)&objects)[startWidgetIndex + 4] = obj;
                    lv_obj_set_pos(obj, 303, 47);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_app_bg_running(obj);
                    lv_label_set_text(obj, "后台");
                }
                {
                    // sd_flag
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    ((lv_obj_t **)&objects)[startWidgetIndex + 5] = obj;
                    lv_obj_set_pos(obj, 313, 30);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    add_style_sd_flag(obj);
                    lv_label_set_text(obj, "SD");
                }
            }
        }
    }
}

void tick_user_widget_app_entry(int startWidgetIndex) {
    (void)startWidgetIndex;
}


void ui_create_groups() {
    if (!groups_created) {
        groups.navigation = lv_group_create();
        groups_created = true;
    }
}


typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_mainmenu,
    tick_screen_theme_list,
    tick_screen_sysinfo,
    tick_screen_spinner,
    tick_screen_displayimg,
    tick_screen_filemanager,
    tick_screen_settings,
    tick_screen_warning,
    tick_screen_confirm,
    tick_screen_applist,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    ui_create_groups();
    
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), true, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_mainmenu();
    create_screen_theme_list();
    create_screen_sysinfo();
    create_screen_spinner();
    create_screen_displayimg();
    create_screen_filemanager();
    create_screen_settings();
    create_screen_warning();
    create_screen_confirm();
    create_screen_applist();
}
