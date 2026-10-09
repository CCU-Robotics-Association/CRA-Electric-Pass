/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#include "cra_epass/ipc_v1.h"

_Static_assert(sizeof(cra_ipc_request_type_t) == 4, "request type ABI changed");
_Static_assert(sizeof(cra_ipc_response_type_t) == 4, "response type ABI changed");
_Static_assert(sizeof(cra_ipc_settings_t) == 20, "settings ABI changed");
_Static_assert(sizeof(cra_ipc_theme_info_t) == 448, "theme info ABI changed");
_Static_assert(sizeof(cra_ipc_request_t) <= CRA_EPASS_IPC_MAX_MESSAGE,
               "request exceeds transport limit");
_Static_assert(sizeof(cra_ipc_response_t) <= CRA_EPASS_IPC_MAX_MESSAGE,
               "response exceeds transport limit");

size_t cra_ipc_request_size(cra_ipc_request_type_t type) {
    size_t size = sizeof(cra_ipc_request_type_t);
    switch (type) {
    case CRA_IPC_REQ_UI_WARNING: size += sizeof(cra_ipc_ui_warning_t); break;
    case CRA_IPC_REQ_UI_SET_CURRENT_SCREEN: size += sizeof(cra_ipc_screen_t); break;
    case CRA_IPC_REQ_THEME_SET:
    case CRA_IPC_REQ_THEME_GET_INFO: size += sizeof(cra_ipc_theme_index_t); break;
    case CRA_IPC_REQ_THEME_SET_BLOCKED_AUTO_SWITCH:
        size += sizeof(cra_ipc_theme_block_t); break;
    case CRA_IPC_REQ_SETTINGS_SET: size += sizeof(cra_ipc_settings_t); break;
    case CRA_IPC_REQ_MEDIAPLAYER_SET_VIDEO_PATH:
        size += sizeof(cra_ipc_video_path_t); break;
    case CRA_IPC_REQ_OVERLAY_SCHEDULE_TRANSITION:
        size += sizeof(cra_ipc_transition_t); break;
    case CRA_IPC_REQ_OVERLAY_SCHEDULE_TRANSITION_VIDEO:
        size += sizeof(cra_ipc_video_transition_t); break;
    case CRA_IPC_REQ_APP_EXIT: size += sizeof(cra_ipc_app_exit_t); break;
    case CRA_IPC_REQ_UI_GET_CURRENT_SCREEN:
    case CRA_IPC_REQ_RESERVED_3:
    case CRA_IPC_REQ_THEME_GET_STATUS:
    case CRA_IPC_REQ_SETTINGS_GET:
    case CRA_IPC_REQ_MEDIAPLAYER_GET_VIDEO_PATH:
    case CRA_IPC_REQ_THEME_RELOAD_ASSETS:
        break;
    default: return 0;
    }
    return size;
}

size_t cra_ipc_response_size(cra_ipc_request_type_t request_type) {
    size_t size = sizeof(cra_ipc_response_type_t);
    switch (request_type) {
    case CRA_IPC_REQ_UI_GET_CURRENT_SCREEN: size += sizeof(cra_ipc_screen_t); break;
    case CRA_IPC_REQ_THEME_GET_STATUS: size += sizeof(cra_ipc_theme_status_t); break;
    case CRA_IPC_REQ_THEME_GET_INFO: size += sizeof(cra_ipc_theme_info_t); break;
    case CRA_IPC_REQ_SETTINGS_GET: size += sizeof(cra_ipc_settings_t); break;
    case CRA_IPC_REQ_MEDIAPLAYER_GET_VIDEO_PATH:
        size += sizeof(cra_ipc_video_path_t); break;
    case CRA_IPC_REQ_UI_WARNING:
    case CRA_IPC_REQ_UI_SET_CURRENT_SCREEN:
    case CRA_IPC_REQ_RESERVED_3:
    case CRA_IPC_REQ_THEME_SET:
    case CRA_IPC_REQ_THEME_SET_BLOCKED_AUTO_SWITCH:
    case CRA_IPC_REQ_SETTINGS_SET:
    case CRA_IPC_REQ_MEDIAPLAYER_SET_VIDEO_PATH:
    case CRA_IPC_REQ_OVERLAY_SCHEDULE_TRANSITION:
    case CRA_IPC_REQ_OVERLAY_SCHEDULE_TRANSITION_VIDEO:
    case CRA_IPC_REQ_APP_EXIT:
    case CRA_IPC_REQ_THEME_RELOAD_ASSETS:
        break;
    default: return 0;
    }
    return size;
}

const char *cra_ipc_response_name(cra_ipc_response_type_t type) {
    switch (type) {
    case CRA_IPC_RESP_OK: return "ok";
    case CRA_IPC_RESP_ERROR_MSG_TOO_LONG: return "message too long";
    case CRA_IPC_RESP_ERROR_NOMEM: return "out of memory";
    case CRA_IPC_RESP_ERROR_INVALID_REQUEST: return "invalid request";
    case CRA_IPC_RESP_ERROR_STATE_CONFLICT: return "state conflict";
    case CRA_IPC_RESP_ERROR_LENGTH_MISMATCH: return "message length mismatch";
    case CRA_IPC_RESP_ERROR_UNKNOWN: return "unknown request";
    default: return "unknown response";
    }
}
