#include "theme/theme.h"
#include <dirent.h>
#include <overlay/theme_info.h>
#include <overlay/overlay.h>
#include <overlay/transitions.h>
#include <theme/loader.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <ui/actions_warning.h>
#include <utils/timer.h>
#include <fcntl.h>
#include "utils/log.h"
#include <unistd.h>
#include <stdlib.h>
#include "config.h"
#include "utils/settings.h"
#include "vars.h"
#include "render/mediaplayer.h"
#include "ui/scr_transition.h"
#include "utils/misc.h"
#include "ui/ipc_helper.h"

extern settings_t g_settings;


inline static bool should_switch_by_interval(theme_t* theme){
    uint64_t interval_us = 0;
    if(atomic_load(&theme->is_auto_switch_blocked) != 0){
        return false;
    }
    switch(g_settings.switch_interval){
        case sw_interval_t_SW_INTERVAL_1MIN:
            interval_us = 1 * 60 * 1000 * 1000;
            break;
        case sw_interval_t_SW_INTERVAL_3MIN:
            interval_us = 3 * 60 * 1000 * 1000;
            break;
        case sw_interval_t_SW_INTERVAL_5MIN:
            interval_us = 5 * 60 * 1000 * 1000;
            break;
        case sw_interval_t_SW_INTERVAL_10MIN:
            interval_us = 10 * 60 * 1000 * 1000;
            break;
        case sw_interval_t_SW_INTERVAL_30MIN:
            interval_us = 30 * 60 * 1000 * 1000;
            break;
        default:
            log_error("invalid switch interval: %d", g_settings.switch_interval);
            return false;
    }


    if(g_settings.switch_mode == sw_mode_t_SW_MODE_MANUAL){
        return false;
    }
    else{
        return get_now_us() - theme->last_switch_time > interval_us;
    }
}

inline static int get_switch_target_index(theme_t* theme){

    if(theme->theme_count == 1){
        return 0;
    }

    int target_index = -1;


    if(g_settings.switch_mode == sw_mode_t_SW_MODE_SEQUENCE){
        return (theme->theme_index + 1) % theme->theme_count;
    }
    else if(g_settings.switch_mode == sw_mode_t_SW_MODE_RANDOM){
        do{
            target_index = rand() % theme->theme_count;
        }while(target_index == theme->theme_index);
        return target_index;
    }
    else{
        log_error("invalid switch mode: %d", g_settings.switch_mode);
        return theme->theme_index;
    }
}


extern mediaplayer_t g_mediaplayer;

static void set_video_cb(void* userdata,bool is_last){
    log_trace("set_video_cb");
    theme_video_t* data = (theme_video_t*)userdata;
    mediaplayer_stop(&g_mediaplayer);
    mediaplayer_play_video(&g_mediaplayer, data->path);
}

extern void mount_video_layer_callback(void *userdata,bool is_last);
static void set_video_mount_layer_cb(void* userdata,bool is_last){
    log_trace("set_video_mount_layer_cb");
    theme_video_t* data = (theme_video_t*)userdata;
    mediaplayer_stop(&g_mediaplayer);
    mediaplayer_play_video(&g_mediaplayer, data->path);

    mount_video_layer_callback(userdata, is_last);
}

// 前向定义
static void schedule_video_and_transitions(theme_t* theme, theme_video_t* video, oltr_params_t* transition, bool is_first_transition, int target_theme_index);

typedef struct {
    theme_t* theme;
    theme_video_t* video;
    oltr_params_t* transition;
    bool is_first_transition;
    int target_theme_index;  // 在调度时捕获目标主题索引
    // 需不需要释放这个结构体
    bool on_heap;
} schedule_video_and_transitions_timer_data_t;

// 用于传递给 schedule_video_and_transitions_end_cb 的数据结构
typedef struct {
    theme_t* theme;
    int target_theme_index;  // 在调度时捕获目标主题索引
} schedule_video_and_transitions_end_cb_data_t;

