#pragma once
#include "config.h"
#include "stdint.h"
#include "utils/uuid.h"
#include "overlay/theme_info.h"
#include "overlay/transitions.h"
#include <overlay/overlay.h>
#include <stdio.h>
#include <utils/settings.h>
#include "utils/spsc_queue.h"
#include "vars.h"

typedef enum {
    DISPLAY_360_640 = 0,
    DISPLAY_480_854 = 1,
    DISPLAY_720_1280 = 2,
} display_type_t;

typedef struct {
    char path[128];

    // only valid in intro:
    bool enabled;
    int duration;
} theme_video_t;

typedef enum {
    THEME_SOURCE_NAND = 0,
    THEME_SOURCE_SD = 1,
} theme_source_t;

typedef struct {
    int index;
    char theme_name[40];
    uuid_t uuid;
    char description[256];
    char icon_path[128];
    display_type_t disp_type;
    theme_source_t source;
    
    theme_video_t intro_video;
    theme_video_t loop_video;

    theme_overlay_params_t overlay_params;
    oltr_params_t transition_in;
    oltr_params_t transition_loop;

} theme_entry_t;

typedef enum {
    THEME_REQUEST_NONE = 0,
    // 请求切换到指定主题
    THEME_REQUEST_SET_THEME,
    // 请求重新从磁盘加载主题
    THEME_REQUEST_RELOAD_ASSETS
} theme_request_type_t;


typedef struct {
    theme_request_type_t type;
    int theme_index;
    // 请求处理结束后 需不需要释放
    bool on_heap;
} theme_request_t;

typedef enum {
    THEME_STATE_IDLE = 0, // 正在播放loop动画。
    THEME_STATE_TRANSITION_IN, // 入场过渡
    THEME_STATE_INTRO, //入场视频
    THEME_STATE_TRANSITION_LOOP, // 循环过渡
    THEME_STATE_PRE_THEME_INFO, // 显示主题信息前的等待
} theme_state_t;

typedef struct {
    overlay_t * overlay;

    // theme 当前状态
    theme_state_t state;

    theme_entry_t themes[THEME_MAX];
    int theme_count;
    int theme_index;

    FILE* parse_log_f;

    // 上次发生主题切换的时机
    uint64_t last_switch_time;
    app_timer_handle_t timer_handle;

    spsc_bq_t req_queue;
    atomic_int is_auto_switch_blocked;

    bool use_sd;
} theme_t;

void theme_init(theme_t* theme,overlay_t* overlay,bool use_sd);
void theme_destroy(theme_t* theme);

void theme_request_set_theme(theme_t* theme,int theme_index);
void theme_request_reload_assets(theme_t* theme);
