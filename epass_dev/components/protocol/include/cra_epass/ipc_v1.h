/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#ifndef CRA_EPASS_IPC_V1_H
#define CRA_EPASS_IPC_V1_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CRA_EPASS_IPC_SOCKET_PATH "/tmp/epass_drm_app.sock"
#define CRA_EPASS_IPC_MAX_MESSAGE 512U

#define CRA_EPASS_WARNING_TITLE_SIZE 64U
#define CRA_EPASS_WARNING_DESC_SIZE 128U
#define CRA_EPASS_WARNING_ICON_SIZE 16U
#define CRA_EPASS_PATH_SIZE 128U
#define CRA_EPASS_THEME_NAME_SIZE 40U
#define CRA_EPASS_THEME_DESC_SIZE 256U
#define CRA_EPASS_UUID_SIZE 16U

typedef int32_t cra_ipc_request_type_t;
enum {
    CRA_IPC_REQ_UI_WARNING = 0,
    CRA_IPC_REQ_UI_GET_CURRENT_SCREEN = 1,
    CRA_IPC_REQ_UI_SET_CURRENT_SCREEN = 2,
    CRA_IPC_REQ_RESERVED_3 = 3,
    CRA_IPC_REQ_THEME_GET_STATUS = 4,
    CRA_IPC_REQ_THEME_SET = 5,
    CRA_IPC_REQ_THEME_GET_INFO = 6,
    CRA_IPC_REQ_THEME_SET_BLOCKED_AUTO_SWITCH = 7,
    CRA_IPC_REQ_SETTINGS_GET = 8,
    CRA_IPC_REQ_SETTINGS_SET = 9,
    CRA_IPC_REQ_MEDIAPLAYER_GET_VIDEO_PATH = 10,
    CRA_IPC_REQ_MEDIAPLAYER_SET_VIDEO_PATH = 11,
    CRA_IPC_REQ_OVERLAY_SCHEDULE_TRANSITION = 12,
    CRA_IPC_REQ_OVERLAY_SCHEDULE_TRANSITION_VIDEO = 13,
    CRA_IPC_REQ_APP_EXIT = 14,
    CRA_IPC_REQ_THEME_RELOAD_ASSETS = 15,
    CRA_IPC_REQ_COUNT = 16
};

typedef int32_t cra_ipc_response_type_t;
enum {
    CRA_IPC_RESP_OK = 0,
    CRA_IPC_RESP_ERROR_MSG_TOO_LONG = 1,
    CRA_IPC_RESP_ERROR_NOMEM = 2,
    CRA_IPC_RESP_ERROR_INVALID_REQUEST = 3,
    CRA_IPC_RESP_ERROR_STATE_CONFLICT = 4,
    CRA_IPC_RESP_ERROR_LENGTH_MISMATCH = 5,
    CRA_IPC_RESP_ERROR_UNKNOWN = 6
};

typedef int32_t cra_screen_t;
enum {
    CRA_SCREEN_MAINMENU = 0,
    CRA_SCREEN_THEME_LIST = 1,
    CRA_SCREEN_SYSINFO = 2,
    CRA_SCREEN_SPINNER = 3,
    CRA_SCREEN_DISPLAYIMG = 4,
    CRA_SCREEN_FILEMANAGER = 5,
    CRA_SCREEN_SETTINGS = 6,
    CRA_SCREEN_WARNING = 7,
    CRA_SCREEN_CONFIRM = 8,
    CRA_SCREEN_APPLIST = 9
};

typedef int32_t cra_switch_interval_t;
enum {
    CRA_SWITCH_INTERVAL_1MIN = 0,
    CRA_SWITCH_INTERVAL_3MIN = 1,
    CRA_SWITCH_INTERVAL_5MIN = 2,
    CRA_SWITCH_INTERVAL_10MIN = 3,
    CRA_SWITCH_INTERVAL_30MIN = 4
};

typedef int32_t cra_switch_mode_t;
enum {
    CRA_SWITCH_MODE_SEQUENCE = 0,
    CRA_SWITCH_MODE_RANDOM = 1,
    CRA_SWITCH_MODE_MANUAL = 2
};

typedef int32_t cra_usb_mode_t;
enum {
    CRA_USB_MODE_MTP = 0,
    CRA_USB_MODE_SERIAL = 1,
    CRA_USB_MODE_RNDIS = 2,
    CRA_USB_MODE_NONE = 3,
    CRA_USB_MODE_EPMANAGER = 4
};