// 用于传递给 schedule_theme_info_timer_cb 的数据结构
typedef struct {
    theme_t* theme;
    int target_theme_index;  // 在调度时捕获目标主题索引
    bool on_heap;
} schedule_theme_info_timer_data_t;

// intro视频播放完（达到intro的duration之后）触发的定时器回调。调度transition_loop -> loop video
static void schedule_video_and_transitions_timer_cb(void* userdata,bool is_last){
    schedule_video_and_transitions_timer_data_t* data = (schedule_video_and_transitions_timer_data_t*)userdata;
    data->theme->state = THEME_STATE_TRANSITION_LOOP;
    schedule_video_and_transitions(data->theme, data->video, data->transition, data->is_first_transition, data->target_theme_index);
    if(data->on_heap){
        free(data);
    }
}

static void schedule_theme_info_timer_cb(void* userdata,bool is_last){
    schedule_theme_info_timer_data_t* data = (schedule_theme_info_timer_data_t*)userdata;
    theme_t* theme = data->theme;
    theme_entry_t* target_theme = &theme->themes[data->target_theme_index];

    log_info("schedule_theme_info_timer_cb: showing theme_info for theme %d (%s)",
             data->target_theme_index, target_theme->theme_name);

    if(target_theme->overlay_params.type == THEME_INFO_TYPE_CRA_PASS){
        overlay_theme_info_show_cra_pass(theme->overlay, &target_theme->overlay_params);
    }
    else if(target_theme->overlay_params.type == THEME_INFO_TYPE_CRA){
        overlay_theme_info_show_cra(theme->overlay, &target_theme->overlay_params);
    }
    else if(target_theme->overlay_params.type == THEME_INFO_TYPE_IMAGE){
        overlay_theme_info_show_image(theme->overlay, &target_theme->overlay_params);
    }
    else{
        log_error("schedule_theme_info_timer_cb: invalid theme_info type: %d", target_theme->overlay_params.type);
    }
    theme->state = THEME_STATE_IDLE;

    if(data->on_heap){
        free(data);
    }
}

static void schedule_theme_info(theme_t* theme,theme_entry_t* target_theme){
    if(target_theme->overlay_params.type != THEME_INFO_TYPE_NONE){
        schedule_theme_info_timer_data_t* data = malloc(sizeof(schedule_theme_info_timer_data_t));
        if(data == NULL){
            log_error("schedule_theme_info: malloc failed");
            return;
        }
        data->theme = theme;
        data->target_theme_index = target_theme->index;
        data->on_heap = true;

        log_info("schedule_theme_info: scheduling for theme %d (%s)",
                 target_theme->index, target_theme->theme_name);

        app_timer_handle_t timer_handle;
        app_timer_create(&timer_handle,
            target_theme->overlay_params.appear_time,
            0,
            1,
            schedule_theme_info_timer_cb,
            (void*)data);
    }
}

// 让我们捋一下主题切换的时间线。
// ===上一态loop video== | =transition_in== | =intro video== | =transition_loop== | =====THEME_INFO +loop video ==
//               主题切换|   d*1|  d*2|  d*3|                |   d*1|   d*2|   d*3| appear_time|
//             middle_cb切视频-|            |       middle_cb切视频-|             |
//                            |     end_cb-|                       |      end_cb-|
//                            | <======Intro Video实际播放时长 ===> |
// 
// 所以，我们在end_cb 调用的时候 ,排期transition_loop的时间应该是 
// intro_video.duration - transition_in.duration * 2 - transtion_loop.duration

