/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#include "cra_epass/ipc_client.h"
#include "cra_epass/ipc_v1.h"

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

typedef struct { const char *name; int32_t value; } name_value_t;

static bool json_output;
static const char *socket_path = CRA_EPASS_IPC_SOCKET_PATH;

static const name_value_t screens[] = {
    {"mainmenu", CRA_SCREEN_MAINMENU}, {"theme_list", CRA_SCREEN_THEME_LIST},
    {"oplist", CRA_SCREEN_THEME_LIST}, {"sysinfo", CRA_SCREEN_SYSINFO},
    {"spinner", CRA_SCREEN_SPINNER}, {"displayimg", CRA_SCREEN_DISPLAYIMG},
    {"filemanager", CRA_SCREEN_FILEMANAGER}, {"settings", CRA_SCREEN_SETTINGS},
    {"warning", CRA_SCREEN_WARNING}, {"confirm", CRA_SCREEN_CONFIRM},
    {"applist", CRA_SCREEN_APPLIST}
};
static const name_value_t theme_states[] = {
    {"idle", CRA_THEME_STATE_IDLE}, {"transition_in", CRA_THEME_STATE_TRANSITION_IN},
    {"intro", CRA_THEME_STATE_INTRO},
    {"transition_loop", CRA_THEME_STATE_TRANSITION_LOOP},
    {"pre_theme_info", CRA_THEME_STATE_PRE_THEME_INFO}
};
static const name_value_t intervals[] = {
    {"1min", CRA_SWITCH_INTERVAL_1MIN}, {"3min", CRA_SWITCH_INTERVAL_3MIN},
    {"5min", CRA_SWITCH_INTERVAL_5MIN}, {"10min", CRA_SWITCH_INTERVAL_10MIN},
    {"30min", CRA_SWITCH_INTERVAL_30MIN}
};
static const name_value_t switch_modes[] = {
    {"sequence", CRA_SWITCH_MODE_SEQUENCE}, {"random", CRA_SWITCH_MODE_RANDOM},
    {"manual", CRA_SWITCH_MODE_MANUAL}
};
static const name_value_t usb_modes[] = {
    {"mtp", CRA_USB_MODE_MTP}, {"serial", CRA_USB_MODE_SERIAL},
    {"rndis", CRA_USB_MODE_RNDIS}, {"none", CRA_USB_MODE_NONE},
    {"epmanager", CRA_USB_MODE_EPMANAGER}
};
static const name_value_t transitions[] = {
    {"fade", CRA_TRANSITION_FADE}, {"move", CRA_TRANSITION_MOVE},
    {"swipe", CRA_TRANSITION_SWIPE}
};
static const name_value_t exit_codes[] = {
    {"normal", 0}, {"restart", 1}, {"appstart", 2}, {"shutdown", 3},
    {"format_sd", 4}, {"device_config", 5}, {"srgn_config", 5}
};

static void json_string_to(FILE *stream, const char *value) {
    const unsigned char *p = (const unsigned char *)value;
    (void)fputc('"', stream);
    while (*p != 0U) {
        switch (*p) {
        case '"': (void)fputs("\\\"", stream); break;
        case '\\': (void)fputs("\\\\", stream); break;
        case '\b': (void)fputs("\\b", stream); break;
        case '\f': (void)fputs("\\f", stream); break;
        case '\n': (void)fputs("\\n", stream); break;
        case '\r': (void)fputs("\\r", stream); break;
        case '\t': (void)fputs("\\t", stream); break;
        default:
            if (*p < 0x20U) (void)fprintf(stream, "\\u%04x", (unsigned int)*p);
            else (void)fputc((int)*p, stream);
        }
        ++p;
    }
    (void)fputc('"', stream);
}

static void json_string(const char *value) {
    json_string_to(stdout, value);
}

static int report_error(const char *format, ...) {
    char message[256];
    va_list args;
    va_start(args, format);
    (void)vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    if (json_output) {
        (void)fputs("{\"status\":\"error\",\"message\":", stderr);
        json_string_to(stderr, message);
        (void)fputs("}\n", stderr);
    } else {
        (void)fprintf(stderr, "error: %s\n", message);
    }
    return 1;
}