typedef int32_t cra_theme_state_t;
enum {
    CRA_THEME_STATE_IDLE = 0,
    CRA_THEME_STATE_TRANSITION_IN = 1,
    CRA_THEME_STATE_INTRO = 2,
    CRA_THEME_STATE_TRANSITION_LOOP = 3,
    CRA_THEME_STATE_PRE_THEME_INFO = 4
};

typedef int32_t cra_theme_source_t;
enum {
    CRA_THEME_SOURCE_NAND = 0,
    CRA_THEME_SOURCE_SD = 1
};

typedef int32_t cra_transition_type_t;
enum {
    CRA_TRANSITION_FADE = 0,
    CRA_TRANSITION_MOVE = 1,
    CRA_TRANSITION_SWIPE = 2,
    CRA_TRANSITION_NONE = 3
};

enum {
    CRA_SETTINGS_LOWBAT_TRIP = 1U << 0,
    CRA_SETTINGS_NO_INTRO = 1U << 1,
    CRA_SETTINGS_NO_OVERLAY = 1U << 2
};

typedef struct {
    char title[CRA_EPASS_WARNING_TITLE_SIZE];
    char desc[CRA_EPASS_WARNING_DESC_SIZE];
    char icon[CRA_EPASS_WARNING_ICON_SIZE];
    uint32_t color;
} cra_ipc_ui_warning_t;

typedef struct { cra_screen_t screen; } cra_ipc_screen_t;
typedef struct { int32_t theme_index; } cra_ipc_theme_index_t;
typedef struct { bool blocked; } cra_ipc_theme_block_t;

typedef struct {
    int32_t brightness;
    cra_switch_interval_t switch_interval;
    cra_switch_mode_t switch_mode;
    cra_usb_mode_t usb_mode;
    uint32_t control_word;
} cra_ipc_settings_t;

typedef struct { char path[CRA_EPASS_PATH_SIZE]; } cra_ipc_video_path_t;

typedef struct {
    int32_t duration;
    cra_transition_type_t type;
    char image_path[CRA_EPASS_PATH_SIZE];
    uint32_t background_color;
} cra_ipc_transition_t;

typedef struct {
    char video_path[CRA_EPASS_PATH_SIZE];
    int32_t duration;
    cra_transition_type_t type;
    char image_path[CRA_EPASS_PATH_SIZE];
    uint32_t background_color;
} cra_ipc_video_transition_t;

typedef struct { int32_t exit_code; } cra_ipc_app_exit_t;

typedef struct {
    cra_ipc_request_type_t type;
    union {
        cra_ipc_ui_warning_t ui_warning;
        cra_ipc_screen_t screen;
        cra_ipc_theme_index_t theme_index;
        cra_ipc_theme_block_t theme_block;
        cra_ipc_settings_t settings;
        cra_ipc_video_path_t video_path;
        cra_ipc_transition_t transition;
        cra_ipc_video_transition_t video_transition;
        cra_ipc_app_exit_t app_exit;
    } data;
} cra_ipc_request_t;

typedef struct {
    cra_theme_state_t state;
    int32_t theme_count;
    int32_t theme_index;
} cra_ipc_theme_status_t;

typedef struct {
    int32_t theme_index;
    char theme_name[CRA_EPASS_THEME_NAME_SIZE];
    uint8_t uuid[CRA_EPASS_UUID_SIZE];
    char description[CRA_EPASS_THEME_DESC_SIZE];
    char icon_path[CRA_EPASS_PATH_SIZE];
    cra_theme_source_t source;
} cra_ipc_theme_info_t;

typedef struct {
    cra_ipc_response_type_t type;
    union {
        cra_ipc_screen_t screen;
        cra_ipc_theme_status_t theme_status;
        cra_ipc_theme_info_t theme_info;
        cra_ipc_settings_t settings;
        cra_ipc_video_path_t video_path;
    } data;
} cra_ipc_response_t;

size_t cra_ipc_request_size(cra_ipc_request_type_t type);
size_t cra_ipc_response_size(cra_ipc_request_type_t request_type);
const char *cra_ipc_response_name(cra_ipc_response_type_t type);

#ifdef __cplusplus
}
#endif

#endif