static void schedule_video_and_transitions_end_cb(void* userdata,bool is_last){
    log_trace("schedule_video_and_transitions_end_cb");
    schedule_video_and_transitions_end_cb_data_t* cb_data = (schedule_video_and_transitions_end_cb_data_t*)userdata;
    theme_t* theme = cb_data->theme;
    int target_theme_index = cb_data->target_theme_index;
    theme_entry_t* target_theme = &theme->themes[target_theme_index];

    log_info("schedule_video_and_transitions_end_cb: processing for theme %d (%s)",
             target_theme_index, target_theme->theme_name);

    theme_state_t curr_state = theme->state;
    theme_state_t next_state = THEME_STATE_IDLE;


    if(curr_state == THEME_STATE_TRANSITION_IN){
        // 入场过渡结束，进入intro视频。等intro视频结束后，排期transition_loop -> loop video
        schedule_video_and_transitions_timer_data_t *data = malloc(sizeof(schedule_video_and_transitions_timer_data_t));
        data->theme = theme;
        data->video = &target_theme->loop_video;
        data->transition = &target_theme->transition_loop;
        data->is_first_transition = false;
        data->target_theme_index = target_theme_index;  // 传递目标主题索引
        data->on_heap = true;
        int delay = target_theme->intro_video.duration - target_theme->transition_in.duration * 2 - target_theme->transition_loop.duration;
        if(delay < 0){
            log_error("schedule_video_and_transitions_end_cb: delay < 0, delay: %d", delay);
            delay = 100 * 1000;
        }
        app_timer_handle_t timer_handle;
        app_timer_create(&timer_handle, delay, 0, 1, schedule_video_and_transitions_timer_cb, (void*)data);
        next_state = THEME_STATE_INTRO;
    }
    else if(theme->state == THEME_STATE_TRANSITION_LOOP){
        // 排期theme_info
        if(target_theme->overlay_params.type != THEME_INFO_TYPE_NONE && g_settings.ctrl_word.no_overlay_block == 0){
            schedule_theme_info(theme, target_theme);
            next_state = THEME_STATE_PRE_THEME_INFO;
        }
        else{
            next_state = THEME_STATE_IDLE;
        }
    }
    else{
        log_error("schedule_video_and_transitions_end_cb: invalid state: %d", curr_state);
    }

    theme->state = next_state;
    // 注意：不在这里释放 cb_data
    // oltr_callback_cleanup 或 swipe_cleanup 会统一处理释放
    // 如果在这里释放会导致 double-free 崩溃
}

static oltr_params_t first_transition_params = {
    .type = TRANSITION_TYPE_MOVE,
    .duration = 500 * 1000,
    .image_path = "",
    .image_w = 0,
    .image_h = 0,
    .image_addr = NULL,
    .background_color = 0xFF000000u,
};