static int print_ok(void) {
    (void)puts(json_output ? "{\"status\":\"ok\"}" : "ok");
    return 0;
}

static int lookup_value(const name_value_t *items, size_t count,
                        const char *name, int32_t *value) {
    size_t i;
    for (i = 0; i < count; ++i) {
        if (strcasecmp(items[i].name, name) == 0) {
            *value = items[i].value;
            return 0;
        }
    }
    return -1;
}

static const char *lookup_name(const name_value_t *items, size_t count,
                               int32_t value) {
    size_t i;
    for (i = 0; i < count; ++i) if (items[i].value == value) return items[i].name;
    return "unknown";
}

static int parse_i32(const char *text, int32_t *value) {
    char *end = NULL;
    long parsed;
    errno = 0;
    parsed = strtol(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' ||
        parsed < INT32_MIN || parsed > INT32_MAX) return -1;
    *value = (int32_t)parsed;
    return 0;
}

static int parse_u32(const char *text, uint32_t *value) {
    char *end = NULL;
    unsigned long parsed;
    if (*text == '-') return -1;
    errno = 0;
    parsed = strtoul(text, &end, 0);
    if (errno != 0 || end == text || *end != '\0' || parsed > UINT32_MAX) return -1;
    *value = (uint32_t)parsed;
    return 0;
}

static int parse_bool(const char *text, bool *value) {
    if (strcasecmp(text, "true") == 0 || strcmp(text, "1") == 0) *value = true;
    else if (strcasecmp(text, "false") == 0 || strcmp(text, "0") == 0) *value = false;
    else return -1;
    return 0;
}

static int copy_argument(char *destination, size_t capacity, const char *source,
                         const char *label) {
    size_t length = strlen(source);
    if (length >= capacity) return report_error("%s is too long (maximum %zu bytes)",
                                                label, capacity - 1U);
    (void)memcpy(destination, source, length + 1U);
    return 0;
}

static int exchange(cra_ipc_request_t *request, cra_ipc_response_t *response) {
    char error[192] = {0};
    size_t request_size = cra_ipc_request_size(request->type);
    size_t response_size = 0;
    size_t expected;
    if (request_size == 0U) return report_error("unsupported request type: %" PRId32,
                                                request->type);
    if (cra_ipc_exchange(socket_path, request, request_size, response,
                         sizeof(*response), &response_size, error, sizeof(error)) != 0)
        return report_error("%s", error);
    if (response_size < sizeof(response->type)) return report_error("truncated IPC response");
    if (response->type != CRA_IPC_RESP_OK)
        return report_error("device rejected request: %s",
                            cra_ipc_response_name(response->type));
    expected = cra_ipc_response_size(request->type);
    if (response_size != expected)
        return report_error("unexpected response size: got %zu, expected %zu",
                            response_size, expected);
    return 0;
}

static int command_ui(int argc, char **argv) {
    cra_ipc_request_t req = {0};
    cra_ipc_response_t resp = {0};
    int32_t value;
    if (argc < 1) return report_error("usage: epassctl ui <warning|get_screen|set_screen> ...");
    if (strcmp(argv[0], "warning") == 0) {
        if (argc != 5) return report_error("usage: epassctl ui warning <title> <desc> <icon> <color>");
        req.type = CRA_IPC_REQ_UI_WARNING;
        if (copy_argument(req.data.ui_warning.title, sizeof(req.data.ui_warning.title), argv[1], "title") ||
            copy_argument(req.data.ui_warning.desc, sizeof(req.data.ui_warning.desc), argv[2], "description") ||
            copy_argument(req.data.ui_warning.icon, sizeof(req.data.ui_warning.icon), argv[3], "icon")) return 1;
        if (parse_u32(argv[4], &req.data.ui_warning.color) != 0)
            return report_error("invalid color: %s", argv[4]);
        if (exchange(&req, &resp)) return 1;
        return print_ok();
    }
    if (strcmp(argv[0], "get_screen") == 0) {
        if (argc != 1) return report_error("usage: epassctl ui get_screen");
        req.type = CRA_IPC_REQ_UI_GET_CURRENT_SCREEN;
        if (exchange(&req, &resp)) return 1;
        const char *name = lookup_name(screens, sizeof(screens) / sizeof(screens[0]),
                                       resp.data.screen.screen);
        if (json_output) (void)printf("{\"screen\":\"%s\",\"screen_id\":%" PRId32 "}\n",
                                     name, resp.data.screen.screen);
        else (void)printf("screen: %s (%" PRId32 ")\n", name, resp.data.screen.screen);
        return 0;
    }
    if (strcmp(argv[0], "set_screen") == 0) {
        if (argc != 2 || lookup_value(screens, sizeof(screens) / sizeof(screens[0]), argv[1], &value))
            return report_error("usage: epassctl ui set_screen <screen_name>");
        req.type = CRA_IPC_REQ_UI_SET_CURRENT_SCREEN;
        req.data.screen.screen = value;
        if (exchange(&req, &resp)) return 1;
        return print_ok();
    }
    return report_error("unknown ui operation: %s", argv[0]);
}

