#include "apps/ipc_handler.h"
#include "apps/ipc_common.h"
#include "apps/apps_types.h"
#include "vars.h"
#include <overlay/overlay.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ui/actions_warning.h>
#include <ui/ipc_helper.h>
#include <ui/scr_transition.h>
#include <utils/log.h>
#include <utils/settings.h>
#include <render/mediaplayer.h>
#include <overlay/transitions.h>
extern settings_t g_settings;
extern mediaplayer_t g_mediaplayer;
// =========================================
// UI 子模块 处理方法
// =========================================
inline static int handle_ui_warning(ipc_req_t *req, ipc_resp_t *resp){
    ui_warning_custom(
        req->ui_warning.title,
        req->ui_warning.desc,
        req->ui_warning.icon,
        req->ui_warning.color
    );
    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

inline static int handle_ui_get_current_screen(ipc_req_t *req, ipc_resp_t *resp){
    resp->ui_current_screen.screen = ui_get_current_screen();
    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

inline static int handle_ui_set_current_screen(ipc_req_t *req, ipc_resp_t *resp){
    ui_ipc_helper_req_t* helper_req = (ui_ipc_helper_req_t*)malloc(sizeof(ui_ipc_helper_req_t));
    if(helper_req == NULL){
        log_error("handle_ui_set_current_screen: malloc failed");
        resp->type = IPC_RESP_ERROR_NOMEM;
        return sizeof(ipc_resp_type_t);
    }
    helper_req->type = UI_IPC_HELPER_REQ_TYPE_SET_CURRENT_SCREEN;
    helper_req->target_screen = req->ui_set_current_screen.screen;
    helper_req->on_heap = true;
    ui_ipc_helper_request(helper_req);
    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

// =========================================
// 主题子模块处理方法
// =========================================
inline static int handle_theme_get_status(apps_t *apps, ipc_req_t *req, ipc_resp_t *resp){
    if(apps->theme == NULL){
        log_error("handle_theme_get_status: theme is NULL");
        resp->type = IPC_RESP_ERROR_INVALID_REQUEST;
        return sizeof(ipc_resp_type_t);
    }
    theme_t *theme = apps->theme;
    resp->theme_status.state = theme->state;
    resp->theme_status.theme_count = theme->theme_count;
    resp->theme_status.theme_index = theme->theme_index;
    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

inline static int handle_theme_set(apps_t *apps, ipc_req_t *req, ipc_resp_t *resp){
    if(apps->theme == NULL){
        log_error("handle_theme_set: theme is NULL");
        resp->type = IPC_RESP_ERROR_INVALID_REQUEST;
        return sizeof(ipc_resp_type_t);
    }
    theme_t *theme = apps->theme;
    int theme_index = req->theme_set.theme_index;

    // 验证索引有效性
    if(theme_index < 0 || theme_index >= theme->theme_count){
        log_error("handle_theme_set: invalid theme_index: %d (count: %d)",
                  theme_index, theme->theme_count);
        resp->type = IPC_RESP_ERROR_INVALID_REQUEST;
        return sizeof(ipc_resp_type_t);
    }

    theme_request_set_theme(theme, theme_index);
    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

inline static int handle_theme_get_info(apps_t *apps, ipc_req_t *req, ipc_resp_t *resp){
    if(apps->theme == NULL){
        log_error("handle_theme_get_info: theme is NULL");
        resp->type = IPC_RESP_ERROR_INVALID_REQUEST;
        return sizeof(ipc_resp_type_t);
    }
    theme_t *manager = apps->theme;
    int theme_index = req->theme_get.theme_index;

    // 验证索引有效性
    if(theme_index < 0 || theme_index >= manager->theme_count){
        log_error("handle_theme_get_info: invalid theme_index: %d (count: %d)",
                  theme_index, manager->theme_count);
        resp->type = IPC_RESP_ERROR_INVALID_REQUEST;
        return sizeof(ipc_resp_type_t);
    }

    theme_entry_t *entry = &manager->themes[theme_index];
    resp->theme_info.theme_index = entry->index;
    strncpy(resp->theme_info.theme_name, entry->theme_name,
            sizeof(resp->theme_info.theme_name));
    resp->theme_info.uuid = entry->uuid;
    strncpy(resp->theme_info.description, entry->description,
            sizeof(resp->theme_info.description));
    strncpy(resp->theme_info.icon_path, entry->icon_path,
            sizeof(resp->theme_info.icon_path));
    resp->theme_info.source = entry->source;
    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

inline static int handle_theme_set_blocked_auto_switch(apps_t *apps, ipc_req_t *req, ipc_resp_t *resp){
    if(apps->theme == NULL){
        log_error("handle_theme_set_blocked_auto_switch: theme is NULL");
        resp->type = IPC_RESP_ERROR_INVALID_REQUEST;
        return sizeof(ipc_resp_type_t);
    }
    theme_t *theme = apps->theme;
    bool is_blocked = req->theme_set_blocked_auto_switch.is_blocked;
    atomic_store(&theme->is_auto_switch_blocked, is_blocked ? 1 : 0);
    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

inline static int handle_theme_reload_assets(apps_t *apps, ipc_req_t *req, ipc_resp_t *resp){
    if(apps->theme == NULL){
        log_error("handle_theme_reload_assets: theme is NULL");
        resp->type = IPC_RESP_ERROR_INVALID_REQUEST;
        return sizeof(ipc_resp_type_t);
    }
    ui_ipc_helper_req_t* helper_req = (ui_ipc_helper_req_t*)malloc(sizeof(ui_ipc_helper_req_t));
    if(helper_req == NULL){
        log_error("handle_theme_reload_assets: malloc failed");
        resp->type = IPC_RESP_ERROR_NOMEM;
        return sizeof(ipc_resp_type_t);
    }
    helper_req->type = UI_IPC_HELPER_REQ_TYPE_SET_CURRENT_SCREEN;
    helper_req->target_screen = curr_screen_t_SCREEN_SPINNER;
    helper_req->on_heap = true;
    ui_ipc_helper_request(helper_req);

    theme_request_reload_assets(apps->theme);
    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

// =========================================
// Settings 子模块 处理方法
// =========================================
inline static int handle_settings_get(ipc_req_t *req, ipc_resp_t *resp){
    settings_lock(&g_settings);
    resp->settings.brightness = g_settings.brightness;
    resp->settings.switch_interval = g_settings.switch_interval;
    resp->settings.switch_mode = g_settings.switch_mode;
    resp->settings.usb_mode = g_settings.usb_mode;
    resp->settings.ctrl_word = g_settings.ctrl_word;
    settings_unlock(&g_settings);
    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

inline static int handle_settings_set(ipc_req_t *req, ipc_resp_t *resp){
    settings_lock(&g_settings);
    usb_mode_t curr_usb_mode = g_settings.usb_mode;
    g_settings.brightness = req->settings.brightness;
    g_settings.switch_interval = req->settings.switch_interval;
    g_settings.switch_mode = req->settings.switch_mode;
    g_settings.usb_mode = req->settings.usb_mode;
    g_settings.ctrl_word = req->settings.ctrl_word;
    settings_unlock(&g_settings);

    if(curr_usb_mode != req->settings.usb_mode){
        settings_set_usb_mode(req->settings.usb_mode);
    }

    // 保存设置并应用（设置亮度）
    settings_update(&g_settings);
    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

// =========================================
// MediaPlayer 子模块 处理方法
// =========================================
inline static int handle_mediaplayer_get_video_path(ipc_req_t *req, ipc_resp_t *resp){
    // 检查 mediaplayer 是否已初始化（通过检查 drm_warpper 指针）
    if(g_mediaplayer.drm_warpper == NULL){
        log_error("handle_mediaplayer_get_video_path: mediaplayer not initialized");
        resp->type = IPC_RESP_ERROR_INVALID_REQUEST;
        return sizeof(ipc_resp_type_t);
    }
    strncpy(resp->mediaplayer_video_path.path, g_mediaplayer.video_path,
            sizeof(resp->mediaplayer_video_path.path));
    resp->mediaplayer_video_path.path[sizeof(resp->mediaplayer_video_path.path) - 1] = '\0';
    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

inline static int handle_mediaplayer_set_video_path(ipc_req_t *req, ipc_resp_t *resp){
    // 检查 mediaplayer 是否已初始化（通过检查 drm_warpper 指针）
    if(g_mediaplayer.drm_warpper == NULL){
        log_error("handle_mediaplayer_set_video_path: mediaplayer not initialized");
        resp->type = IPC_RESP_ERROR_INVALID_REQUEST;
        return sizeof(ipc_resp_type_t);
    }
    // 这是"直接设置并播放"
    mediaplayer_stop(&g_mediaplayer);
    int ret = mediaplayer_play_video(&g_mediaplayer, req->mediaplayer_video_path.path);
    if(ret != 0){
        log_error("handle_mediaplayer_set_video_path: mediaplayer_play_video failed");
        resp->type = IPC_RESP_ERROR_INVALID_REQUEST;
        return sizeof(ipc_resp_type_t);
    }
    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

// =========================================
// Overlay 子模块 处理方法
// =========================================

// 用于存储视频路径的结构体（用于 middle_cb）
typedef struct {
    char video_path[128];
} overlay_video_userdata_t;

// 用于存储过渡参数的结构体（用于 end_cb 释放图片）
typedef struct {
    oltr_params_t* params;
    overlay_video_userdata_t* middle_userdata;  // 如果有视频切换，需要释放
} overlay_params_userdata_t;

// middle_cb: 在过渡中间切换视频
static void overlay_transition_middle_cb_video(void* userdata, bool is_last){
    overlay_video_userdata_t* data = (overlay_video_userdata_t*)userdata;
    log_trace("overlay_transition_middle_cb_video: switching video to %s", data->video_path);
    // 检查 mediaplayer 是否已初始化（通过检查 drm_warpper 指针）
    if(g_mediaplayer.drm_warpper == NULL){
        log_error("overlay_transition_middle_cb_video: mediaplayer not initialized");
        return;
    }
    mediaplayer_stop(&g_mediaplayer);
    mediaplayer_play_video(&g_mediaplayer, data->video_path);
}

// end_cb: 释放图片资源和其他资源
static void overlay_transition_end_cb_free_image(void* userdata, bool is_last){
    overlay_params_userdata_t* data = (overlay_params_userdata_t*)userdata;
    log_trace("overlay_transition_end_cb_free_image: freeing image and resources");
    overlay_transition_free_image(data->params);
    // 释放 params
    free(data->params);
    // 如果有 middle_userdata，也需要释放
    if(data->middle_userdata != NULL){
        free(data->middle_userdata);
    }
    // 释放 userdata 结构体
    free(data);
}

inline static int handle_overlay_schedule_transition(apps_t *apps, ipc_req_t *req, ipc_resp_t *resp){
    if(apps->theme == NULL || apps->theme->overlay == NULL){
        log_error("handle_overlay_schedule_transition: theme or overlay is NULL");
        resp->type = IPC_RESP_ERROR_INVALID_REQUEST;
        return sizeof(ipc_resp_type_t);
    }

    overlay_t* overlay = apps->theme->overlay;
    // 先终止overlay上的其他操作。
    overlay_abort(overlay);

    // 分配并填充 oltr_params_t
    oltr_params_t* params = (oltr_params_t*)malloc(sizeof(oltr_params_t));
    if(params == NULL){
        log_error("handle_overlay_schedule_transition: malloc failed for params");
        resp->type = IPC_RESP_ERROR_NOMEM;
        return sizeof(ipc_resp_type_t);
    }

    params->duration = req->overlay_schedule_transition.duration;
    params->type = req->overlay_schedule_transition.type;
    strncpy(params->image_path, req->overlay_schedule_transition.image_path, sizeof(params->image_path));
    params->image_path[sizeof(params->image_path) - 1] = '\0';
    params->background_color = req->overlay_schedule_transition.background_color;
    params->image_w = 0;
    params->image_h = 0;
    params->image_addr = NULL;

    // 加载图片。如果image_path为空 自动不加载。
    overlay_transition_load_image(params);

    // 分配 callback 结构体
    oltr_callback_t* callback = (oltr_callback_t*)malloc(sizeof(oltr_callback_t));
    if(callback == NULL){
        log_error("handle_overlay_schedule_transition: malloc failed for callback");
        overlay_transition_free_image(params);
        free(params);
        resp->type = IPC_RESP_ERROR_NOMEM;
        return sizeof(ipc_resp_type_t);
    }

    // 分配用于 end_cb 的 userdata
    overlay_params_userdata_t* end_userdata = (overlay_params_userdata_t*)malloc(sizeof(overlay_params_userdata_t));
    if(end_userdata == NULL){
        log_error("handle_overlay_schedule_transition: malloc failed for end_userdata");
        overlay_transition_free_image(params);
        free(params);
        free(callback);
        resp->type = IPC_RESP_ERROR_NOMEM;
        return sizeof(ipc_resp_type_t);
    }

    end_userdata->params = params;
    end_userdata->middle_userdata = NULL;  // 纯过渡，没有视频切换

    // 设置 callback
    callback->middle_cb = NULL;  // 纯过渡，不需要视频切换
    callback->middle_cb_userdata = NULL;
    callback->end_cb = overlay_transition_end_cb_free_image;
    callback->end_cb_userdata = end_userdata;
    callback->on_heap = true;
    callback->end_cb_userdata_on_heap = false;  // end_cb 自己处理释放，cleanup 不需要

    // 根据类型调用对应的过渡函数
    switch(params->type){
        case TRANSITION_TYPE_FADE:
            overlay_transition_fade(overlay, callback, params);
            break;
        case TRANSITION_TYPE_MOVE:
            overlay_transition_move(overlay, callback, params);
            break;
        case TRANSITION_TYPE_SWIPE:
            overlay_transition_swipe(overlay, callback, params);
            break;
        default:
            log_error("handle_overlay_schedule_transition: invalid transition type: %d", params->type);
            overlay_transition_free_image(params);
            free(params);
            free(callback);
            free(end_userdata);
            resp->type = IPC_RESP_ERROR_INVALID_REQUEST;
            return sizeof(ipc_resp_type_t);
    }

    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

inline static int handle_overlay_schedule_transition_video(apps_t *apps, ipc_req_t *req, ipc_resp_t *resp){
    if(apps->theme == NULL || apps->theme->overlay == NULL){
        log_error("handle_overlay_schedule_transition_video: theme or overlay is NULL");
        resp->type = IPC_RESP_ERROR_INVALID_REQUEST;
        return sizeof(ipc_resp_type_t);
    }

    overlay_t* overlay = apps->theme->overlay;

    // 先终止overlay上的其他操作。
    overlay_abort(overlay);

    // 分配并填充 oltr_params_t
    oltr_params_t* params = (oltr_params_t*)malloc(sizeof(oltr_params_t));
    if(params == NULL){
        log_error("handle_overlay_schedule_transition_video: malloc failed for params");
        resp->type = IPC_RESP_ERROR_NOMEM;
        return sizeof(ipc_resp_type_t);
    }

    params->duration = req->overlay_schedule_transition_video.duration;
    params->type = req->overlay_schedule_transition_video.type;
    strncpy(params->image_path, req->overlay_schedule_transition_video.image_path, sizeof(params->image_path));
    params->image_path[sizeof(params->image_path) - 1] = '\0';
    params->background_color = req->overlay_schedule_transition_video.background_color;
    params->image_w = 0;
    params->image_h = 0;
    params->image_addr = NULL;

    // 加载图片
    overlay_transition_load_image(params);

    // 分配 callback 结构体
    oltr_callback_t* callback = (oltr_callback_t*)malloc(sizeof(oltr_callback_t));
    if(callback == NULL){
        log_error("handle_overlay_schedule_transition_video: malloc failed for callback");
        overlay_transition_free_image(params);
        free(params);
        resp->type = IPC_RESP_ERROR_NOMEM;
        return sizeof(ipc_resp_type_t);
    }

    // 分配用于 middle_cb 的 userdata（视频路径）
    overlay_video_userdata_t* middle_userdata = (overlay_video_userdata_t*)malloc(sizeof(overlay_video_userdata_t));
    if(middle_userdata == NULL){
        log_error("handle_overlay_schedule_transition_video: malloc failed for middle_userdata");
        overlay_transition_free_image(params);
        free(params);
        free(callback);
        resp->type = IPC_RESP_ERROR_NOMEM;
        return sizeof(ipc_resp_type_t);
    }
    strncpy(middle_userdata->video_path, req->overlay_schedule_transition_video.video_path, sizeof(middle_userdata->video_path));
    middle_userdata->video_path[sizeof(middle_userdata->video_path) - 1] = '\0';

    // 分配用于 end_cb 的 userdata
    overlay_params_userdata_t* end_userdata = (overlay_params_userdata_t*)malloc(sizeof(overlay_params_userdata_t));
    if(end_userdata == NULL){
        log_error("handle_overlay_schedule_transition_video: malloc failed for end_userdata");
        overlay_transition_free_image(params);
        free(params);
        free(callback);
        free(middle_userdata);
        resp->type = IPC_RESP_ERROR_NOMEM;
        return sizeof(ipc_resp_type_t);
    }
    end_userdata->params = params;
    end_userdata->middle_userdata = middle_userdata;  // 保存 middle_userdata，在 end_cb 中释放

    // 设置 callback
    callback->middle_cb = overlay_transition_middle_cb_video;  // 在过渡中间切换视频
    callback->middle_cb_userdata = middle_userdata;
    callback->end_cb = overlay_transition_end_cb_free_image;  // 在过渡结束时释放图片
    callback->end_cb_userdata = end_userdata;
    callback->on_heap = true;
    callback->end_cb_userdata_on_heap = false;  // end_cb 自己处理释放，cleanup 不需要

    // 根据类型调用对应的过渡函数
    switch(params->type){
        case TRANSITION_TYPE_FADE:
            overlay_transition_fade(overlay, callback, params);
            break;
        case TRANSITION_TYPE_MOVE:
            overlay_transition_move(overlay, callback, params);
            break;
        case TRANSITION_TYPE_SWIPE:
            overlay_transition_swipe(overlay, callback, params);
            break;
        default:
            log_error("handle_overlay_schedule_transition_video: invalid transition type: %d", params->type);
            overlay_transition_free_image(params);
            free(params);
            free(callback);
            free(middle_userdata);
            free(end_userdata);
            resp->type = IPC_RESP_ERROR_INVALID_REQUEST;
            return sizeof(ipc_resp_type_t);
    }

    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

// =========================================
// 全局请求 处理方法
// =========================================
extern int g_exitcode;
extern int g_running;
inline static int handle_app_exit(ipc_req_t *req, ipc_resp_t *resp){
    g_exitcode = req->app_exit.exit_code;
    g_running = 0;
    resp->type = IPC_RESP_OK;
    return calculate_ipc_resp_size_by_req(req->type);
}

int apps_ipc_handler(apps_t *apps, uint8_t* rxbuf, size_t rxlen,uint8_t* txbuf, size_t txcap){
    ipc_req_t *req = (ipc_req_t *)rxbuf;
    ipc_resp_t *resp = (ipc_resp_t *)txbuf;
    size_t rx_expected_len = calculate_ipc_req_size(req->type);
    if (rxlen != rx_expected_len) {
        resp->type = IPC_RESP_ERROR_LENGTH_MISMATCH;
        return sizeof(ipc_resp_type_t);
    }

    switch(req->type){
        case IPC_REQ_UI_WARNING:
            return handle_ui_warning(req, resp);
        case IPC_REQ_UI_GET_CURRENT_SCREEN:
            return handle_ui_get_current_screen(req, resp);
        case IPC_REQ_UI_SET_CURRENT_SCREEN:
            return handle_ui_set_current_screen(req, resp);
        case IPC_REQ_THEME_GET_STATUS:
            return handle_theme_get_status(apps, req, resp);
        case IPC_REQ_THEME_SET:
            return handle_theme_set(apps, req, resp);
        case IPC_REQ_THEME_GET_INFO:
            return handle_theme_get_info(apps, req, resp);
        case IPC_REQ_THEME_SET_BLOCKED_AUTO_SWITCH:
            return handle_theme_set_blocked_auto_switch(apps, req, resp);
        case IPC_REQ_THEME_RELOAD_ASSETS:
            return handle_theme_reload_assets(apps, req, resp);
        case IPC_REQ_SETTINGS_GET:
            return handle_settings_get(req, resp);
        case IPC_REQ_SETTINGS_SET:
            return handle_settings_set(req, resp);
        case IPC_REQ_MEDIAPLAYER_GET_VIDEO_PATH:
            return handle_mediaplayer_get_video_path(req, resp);
        case IPC_REQ_MEDIAPLAYER_SET_VIDEO_PATH:
            return handle_mediaplayer_set_video_path(req, resp);
        case IPC_REQ_OVERLAY_SCHEDULE_TRANSITION:
            return handle_overlay_schedule_transition(apps, req, resp);
        case IPC_REQ_OVERLAY_SCHEDULE_TRANSITION_VIDEO:
            return handle_overlay_schedule_transition_video(apps, req, resp);
        case IPC_REQ_APP_EXIT:
            return handle_app_exit(req, resp);
        default:
            log_error("apps_ipc_handler: unknown request type: %d", req->type);
            resp->type = IPC_RESP_ERROR_UNKNOWN;
            return sizeof(ipc_resp_type_t);
    }
}