// 排期视频和过渡。
// 在第一次过渡时，需要挂载视频图层，并使用move过渡。
// 在非第一次过渡时，需要使用过渡类型对应的过渡效果。
// 如果transition的type为NONE，则直接调用回调函数来切换视频并推进状态机。
// 由于回调函数那边会检定现在的状态，因此：先推进状态机，再来调用这个函数。
static void schedule_video_and_transitions(theme_t* theme, theme_video_t* video, oltr_params_t* transition, bool is_first_transition, int target_theme_index){
    oltr_callback_t* callback = malloc(sizeof(oltr_callback_t));

    log_trace("schedule_video_and_transitions: video: %s, transition: %d, is_first_transition: %d, target_theme: %d",
              video->path, transition->type, is_first_transition, target_theme_index);

    // 创建 end_cb 的数据结构，包含目标主题索引
    schedule_video_and_transitions_end_cb_data_t* end_cb_data = malloc(sizeof(schedule_video_and_transitions_end_cb_data_t));
    if(end_cb_data == NULL){
        log_error("schedule_video_and_transitions: malloc failed for end_cb_data");
        free(callback);
        return;
    }
    end_cb_data->theme = theme;
    end_cb_data->target_theme_index = target_theme_index;

    callback->middle_cb_userdata = video;
    callback->end_cb = schedule_video_and_transitions_end_cb;
    callback->end_cb_userdata = end_cb_data;
    callback->on_heap = true;
    callback->end_cb_userdata_on_heap = true;  // 标记 end_cb_userdata 需要释放

    // 第一次发生过渡时 有两个问题:
    // 1. 需要挂载视频图层（用mount_video_layer_callback）
    // 2. 不能使用fade过渡，否则有bug（强制用move）
    // 3. swipe会需要手动填充像素，不太适合刚开机的情况。
    if(is_first_transition){
        callback->middle_cb = set_video_mount_layer_cb;
        overlay_transition_move(theme->overlay, callback, &first_transition_params);
    }
    else{
        callback->middle_cb = set_video_cb;
        switch(transition->type){
            case TRANSITION_TYPE_FADE:
                overlay_transition_fade(theme->overlay, callback, transition);
                break;
            case TRANSITION_TYPE_MOVE:
                overlay_transition_move(theme->overlay, callback, transition);
                break;
            case TRANSITION_TYPE_SWIPE:
                overlay_transition_swipe(theme->overlay, callback, transition);
                break;
            case TRANSITION_TYPE_NONE:
                // 没有过渡效果时，直接调用回调来切换视频并推进状态机
                if(callback->middle_cb){
                    callback->middle_cb(callback->middle_cb_userdata, true);
                }
                if(callback->end_cb){
                    callback->end_cb(callback->end_cb_userdata, true);
                }
                // 由于没有调用 overlay_transition_*，不会有 cleanup 定时器
                // 需要手动释放 end_cb_userdata
                if(callback->end_cb_userdata_on_heap && callback->end_cb_userdata){
                    free(callback->end_cb_userdata);
                }
                // 释放callback结构体
                if(callback->on_heap){
                    free(callback);
                }
                break;
            default:
                log_error("invalid transition type: %d", transition->type);
                // 即使类型无效，也要调用回调以避免状态机卡住
                if(callback->middle_cb){
                    callback->middle_cb(callback->middle_cb_userdata, true);
                }
                if(callback->end_cb){
                    callback->end_cb(callback->end_cb_userdata, true);
                }
                // 由于没有调用 overlay_transition_*，不会有 cleanup 定时器
                // 需要手动释放 end_cb_userdata
                if(callback->end_cb_userdata_on_heap && callback->end_cb_userdata){
                    free(callback->end_cb_userdata);
                }
                if(callback->on_heap){
                    free(callback);
                }
                break;
        }
    }
}


typedef struct {
    theme_t* theme;
    int target_index;
    theme_entry_t* target_theme;
    bool *is_first_switch;
    bool on_heap;
} switch_theme_second_stage_data_t;

static void switch_theme_second_stage(void* userdata,bool is_last){
    switch_theme_second_stage_data_t* data = (switch_theme_second_stage_data_t*)userdata;
    theme_t* theme = data->theme;
    int target_index = data->target_index;
    theme_entry_t* target_theme = data->target_theme;
    bool is_first_switch = *data->is_first_switch;


    // 第一步。 存在intro video，且闭锁入场动画软压板没投
    // 则做全量 transition_in -> intro video -> transition_loop -> loop video
    if(target_theme->intro_video.enabled && g_settings.ctrl_word.no_intro_block == 0){
        theme->state = THEME_STATE_TRANSITION_IN;
        schedule_video_and_transitions(theme,
            &target_theme->intro_video,
            &target_theme->transition_in,
            is_first_switch,
            target_index
        );
    }
    // 不存在 intro video，则做 transition_in -> loop video
    // 我格式没设计好，所以明明使用的是transition_in 却进入了LOOP状态
    // 这个LOOP状态用于在回调中推进状态机。
    else{
        theme->state = THEME_STATE_TRANSITION_LOOP;
        schedule_video_and_transitions(theme,
            &target_theme->loop_video,
            &target_theme->transition_in,
            is_first_switch,
            target_index
        );
    }

    theme->last_switch_time = get_now_us();
    theme->theme_index = target_index;
    *data->is_first_switch = false;

    if(data->on_heap){
        free(data);
    }

}