static int command_theme(int argc, char **argv) {
    cra_ipc_request_t req = {0};
    cra_ipc_response_t resp = {0};
    int32_t index;
    bool blocked;
    if (argc < 1) return report_error("usage: epassctl theme <status|set|info|block_auto_switch|reload_assets>");
    if (strcmp(argv[0], "status") == 0) {
        if (argc != 1) return report_error("usage: epassctl theme status");
        req.type = CRA_IPC_REQ_THEME_GET_STATUS;
        if (exchange(&req, &resp)) return 1;
        const char *state = lookup_name(theme_states, sizeof(theme_states) / sizeof(theme_states[0]),
                                        resp.data.theme_status.state);
        if (json_output)
            (void)printf("{\"state\":\"%s\",\"state_id\":%" PRId32
                         ",\"theme_count\":%" PRId32 ",\"theme_index\":%" PRId32 "}\n",
                         state, resp.data.theme_status.state, resp.data.theme_status.theme_count,
                         resp.data.theme_status.theme_index);
        else
            (void)printf("state: %s (%" PRId32 ")\ntheme_count: %" PRId32
                         "\ntheme_index: %" PRId32 "\n", state, resp.data.theme_status.state,
                         resp.data.theme_status.theme_count, resp.data.theme_status.theme_index);
        return 0;
    }
    if (strcmp(argv[0], "set") == 0 || strcmp(argv[0], "set_operator") == 0) {
        if (argc != 2 || parse_i32(argv[1], &index)) return report_error("usage: epassctl theme set <index>");
        req.type = CRA_IPC_REQ_THEME_SET;
        req.data.theme_index.theme_index = index;
    } else if (strcmp(argv[0], "info") == 0 || strcmp(argv[0], "get_operator_info") == 0) {
        if (argc != 2 || parse_i32(argv[1], &index)) return report_error("usage: epassctl theme info <index>");
        req.type = CRA_IPC_REQ_THEME_GET_INFO;
        req.data.theme_index.theme_index = index;
        if (exchange(&req, &resp)) return 1;
        resp.data.theme_info.theme_name[sizeof(resp.data.theme_info.theme_name) - 1U] = '\0';
        resp.data.theme_info.description[sizeof(resp.data.theme_info.description) - 1U] = '\0';
        resp.data.theme_info.icon_path[sizeof(resp.data.theme_info.icon_path) - 1U] = '\0';
        char uuid[37];
        (void)snprintf(uuid, sizeof(uuid),
            "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            resp.data.theme_info.uuid[0], resp.data.theme_info.uuid[1],
            resp.data.theme_info.uuid[2], resp.data.theme_info.uuid[3],
            resp.data.theme_info.uuid[4], resp.data.theme_info.uuid[5],
            resp.data.theme_info.uuid[6], resp.data.theme_info.uuid[7],
            resp.data.theme_info.uuid[8], resp.data.theme_info.uuid[9],
            resp.data.theme_info.uuid[10], resp.data.theme_info.uuid[11],
            resp.data.theme_info.uuid[12], resp.data.theme_info.uuid[13],
            resp.data.theme_info.uuid[14], resp.data.theme_info.uuid[15]);
        if (json_output) {
            (void)printf("{\"theme_index\":%" PRId32 ",\"theme_name\":", resp.data.theme_info.theme_index);
            json_string(resp.data.theme_info.theme_name);
            (void)fputs(",\"uuid\":", stdout); json_string(uuid);
            (void)fputs(",\"description\":", stdout); json_string(resp.data.theme_info.description);
            (void)fputs(",\"icon_path\":", stdout); json_string(resp.data.theme_info.icon_path);
            (void)printf(",\"source\":\"%s\",\"source_id\":%" PRId32 "}\n",
                         resp.data.theme_info.source == CRA_THEME_SOURCE_SD ? "sd" : "nand",
                         resp.data.theme_info.source);
        } else {
            (void)printf("theme_index: %" PRId32 "\ntheme_name: %s\nuuid: %s\ndescription: %s\n"
                         "icon_path: %s\nsource: %s (%" PRId32 ")\n",
                         resp.data.theme_info.theme_index, resp.data.theme_info.theme_name, uuid,
                         resp.data.theme_info.description, resp.data.theme_info.icon_path,
                         resp.data.theme_info.source == CRA_THEME_SOURCE_SD ? "sd" : "nand",
                         resp.data.theme_info.source);
        }
        return 0;
    } else if (strcmp(argv[0], "block_auto_switch") == 0) {
        if (argc != 2 || parse_bool(argv[1], &blocked))
            return report_error("usage: epassctl theme block_auto_switch <true|false>");
        req.type = CRA_IPC_REQ_THEME_SET_BLOCKED_AUTO_SWITCH;
        req.data.theme_block.blocked = blocked;
    } else if (strcmp(argv[0], "reload_assets") == 0) {
        if (argc != 1) return report_error("usage: epassctl theme reload_assets");
        req.type = CRA_IPC_REQ_THEME_RELOAD_ASSETS;
    } else return report_error("unknown theme operation: %s", argv[0]);
    if (exchange(&req, &resp)) return 1;
    return print_ok();
}

