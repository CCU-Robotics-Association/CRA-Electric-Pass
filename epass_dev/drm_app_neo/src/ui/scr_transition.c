// UI 屏幕过渡 相关处理方法
#include "ui/scr_transition.h"
#include "config.h"
#include "render/layer_animation.h"
#include "render/lvgl_drm_warp.h"
#include "utils/timer.h"
#include "utils/log.h"
#include "vars.h"
#include "screens.h"
#include "ui.h"
#include "ui/actions_settings.h"
#include "ui/screen_sleep.h"
#include "ui/filemanager.h"
#include "ui/actions_theme_list.h"
#include "ui/actions_apps.h"
#include "ui/scr_transition.h"
#include "ui/cat_pet.h"
#include "ui/actions_confirm.h"

extern int g_running;
extern int g_exitcode;

static curr_screen_t g_cur_scr;

// =========================================
// 自己添加的方法 START
// =========================================

inline static int get_screen_target_y(curr_screen_t screen){
    switch(screen){
        case curr_screen_t_SCREEN_THEME_LIST:
            return UI_THEME_LIST_Y;
        case curr_screen_t_SCREEN_MAINMENU:
            return UI_MAINMENU_Y;
        case curr_screen_t_SCREEN_WARNING:
            return UI_WARNING_Y;
        case curr_screen_t_SCREEN_SPINNER:
            return UI_HEIGHT;
        case curr_screen_t_SCREEN_CONFIRM:
            return UI_CONFIRM_Y;
        default:
            return 0;
    }
}

inline static void load_screen(curr_screen_t screen){
    switch(screen){
        case curr_screen_t_SCREEN_THEME_LIST:
            loadScreen(SCREEN_ID_THEME_LIST);
            break;
        case curr_screen_t_SCREEN_MAINMENU:
            loadScreen(SCREEN_ID_MAINMENU);
            break;
        case curr_screen_t_SCREEN_WARNING:
            loadScreen(SCREEN_ID_WARNING);
            break;
        case curr_screen_t_SCREEN_SPINNER:
            loadScreen(SCREEN_ID_SPINNER);
            break;
        case curr_screen_t_SCREEN_DISPLAYIMG:
            loadScreen(SCREEN_ID_DISPLAYIMG);
            break;
        case curr_screen_t_SCREEN_SETTINGS:
            loadScreen(SCREEN_ID_SETTINGS);
            break;
        case curr_screen_t_SCREEN_SYSINFO:
            loadScreen(SCREEN_ID_SYSINFO);
            break;
        case curr_screen_t_SCREEN_FILEMANAGER:
            loadScreen(SCREEN_ID_FILEMANAGER);
            break;
        case curr_screen_t_SCREEN_CONFIRM:
            loadScreen(SCREEN_ID_CONFIRM);
            break;
        case curr_screen_t_SCREEN_APPLIST:
            loadScreen(SCREEN_ID_APPLIST);
            break;
    }
}


static void delayed_load_screen_cb(void* arg, bool is_last){
    curr_screen_t screen = (curr_screen_t)arg;
    load_screen(screen);
}

void ui_schedule_screen_transition(curr_screen_t to_screen){
    lv_display_t * disp = lv_display_get_default();
    lvgl_drm_warp_t* lvgl_drm_warp = (lvgl_drm_warp_t*)lv_display_get_driver_data(disp);
    
    int current_y = get_screen_target_y(g_cur_scr);
    int target_y = get_screen_target_y(to_screen);

    app_timer_handle_t timer_handle;

    // 从spinner 到 其他任何屏幕，(除告警页）都做二阶段动画
    if(g_cur_scr == curr_screen_t_SCREEN_SPINNER && to_screen != curr_screen_t_SCREEN_WARNING && to_screen != curr_screen_t_SCREEN_CONFIRM){
        app_timer_create(
            &timer_handle, 
            UI_LAYER_ANIMATION_INTRO_LOADSCREEN_DELAY, 
            0, 
            1, 
            delayed_load_screen_cb, 
            (void*)to_screen
        );
        layer_animation_ease_in_out_move(
            lvgl_drm_warp->layer_animation, 
            DRM_WARPPER_LAYER_UI, 
            0, SCREEN_HEIGHT, 
            0, UI_SPINNER_INTRO_Y, 
            UI_LAYER_ANIMATION_INTRO_SPINNER_TRANSITION_DURATION, 0);
    
        layer_animation_ease_in_out_move(
            lvgl_drm_warp->layer_animation, 
            DRM_WARPPER_LAYER_UI, 
            0, UI_SPINNER_INTRO_Y, 
            0, target_y, 
            UI_LAYER_ANIMATION_INTRO_DEST_TRANSITION_DURATION, 
            UI_LAYER_ANIMATION_INTRO_DEST_TRANSITION_DELAY);
    }
    else{
        // 其他情况，直接过渡。
        if(current_y != target_y){
            layer_animation_ease_in_out_move(
                lvgl_drm_warp->layer_animation, 
                DRM_WARPPER_LAYER_UI, 
                0, current_y, 
                0, target_y, 
                UI_LAYER_ANIMATION_DURATION, 0);
        }
        load_screen(to_screen);
    }

}