static void switch_theme(theme_t* theme,int target_index){
    static bool is_first_switch = true;

    theme_entry_t* target_theme = &theme->themes[target_index];
    theme_entry_t* curr_theme = &theme->themes[theme->theme_index];


    if(theme->state != THEME_STATE_IDLE){
        log_error("switch_theme: theme is not idle?? curr_state: %d", theme->state);
        return;
    }

    log_info("switching theme from %s to %s", curr_theme->theme_name, target_theme->theme_name);

    // 卸载当前主题。此时overlay播放消失动画，但是mediaplayer还在运行。
    if(!is_first_switch){
        overlay_abort(theme->overlay);
        overlay_theme_info_free_image(&curr_theme->overlay_params);
        overlay_transition_free_image(&curr_theme->transition_in);
        overlay_transition_free_image(&curr_theme->transition_loop);
    }

    switch_theme_second_stage_data_t *data = malloc(sizeof(switch_theme_second_stage_data_t));
    data->theme = theme;
    data->target_index = target_index;
    data->target_theme = target_theme;
    data->is_first_switch = &is_first_switch;
    data->on_heap = true;

    // 先排期 overlay_abort结束后的操作（第二阶段）
    // 一旦调用第二阶段的schedule代码 就会立刻把buffer覆盖。
    // 我们要等overlay先结束之后 再进入第二阶段。
    app_timer_handle_t timer_handle;
    app_timer_create(
        &timer_handle, 
        UI_LAYER_ANIMATION_DURATION, 
        0, 
        1, 
        switch_theme_second_stage, 
        (void*)data
    );
    
    // 加载新主题
    overlay_transition_load_image(&target_theme->transition_in);
    overlay_transition_load_image(&target_theme->transition_loop);
    overlay_theme_info_load_image(&target_theme->overlay_params);

}


static void theme_reload_assets(theme_t* theme,bool is_first_load) {

    uuid_t theme_uuid_before;

    atomic_store(&theme->is_auto_switch_blocked,1);

    if(!is_first_load){
        memcpy(&theme_uuid_before, &theme->themes[theme->theme_index].uuid, sizeof(uuid_t));
    }

    theme->parse_log_f = fopen(THEME_PARSE_LOG, "w");
    if(theme->parse_log_f == NULL){
        log_error("failed to open parse log file: %s", THEME_PARSE_LOG);
    }
    theme->theme_count = 0;

    int errcnt = theme_scan_assets(theme, THEME_DIR,THEME_SOURCE_NAND);

    if(theme->use_sd){
        log_info("==> THEME will scan SD assets directory: %s", THEME_DIR_SD);
        errcnt += theme_scan_assets(theme, THEME_DIR_SD,THEME_SOURCE_SD);
    }

    if(errcnt != 0){
        ui_warning(UI_WARNING_ASSET_ERROR);
        log_warn("failed to load assets, error count: %d", errcnt);
    }

    if(theme->theme_count == 0){
        log_warn("no assets loaded, using fallback");
        ui_warning(UI_WARNING_NO_ASSETS);
        theme_try_load(theme, &theme->themes[0], THEME_FALLBACK_DIR, THEME_SOURCE_NAND, 0);
        theme->theme_count = 1;
    }

#ifndef APP_RELEASE
    for(int i = 0; i < theme->theme_count; i++){
        log_debug("========================");
        log_debug("theme[%d]:", i);
        theme_log_entry(&theme->themes[i]);
    }
#endif // APP_RELEASE

    if(!is_first_load){
        int i;
        for(i = 0; i < theme->theme_count; i++){
            if(uuid_compare(&theme_uuid_before, &theme->themes[i].uuid)){
                theme->theme_index = i;
                break;
            }
        }

        if(i == theme->theme_count){
            log_warn("old theme not found, try to switch to first theme");
            theme->theme_index = 0;
            switch_theme(theme, 0);
        }

        // 通知UI刷新主题列表
        log_info("==> THEME will notify UI to refresh theme_list");
        ui_ipc_helper_req_t* req = malloc(sizeof(ui_ipc_helper_req_t));
        req->type = UI_IPC_HELPER_REQ_TYPE_REFRESH_THEME_LIST;
        req->on_heap = true;

        ui_ipc_helper_request(req);
    }

    atomic_store(&theme->is_auto_switch_blocked,0);
}