static int get_settings(cra_ipc_settings_t *settings) {
    cra_ipc_request_t req = { .type = CRA_IPC_REQ_SETTINGS_GET };
    cra_ipc_response_t resp = {0};
    if (exchange(&req, &resp)) return 1;
    *settings = resp.data.settings;
    return 0;
}

static void print_settings(const cra_ipc_settings_t *s) {
    const char *interval = lookup_name(intervals, sizeof(intervals) / sizeof(intervals[0]), s->switch_interval);
    const char *mode = lookup_name(switch_modes, sizeof(switch_modes) / sizeof(switch_modes[0]), s->switch_mode);
    const char *usb = lookup_name(usb_modes, sizeof(usb_modes) / sizeof(usb_modes[0]), s->usb_mode);
    if (json_output)
        (void)printf("{\"brightness\":%" PRId32 ",\"interval\":\"%s\",\"interval_id\":%" PRId32
                     ",\"mode\":\"%s\",\"mode_id\":%" PRId32 ",\"usb\":\"%s\",\"usb_id\":%" PRId32
                     ",\"lowbat\":%s,\"no_intro\":%s,\"no_overlay\":%s}\n",
                     s->brightness, interval, s->switch_interval, mode, s->switch_mode, usb, s->usb_mode,
                     (s->control_word & CRA_SETTINGS_LOWBAT_TRIP) ? "true" : "false",
                     (s->control_word & CRA_SETTINGS_NO_INTRO) ? "true" : "false",
                     (s->control_word & CRA_SETTINGS_NO_OVERLAY) ? "true" : "false");
    else
        (void)printf("brightness: %" PRId32 "\ninterval: %s (%" PRId32 ")\nmode: %s (%" PRId32
                     ")\nusb: %s (%" PRId32 ")\nlowbat: %u\nno_intro: %u\nno_overlay: %u\n",
                     s->brightness, interval, s->switch_interval, mode, s->switch_mode, usb, s->usb_mode,
                     (s->control_word & CRA_SETTINGS_LOWBAT_TRIP) != 0U,
                     (s->control_word & CRA_SETTINGS_NO_INTRO) != 0U,
                     (s->control_word & CRA_SETTINGS_NO_OVERLAY) != 0U);
}