bool ui_is_hidden(){
    return g_cur_scr == curr_screen_t_SCREEN_SPINNER;
}
// =========================================
// EEZ 回调 START
// =========================================

// 屏幕加载回调。更新“现在所显示的屏幕”变量。
// 对我们手动创建的widget，还需要把它们加入到group中。
void action_screen_loaded_cb(lv_event_t * e){
    g_cur_scr = (curr_screen_t)(lv_event_get_user_data(e));
    log_debug("action_screen_loaded_cb: cur_scr = %d", g_cur_scr);

    if(g_cur_scr == curr_screen_t_SCREEN_SETTINGS){
        ui_settings_apply_dropdown_theme();
        ui_settings_load_ctrl_word();
    }
    else if(g_cur_scr == curr_screen_t_SCREEN_FILEMANAGER){
        add_filemanager_to_group();
    }
    else if(g_cur_scr == curr_screen_t_SCREEN_THEME_LIST){
        add_theme_list_btn_to_group();
        ui_theme_list_focus_current_theme();
    }
    else if(g_cur_scr == curr_screen_t_SCREEN_APPLIST){
        add_applist_btn_to_group();
    }
    return;
};


// 全局按钮回调。主要用于屏幕切换时放过渡。 
bool screen_key_event_cb(uint32_t key){
    log_debug("screen_key_event_cb: g_cur_scr = %d, key = %d", g_cur_scr, key);

    if(ui_screen_sleep_wake_on_key()) {
        return true;
    }

    if(key == LV_KEY_END){
        ui_confirm(UI_CONFIRM_TYPE_SHUTDOWN);
    }

    // Cat 页面：Esc 返回主菜单，其余按键交给电子宠物预览逻辑。
    if (g_cur_scr == curr_screen_t_SCREEN_DISPLAYIMG){
        if(key == LV_KEY_ESC){
            ui_schedule_screen_transition(curr_screen_t_SCREEN_MAINMENU);
        }
        else{
            cat_pet_key_event(key);
        }
        return false;
    }

    // 主界面 和 主题列表
    if (g_cur_scr == curr_screen_t_SCREEN_MAINMENU || g_cur_scr == curr_screen_t_SCREEN_THEME_LIST){
        if(key == LV_KEY_ESC){
            ui_schedule_screen_transition(curr_screen_t_SCREEN_SPINNER);
        }
        return false;
    }

    // 警告页面 按任何按钮都复归
    if (g_cur_scr == curr_screen_t_SCREEN_WARNING){
        ui_schedule_screen_transition(curr_screen_t_SCREEN_SPINNER);
        return false;
    }

    // 其他界面
    if (g_cur_scr != curr_screen_t_SCREEN_SPINNER){
        // 不在编辑状态的时候再按下esc 回到主界面
        bool is_editing = lv_group_get_editing(groups.navigation);
        if(key == LV_KEY_ESC){
            if(!is_editing)
                ui_schedule_screen_transition(curr_screen_t_SCREEN_MAINMENU);
            else
                lv_group_set_editing(groups.navigation, false);
        }
        return false;
    }

    // spinner（空界面），处理ui出现

    switch(key){
        // go to theme_list screen
        case LV_KEY_LEFT:
        case LV_KEY_RIGHT:
           ui_schedule_screen_transition(curr_screen_t_SCREEN_THEME_LIST);
            break;
        case LV_KEY_ENTER:
            ui_schedule_screen_transition(curr_screen_t_SCREEN_DISPLAYIMG);
            break;
        case LV_KEY_ESC:
            ui_schedule_screen_transition(curr_screen_t_SCREEN_MAINMENU);
            break;
    }
    return false;
}

curr_screen_t ui_get_current_screen(){
    return g_cur_scr;
}