static void theme_tick_cb(void* userdata,bool is_last){
    theme_t* theme = (theme_t*)userdata;
    theme_request_t* req;


    // 如果 这一次tick中 需要处理多个主题切换，我们只处理最后一次
    // 以防止切换堆叠到一起的情况。
    int target_theme_index = -1;

    // 处理本次 tick 中收到的全部主题请求。
    while(spsc_bq_try_pop(&theme->req_queue, (void**)&req) == 0){
        switch(req->type){
            case THEME_REQUEST_SET_THEME:
                target_theme_index = req->theme_index;
                break;
            case THEME_REQUEST_RELOAD_ASSETS:
                // 都要切换素材了 先别处理当前请求了，
                if(theme->state != THEME_STATE_IDLE){
                    spsc_bq_push(&theme->req_queue, (void *)req);
                    return;
                }
                theme_reload_assets(theme, false);
                break;
            default:
                log_error("invalid request type: %d", req->type);
                break;
        }

        if(req->on_heap){
            free(req);
        }
    }

    settings_lock(&g_settings);
    bool interval_sw = should_switch_by_interval(theme);
    // 应当发生主题切换
    if(target_theme_index != -1 || interval_sw){
        // 如果THEME正在处理主题切换，则告警
        if(theme->state != THEME_STATE_IDLE){
            ui_warning(UI_WARNING_THEME_CONFLICT);
            log_warn("theme is busy, skip switch");
            settings_unlock(&g_settings);
            return;
        }
    }
    else{
        settings_unlock(&g_settings);
        return;
    }

    // 由 时间触发
    if(target_theme_index == -1){
        if(!ui_is_hidden()){
            log_warn("switch_theme: ui is not hidden, skip switch");
            settings_unlock(&g_settings);
            return;
        }
        target_theme_index = get_switch_target_index(theme);
    }

    switch_theme(theme, target_theme_index);

    settings_unlock(&g_settings);

    return;
}

// 由其他线程调用，请求切换主题
void theme_request_set_theme(theme_t* theme,int theme_index){
    theme_request_t* req = malloc(sizeof(theme_request_t));
    req->type = THEME_REQUEST_SET_THEME;
    req->theme_index = theme_index;
    req->on_heap = true;
    spsc_bq_push(&theme->req_queue, (void *)req);
}

// 由其他线程调用，请求重新从磁盘加载主题
void theme_request_reload_assets(theme_t* theme){
    theme_request_t* req = malloc(sizeof(theme_request_t));
    req->type = THEME_REQUEST_RELOAD_ASSETS;
    req->on_heap = true;
    spsc_bq_push(&theme->req_queue, (void *)req);
}

void theme_init(theme_t* theme, overlay_t* overlay, bool use_sd){
    log_info("==> THEME Initializing...");
    theme->overlay = overlay;
    theme->use_sd = use_sd;

    atomic_store(&theme->is_auto_switch_blocked, 0);

    spsc_bq_init(&theme->req_queue, 10);

    theme_reload_assets(theme, true);

    log_info("==> THEME will perform first switch...");
    // 进行第一次主题切换
    theme->state = THEME_STATE_IDLE;
    theme->theme_index = 0;
    switch_theme(theme, 0);
    
    app_timer_create(
        &theme->timer_handle, 
        0,
        THEME_TICK_PERIOD, 
        -1, 
        theme_tick_cb, 
        theme
    );

    log_info("==> THEME Initalized!");

}
void theme_destroy(theme_t* theme){
    if(theme->parse_log_f != NULL){
        fclose(theme->parse_log_f);
    }
    if(theme->timer_handle){
        app_timer_cancel(theme->timer_handle);
    }
}