static int command_settings(int argc, char **argv) {
    cra_ipc_settings_t settings;
    cra_ipc_request_t req = {0};
    cra_ipc_response_t resp = {0};
    int i;
    if (argc == 1 && strcmp(argv[0], "get") == 0) {
        if (get_settings(&settings)) return 1;
        print_settings(&settings);
        return 0;
    }
    if (argc < 3 || strcmp(argv[0], "set") != 0 || ((argc - 1) % 2) != 0)
        return report_error("usage: epassctl settings set <key> <value> [key value ...]");
    if (get_settings(&settings)) return 1;
    for (i = 1; i < argc; i += 2) {
        int32_t value;
        bool enabled;
        uint32_t mask = 0U;
        if (strcasecmp(argv[i], "brightness") == 0) {
            if (parse_i32(argv[i + 1], &settings.brightness)) return report_error("invalid brightness");
        } else if (strcasecmp(argv[i], "interval") == 0) {
            if (lookup_value(intervals, sizeof(intervals) / sizeof(intervals[0]), argv[i + 1], &value))
                return report_error("invalid interval: %s", argv[i + 1]);
            settings.switch_interval = value;
        } else if (strcasecmp(argv[i], "mode") == 0) {
            if (lookup_value(switch_modes, sizeof(switch_modes) / sizeof(switch_modes[0]), argv[i + 1], &value))
                return report_error("invalid switch mode: %s", argv[i + 1]);
            settings.switch_mode = value;
        } else if (strcasecmp(argv[i], "usb") == 0) {
            if (lookup_value(usb_modes, sizeof(usb_modes) / sizeof(usb_modes[0]), argv[i + 1], &value))
                return report_error("invalid USB mode: %s", argv[i + 1]);
            settings.usb_mode = value;
        } else {
            if (strcasecmp(argv[i], "lowbat") == 0) mask = CRA_SETTINGS_LOWBAT_TRIP;
            else if (strcasecmp(argv[i], "no_intro") == 0) mask = CRA_SETTINGS_NO_INTRO;
            else if (strcasecmp(argv[i], "no_overlay") == 0) mask = CRA_SETTINGS_NO_OVERLAY;
            else return report_error("unknown setting: %s", argv[i]);
            if (parse_bool(argv[i + 1], &enabled)) return report_error("invalid boolean: %s", argv[i + 1]);
            if (enabled) settings.control_word |= mask; else settings.control_word &= ~mask;
        }
    }
    req.type = CRA_IPC_REQ_SETTINGS_SET;
    req.data.settings = settings;
    if (exchange(&req, &resp)) return 1;
    return print_ok();
}

static int command_mediaplayer(int argc, char **argv) {
    cra_ipc_request_t req = {0};
    cra_ipc_response_t resp = {0};
    if (argc == 1 && strcmp(argv[0], "get_video") == 0) {
        req.type = CRA_IPC_REQ_MEDIAPLAYER_GET_VIDEO_PATH;
        if (exchange(&req, &resp)) return 1;
        if (json_output) { (void)fputs("{\"video_path\":", stdout); json_string(resp.data.video_path.path); (void)puts("}"); }
        else (void)printf("video_path: %s\n", resp.data.video_path.path);
        return 0;
    }
    if (argc == 2 && strcmp(argv[0], "set_video") == 0) {
        req.type = CRA_IPC_REQ_MEDIAPLAYER_SET_VIDEO_PATH;
        if (copy_argument(req.data.video_path.path, sizeof(req.data.video_path.path), argv[1], "video path")) return 1;
        if (exchange(&req, &resp)) return 1;
        return print_ok();
    }
    return report_error("usage: epassctl mediaplayer <get_video|set_video> [path]");
}

static int parse_transition(const char *text, cra_transition_type_t *value) {
    return lookup_value(transitions, sizeof(transitions) / sizeof(transitions[0]), text, value);
}

static int command_overlay(int argc, char **argv) {
    cra_ipc_request_t req = {0};
    cra_ipc_response_t resp = {0};
    int32_t duration;
    if (argc == 5 && strcmp(argv[0], "transition") == 0) {
        req.type = CRA_IPC_REQ_OVERLAY_SCHEDULE_TRANSITION;
        if (parse_i32(argv[1], &duration) || duration < 0 ||
            parse_transition(argv[2], &req.data.transition.type) ||
            parse_u32(argv[4], &req.data.transition.background_color))
            return report_error("usage: epassctl overlay transition <duration_ms> <fade|move|swipe> <image|-> <color>");
        req.data.transition.duration = duration;
        if (strcmp(argv[3], "-") != 0 && copy_argument(req.data.transition.image_path,
            sizeof(req.data.transition.image_path), argv[3], "image path")) return 1;
    } else if (argc == 6 && strcmp(argv[0], "transition_video") == 0) {
        req.type = CRA_IPC_REQ_OVERLAY_SCHEDULE_TRANSITION_VIDEO;
        if (copy_argument(req.data.video_transition.video_path,
                          sizeof(req.data.video_transition.video_path), argv[1], "video path") ||
            parse_i32(argv[2], &duration) || duration < 0 ||
            parse_transition(argv[3], &req.data.video_transition.type) ||
            parse_u32(argv[5], &req.data.video_transition.background_color))
            return report_error("usage: epassctl overlay transition_video <video> <duration_ms> <fade|move|swipe> <image|-> <color>");
        req.data.video_transition.duration = duration;
        if (strcmp(argv[4], "-") != 0 && copy_argument(req.data.video_transition.image_path,
            sizeof(req.data.video_transition.image_path), argv[4], "image path")) return 1;
    } else return report_error("usage: epassctl overlay <transition|transition_video> ...");
    if (exchange(&req, &resp)) return 1;
    return print_ok();
}

static int command_app(int argc, char **argv) {
    cra_ipc_request_t req = { .type = CRA_IPC_REQ_APP_EXIT };
    cra_ipc_response_t resp = {0};
    int32_t code;
    if (argc != 2 || strcmp(argv[0], "exit") != 0) return report_error("usage: epassctl app exit <code|name>");
    if (lookup_value(exit_codes, sizeof(exit_codes) / sizeof(exit_codes[0]), argv[1], &code) != 0 &&
        parse_i32(argv[1], &code) != 0) return report_error("invalid exit code: %s", argv[1]);
    req.data.app_exit.exit_code = code;
    if (exchange(&req, &resp)) return 1;
    return print_ok();
}

static void print_help(void) {
    (void)puts(
        "CRA Electric Pass control client\n"
        "usage: epassctl [json] <module> <operation> [arguments...]\n\n"
        "modules:\n"
        "  ui           warning, get_screen, set_screen\n"
        "  theme|prts   status, set, info, block_auto_switch, reload_assets\n"
        "  settings     get, set\n"
        "  mediaplayer  get_video, set_video\n"
        "  overlay      transition, transition_video\n"
        "  app          exit\n\n"
        "Set CRA_EPASS_SOCKET to override the default IPC socket for testing.");
}

int main(int argc, char **argv) {
    int index = 1;
    const char *environment_socket = getenv("CRA_EPASS_SOCKET");
    if (environment_socket != NULL && *environment_socket != '\0') socket_path = environment_socket;
    if (index < argc && strcmp(argv[index], "json") == 0) { json_output = true; ++index; }
    if (index >= argc || strcmp(argv[index], "help") == 0 || strcmp(argv[index], "--help") == 0) {
        print_help();
        return 0;
    }
    const char *module = argv[index++];
    if (strcmp(module, "ui") == 0) return command_ui(argc - index, argv + index);
    if (strcmp(module, "theme") == 0 || strcmp(module, "prts") == 0)
        return command_theme(argc - index, argv + index);
    if (strcmp(module, "settings") == 0) return command_settings(argc - index, argv + index);
    if (strcmp(module, "mediaplayer") == 0) return command_mediaplayer(argc - index, argv + index);
    if (strcmp(module, "overlay") == 0) return command_overlay(argc - index, argv + index);
    if (strcmp(module, "app") == 0) return command_app(argc - index, argv + index);
    return report_error("unknown module: %s", module);
}
