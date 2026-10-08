#include "theme/theme.h"
#include "theme/loader.h"
#include "utils/cJSON.h"
#include "utils/stb_image.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>
#include "utils/log.h"
#include "utils/misc.h"

/*
 * 新配置统一使用中性主题字段；第二个键仅用于读取旧版主题包。
 * 兼容键不会再写入新配置，也不会继续传播到内部数据结构。
 */
static const char *json_get_string_compat(
    cJSON *object,
    const char *key,
    const char *legacy_key
) {
    const char *value = json_get_string(object, key);
    if (value || !legacy_key) {
        return value;
    }
    return json_get_string(object, legacy_key);
}

// 可选图片通用校验规则（优化版：仅检查文件存在性，不加载图片）：
// - json字段不存在 / 非字符串 / 空字符串 => 视为不存在，dst置空
// - 若存在且不为空：join到绝对路径，检查文件存在且可读
// - 不满足 => 视为不存在，dst置空
// 注意：图片格式/尺寸验证推迟到实际加载时进行
static void validate_optional_image_path(
    theme_t *manager,
    const char *theme_dir,
    const char *field_for_log,
    const char *rel_path,
    char *dst,
    size_t dst_sz
) {
    if (!dst || dst_sz == 0) return;
    dst[0] = '\0';

    if (!rel_path || rel_path[0] == '\0') {
        return;
    }

    char abs_path[256];
    abs_path[0] = '\0';
    join_path(abs_path, sizeof(abs_path), theme_dir, rel_path);

    // 仅检查文件存在和可读性，不加载图片（性能优化）
    if (!file_exists_readable(abs_path)) {
        parse_log_file(manager->parse_log_f, (char*)theme_dir, (char*)field_for_log, PARSE_LOG_WARN);
        return;
    }

    safe_strcpy(dst, dst_sz, abs_path);
}

static int parse_transition_obj(
    theme_t *manager,
    const char *theme_dir,
    const char *which,
    cJSON *tr_obj,
    oltr_params_t *out
) {
    if (!out) return -1;
    memset(out, 0, sizeof(*out));
    out->type = TRANSITION_TYPE_NONE;
    out->background_color = 0xFF000000u;
    out->image_path[0] = '\0';

    if (!tr_obj) {
        // 可选：不存在则认为 none
        return 0;
    }
    if (!cJSON_IsObject(tr_obj)) {
        parse_log_file(manager->parse_log_f, (char*)theme_dir, (char*)which, PARSE_LOG_ERROR);
        return -1;
    }

    const char *t = json_get_string(tr_obj, "type");
    if (!t) {
        parse_log_file(manager->parse_log_f, (char*)theme_dir, "transition 缺少 type", PARSE_LOG_ERROR);
        return -1;
    }

    if (strcmp(t, "none") == 0) {
        out->type = TRANSITION_TYPE_NONE;
        return 0;
    }
    if (strcmp(t, "fade") == 0) out->type = TRANSITION_TYPE_FADE;
    else if (strcmp(t, "move") == 0) out->type = TRANSITION_TYPE_MOVE;
    else if (strcmp(t, "swipe") == 0) out->type = TRANSITION_TYPE_SWIPE;
    else {
        parse_log_file(manager->parse_log_f, (char*)theme_dir, "transition.type 不合法", PARSE_LOG_ERROR);
        return -1;
    }

    cJSON *opt = cJSON_GetObjectItem(tr_obj, "options");
    if (!opt || !cJSON_IsObject(opt)) {
        parse_log_file(manager->parse_log_f, (char*)theme_dir, "transition.type!=none 但 options 不存在", PARSE_LOG_ERROR);
        return -1;
    }

    out->duration = json_get_int(opt, "duration", 0);
    if (out->duration <= 0) {
        parse_log_file(manager->parse_log_f, (char*)theme_dir, "transition.options.duration<=0", PARSE_LOG_ERROR);
        return -1;
    }

    const char *bg = json_get_string(opt, "background_color");
    out->background_color = (bg && is_hex_color_6(bg)) ? parse_rgbff(bg) : 0xFF000000u;

    const char *img = json_get_string(opt, "image");
    // 可选图片通用校验规则
    char warn_msg[128];
    snprintf(warn_msg, sizeof(warn_msg), "%s.options.image 校验失败，按不存在处理", which);
    validate_optional_image_path(manager, theme_dir, warn_msg, img, out->image_path, sizeof(out->image_path));
    return 0;
}


int theme_try_load(
    theme_t *manager,
    theme_entry_t *entry,
    const char *path,
    theme_source_t source,
    int index
) {
    if (!manager || !path || strlen(path) == 0) {
        return -1;
    }
    if (!entry) {
        parse_log_file(manager->parse_log_f, path, "theme 指针为空", PARSE_LOG_ERROR);
        return -1;
    }
    memset(entry, 0, sizeof(*entry));
    entry->index = index;
    entry->source = source;
    entry->overlay_params.type = THEME_INFO_TYPE_NONE;
    entry->transition_in.type = TRANSITION_TYPE_NONE;
    entry->transition_in.background_color = 0xFF000000u;
    entry->transition_loop.type = TRANSITION_TYPE_NONE;
    entry->transition_loop.background_color = 0xFF000000u;

    char cfg_path[256];
    join_path(cfg_path, sizeof(cfg_path), path, THEME_CONFIG_FILENAME);

    size_t json_len = 0;
    char *buf = read_file_all(cfg_path, &json_len);
    if (!buf) {
        parse_log_file(manager->parse_log_f, path, THEME_CONFIG_FILENAME "不存在或读取失败", PARSE_LOG_ERROR);
        return -1;
    }

    cJSON *json = cJSON_Parse(buf);
    free(buf);
    if(json == NULL){
        parse_log_file(manager->parse_log_f, path, THEME_CONFIG_FILENAME "解析失败", PARSE_LOG_ERROR);
        return -1;
    }

    // ===== version =====
    cJSON *ver = cJSON_GetObjectItem(json, "version");
    if (!ver || !cJSON_IsNumber(ver) || ver->valueint != THEME_CONFIG_VERSION) {
        parse_log_file(manager->parse_log_f, path, "version 校验失败", PARSE_LOG_ERROR);
        cJSON_Delete(json);
        return -1;
    }

    // ===== top-level basic fields =====
    const char *name = json_get_string(json, "name");
    if (!name || name[0] == '\0') {
        // name 可选：没有就用文件夹名称
        name = path_basename(path);
    }
    safe_strcpy(entry->theme_name, sizeof(entry->theme_name), name);

    const char *uuid_str = json_get_string(json, "uuid");
    if (!uuid_str || uuid_str[0] == '\0') {
        parse_log_file(manager->parse_log_f, path, "缺少字段 uuid", PARSE_LOG_ERROR);
        cJSON_Delete(json);
        return -1;
    }
    if (uuid_parse(uuid_str, &entry->uuid) != 0) {
        parse_log_file(manager->parse_log_f, path, "uuid 解析失败", PARSE_LOG_ERROR);
        cJSON_Delete(json);
        return -1;
    }

    const char *desc = json_get_string(json, "description");
    if (!desc || desc[0] == '\0') desc = "(无描述)";
    safe_strcpy(entry->description, sizeof(entry->description), desc);

    // icon: 可选，输出为 LVGL path（A: 前缀）；不存在则用默认 icon
    const char *icon = json_get_string(json, "icon");
    if (!icon || icon[0] == '\0') {
        safe_strcpy(entry->icon_path, sizeof(entry->icon_path), THEME_DEFAULT_ICON_PATH);
    } else {
        char abs_icon[256];
        abs_icon[0] = '\0';
        join_path(abs_icon, sizeof(abs_icon), path, icon);
        if (!file_exists_readable(abs_icon)) {
            parse_log_file(manager->parse_log_f, path, "icon 文件不存在，使用默认icon", PARSE_LOG_WARN);
            safe_strcpy(entry->icon_path, sizeof(entry->icon_path), THEME_DEFAULT_ICON_PATH);
        } else {
            set_lvgl_path(entry->icon_path, sizeof(entry->icon_path), abs_icon);
        }
    }

    const char *screen = json_get_string(json, "screen");
    if (!screen || screen[0] == '\0') {
        parse_log_file(manager->parse_log_f, path, "缺少字段 screen", PARSE_LOG_ERROR);
        cJSON_Delete(json);
        return -1;
    }
#if defined(USE_360_640_SCREEN)
    if (strcmp(screen, "360x640") != 0) {
        parse_log_file(manager->parse_log_f, path, "screen 与当前固件配置不匹配", PARSE_LOG_ERROR);
        cJSON_Delete(json);
        return -1;
    }
    entry->disp_type = DISPLAY_360_640;
#elif defined(USE_480_854_SCREEN)
    if (strcmp(screen, "480x854") != 0) {
        parse_log_file(manager->parse_log_f, path, "screen 与当前固件配置不匹配", PARSE_LOG_ERROR);
        cJSON_Delete(json);
        return -1;
    }
    entry->disp_type = DISPLAY_480_854
#elif defined(USE_720_1280_SCREEN)
    if (strcmp(screen, "720x1280") != 0) {
        parse_log_file(manager->parse_log_f, path, "screen 与当前固件配置不匹配", PARSE_LOG_ERROR);
        cJSON_Delete(json);
        return -1;
    }
    entry->disp_type = DISPLAY_720_1280;
#endif


    // ===== loop / intro videos =====
    cJSON *loop = cJSON_GetObjectItem(json, "loop");
    if (!loop || !cJSON_IsObject(loop)) {
        parse_log_file(manager->parse_log_f, path, "缺少对象 loop", PARSE_LOG_ERROR);
        cJSON_Delete(json);
        return -1;
    }
    const char *loop_file = json_get_string(loop, "file");
    if (!loop_file || loop_file[0] == '\0') {
        parse_log_file(manager->parse_log_f, path, "缺少 loop.file", PARSE_LOG_ERROR);
        cJSON_Delete(json);
        return -1;
    }
    join_path(entry->loop_video.path, sizeof(entry->loop_video.path), path, loop_file);
    if (!file_exists_readable(entry->loop_video.path)) {
        parse_log_file(manager->parse_log_f, path, "loop.file 文件不存在", PARSE_LOG_ERROR);
        cJSON_Delete(json);
        return -1;
    }

    cJSON *intro = cJSON_GetObjectItem(json, "intro");
    if (!intro || !cJSON_IsObject(intro)) {
        entry->intro_video.enabled = false;
        entry->intro_video.duration = 0;
        entry->intro_video.path[0] = '\0';
    } else {
        entry->intro_video.enabled = json_get_bool(intro, "enabled", false);
        entry->intro_video.duration = json_get_int(intro, "duration", 0);
        const char *intro_file = json_get_string(intro, "file");
        if (entry->intro_video.enabled) {
            if (!intro_file || intro_file[0] == '\0') {
                parse_log_file(manager->parse_log_f, path, "intro.enabled=true 但缺少 intro.file", PARSE_LOG_ERROR);
                cJSON_Delete(json);
                return -1;
            }
            if (entry->intro_video.duration <= 0) {
                parse_log_file(manager->parse_log_f, path, "intro.enabled=true 但 duration<=0", PARSE_LOG_ERROR);
                cJSON_Delete(json);
                return -1;
            }
            join_path(entry->intro_video.path, sizeof(entry->intro_video.path), path, intro_file);
            if (!file_exists_readable(entry->intro_video.path)) {
                parse_log_file(manager->parse_log_f, path, "intro.file 文件不存在", PARSE_LOG_ERROR);
                cJSON_Delete(json);
                return -1;
            }
        } else {
            entry->intro_video.duration = 0;
            entry->intro_video.path[0] = '\0';
        }
    }

    // ===== transitions (only keep params: duration/image/background_color) =====
    cJSON *tr_in = cJSON_GetObjectItem(json, "transition_in");
    if (parse_transition_obj(manager, path, "transition_in", tr_in, &entry->transition_in) != 0) {
        cJSON_Delete(json);
        return -1;
    }
    cJSON *tr_lp = cJSON_GetObjectItem(json, "transition_loop");
    if (parse_transition_obj(manager, path, "transition_loop", tr_lp, &entry->transition_loop) != 0) {
        cJSON_Delete(json);
        return -1;
    }

    // ===== overlay / theme_info (图片不加载，只填路径) =====
    cJSON *overlay = cJSON_GetObjectItem(json, "overlay");
    if (overlay && cJSON_IsObject(overlay)) {
        const char *ov_type = json_get_string(overlay, "type");
        cJSON *opt = cJSON_GetObjectItem(overlay, "options");

        if (!ov_type) {
            // 缺 type 时按 none 处理
            entry->overlay_params.type = THEME_INFO_TYPE_NONE;
        } else if (strcmp(ov_type, "none") == 0) {
            entry->overlay_params.type = THEME_INFO_TYPE_NONE;
        } else if (
            strcmp(ov_type, "cra_pass") == 0 ||
            strcmp(ov_type, "arknights") == 0
        ) {
            entry->overlay_params.type = THEME_INFO_TYPE_CRA_PASS;
            if (!opt || !cJSON_IsObject(opt)) {
                parse_log_file(manager->parse_log_f, path, "overlay.type=cra_pass 但 options 不存在", PARSE_LOG_ERROR);
                cJSON_Delete(json);
                return -1;
            }
            // appear_time
            int appear_time = json_get_int(opt, "appear_time", 0);
            if (appear_time <= 0) {
                parse_log_file(manager->parse_log_f, path, "overlay.options.appear_time<=0", PARSE_LOG_ERROR);
                cJSON_Delete(json);
                return -1;
            }
            entry->overlay_params.appear_time = appear_time;

            const char *display_name =
                json_get_string_compat(opt, "display_name", "operator_name");
            const char *display_code =
                json_get_string_compat(opt, "display_code", "operator_code");
            const char *bct = json_get_string(opt, "barcode_text");
            const char *aux = json_get_string(opt, "aux_text");
            const char *stf = json_get_string(opt, "staff_text");

            safe_strcpy(
                entry->overlay_params.display_name,
                sizeof(entry->overlay_params.display_name),
                (display_name && display_name[0]) ? display_name : "CRA E-PASS"
            );
            safe_strcpy(
                entry->overlay_params.display_code,
                sizeof(entry->overlay_params.display_code),
                (display_code && display_code[0]) ? display_code : "CRA-0000"
            );
            safe_strcpy(
                entry->overlay_params.barcode_text,
                sizeof(entry->overlay_params.barcode_text),
                (bct && bct[0]) ? bct : "CCU ROBOTICS ASSOCIATION"
            );
            safe_strcpy(entry->overlay_params.aux_text, sizeof(entry->overlay_params.aux_text),
                        (aux && aux[0]) ? aux : "CRA Electric Pass\nCCU Robotics Association");
            safe_strcpy(entry->overlay_params.staff_text, sizeof(entry->overlay_params.staff_text), (stf && stf[0]) ? stf : "CRA");

            const char *color = json_get_string(opt, "color");
            entry->overlay_params.color = (color && is_hex_color_6(color)) ? parse_rgbff(color) : 0xFF000000u;

            const char *logo = json_get_string(opt, "logo");
            validate_optional_image_path(manager, path, "overlay.options.logo 校验失败，按不存在处理", logo, entry->overlay_params.logo_path, sizeof(entry->overlay_params.logo_path));
            const char *cls =
                json_get_string_compat(opt, "class_icon", "operator_class_icon");
            validate_optional_image_path(manager, path, "overlay.options.class_icon 校验失败，按不存在处理", cls, entry->overlay_params.class_path, sizeof(entry->overlay_params.class_path));

            const char *top_left_text =
                json_get_string_compat(opt, "top_left_text", "top_left_rhodes");
            safe_strcpy(entry->overlay_params.top_left_text, sizeof(entry->overlay_params.top_left_text),
                        (top_left_text && top_left_text[0]) ? top_left_text : "");

            const char *trbt = json_get_string(opt, "top_right_bar_text");
            safe_strcpy(entry->overlay_params.top_right_bar_text, sizeof(entry->overlay_params.top_right_bar_text),
                        (trbt && trbt[0]) ? trbt : "");

        } else if (strcmp(ov_type, "cra") == 0) {
            entry->overlay_params.type = THEME_INFO_TYPE_CRA;
            if (!opt || !cJSON_IsObject(opt)) {
                parse_log_file(manager->parse_log_f, path, "overlay.type=cra 但 options 不存在", PARSE_LOG_ERROR);
                cJSON_Delete(json);
                return -1;
            }

            int appear_time = json_get_int(opt, "appear_time", 0);
            if (appear_time <= 0) {
                parse_log_file(manager->parse_log_f, path, "overlay.options.appear_time<=0", PARSE_LOG_ERROR);
                cJSON_Delete(json);
                return -1;
            }
            entry->overlay_params.appear_time = appear_time;

            const char *member_name = json_get_string(opt, "member_name");
            const char *member_code = json_get_string(opt, "member_code");
            const char *role_text = json_get_string(opt, "role_text");
            const char *detail_text = json_get_string(opt, "detail_text");
            const char *organization_text = json_get_string(opt, "organization_text");
            const char *header_text = json_get_string(opt, "header_text");

            safe_strcpy(
                entry->overlay_params.cra.member_name,
                sizeof(entry->overlay_params.cra.member_name),
                (member_name && member_name[0]) ? member_name : "CRA MEMBER"
            );
            safe_strcpy(
                entry->overlay_params.cra.member_code,
                sizeof(entry->overlay_params.cra.member_code),
                (member_code && member_code[0]) ? member_code : "CRA-0000"
            );
            safe_strcpy(
                entry->overlay_params.cra.role_text,
                sizeof(entry->overlay_params.cra.role_text),
                (role_text && role_text[0]) ? role_text : "MEMBER"
            );
            safe_strcpy(
                entry->overlay_params.cra.detail_text,
                sizeof(entry->overlay_params.cra.detail_text),
                (detail_text && detail_text[0]) ? detail_text : "长春大学机器人协会"
            );
            safe_strcpy(
                entry->overlay_params.cra.organization_text,
                sizeof(entry->overlay_params.cra.organization_text),
                (organization_text && organization_text[0])
                    ? organization_text
                    : "CCU ROBOTICS ASSOCIATION"
            );
            safe_strcpy(
                entry->overlay_params.cra.header_text,
                sizeof(entry->overlay_params.cra.header_text),
                (header_text && header_text[0]) ? header_text : "ELECTRIC PASS"
            );

            const char *color = json_get_string(opt, "color");
            entry->overlay_params.cra.color =
                (color && is_hex_color_6(color)) ? parse_rgbff(color) : 0xFF4B3B99u;

            const char *logo = json_get_string(opt, "logo");
            validate_optional_image_path(
                manager,
                path,
                "overlay.options.logo 校验失败，按不存在处理",
                logo,
                entry->overlay_params.cra.logo_path,
                sizeof(entry->overlay_params.cra.logo_path)
            );

        } else if (strcmp(ov_type, "image") == 0) {
            entry->overlay_params.type = THEME_INFO_TYPE_IMAGE;
            if (!opt || !cJSON_IsObject(opt)) {
                parse_log_file(manager->parse_log_f, path, "overlay.type=image 但 options 不存在", PARSE_LOG_ERROR);
                cJSON_Delete(json);
                return -1;
            }
            int appear_time = json_get_int(opt, "appear_time", 0);
            if (appear_time <= 0) {
                parse_log_file(manager->parse_log_f, path, "overlay.options.appear_time<=0", PARSE_LOG_ERROR);
                cJSON_Delete(json);
                return -1;
            }
            entry->overlay_params.appear_time = appear_time;
            entry->overlay_params.duration = json_get_int(opt, "duration", 0);
            if (entry->overlay_params.duration <= 0) {
                parse_log_file(manager->parse_log_f, path, "overlay.options.duration<=0", PARSE_LOG_ERROR);
                cJSON_Delete(json);
                return -1;
            }

            const char *img = json_get_string(opt, "image");
            validate_optional_image_path(manager, path, "overlay.options.image 校验失败，按不存在处理", img, entry->overlay_params.image_path, sizeof(entry->overlay_params.image_path));
        } else {
            parse_log_file(manager->parse_log_f, path, "overlay.type 不合法", PARSE_LOG_ERROR);
            cJSON_Delete(json);
            return -1;
        }
    }

    cJSON_Delete(json);
    return 0;
}

int theme_scan_assets(theme_t *manager, const char *dirpath, theme_source_t source){
    int error_cnt = 0;
    char path[128];
    DIR *dir = opendir(dirpath);
    if (!dir) {
        parse_log_file(manager->parse_log_f, dirpath, "无法打开素材目录", PARSE_LOG_WARN);
        return 0;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL){
        if(entry->d_type != DT_DIR){
            continue;
        }
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        if (manager->theme_count >= THEME_MAX) {
            parse_log_file(manager->parse_log_f, dirpath, "素材数量超过最大值", PARSE_LOG_WARN);
            break;
        }

        join_path(path, sizeof(path), dirpath, entry->d_name);
        if (theme_try_load(manager, &manager->themes[manager->theme_count], path, source, manager->theme_count) == 0) {
            manager->theme_count++;
        }
        else{
            error_cnt ++;
        }
    }
    closedir(dir);
    return error_cnt;
}
#ifndef APP_RELEASE
void theme_log_entry(const theme_entry_t *entry){
    log_debug("name: %s", entry->theme_name);
    log_debug("uuid: ");
    uuid_print(&entry->uuid);
    log_trace("description: %s", entry->description);
    log_trace("icon_path: %s", entry->icon_path);
    log_trace("disp_type: %d", entry->disp_type);
    log_trace("intro_video.enabled: %d", entry->intro_video.enabled);
    log_trace("intro_video.duration: %d", entry->intro_video.duration);
    log_trace("intro_video.path: %s", entry->intro_video.path);
    log_trace("loop_video.enabled: %d", entry->loop_video.enabled);
    log_trace("loop_video.duration: %d", entry->loop_video.duration);
    log_trace("loop_video.path: %s", entry->loop_video.path);
    log_trace("overlay_params.type: %d", entry->overlay_params.type);
    log_trace("overlay_params.appear_time: %d", entry->overlay_params.appear_time);
    log_trace("overlay_params.duration: %d", entry->overlay_params.duration);
    log_trace("overlay_params.display_name: %s", entry->overlay_params.display_name);
    log_trace("overlay_params.display_code: %s", entry->overlay_params.display_code);
    log_trace("overlay_params.barcode_text: %s", entry->overlay_params.barcode_text);
    log_trace("overlay_params.aux_text: %s", entry->overlay_params.aux_text);
    log_trace("overlay_params.staff_text: %s", entry->overlay_params.staff_text);
    log_trace("overlay_params.color: %x", entry->overlay_params.color);
    log_trace("overlay_params.logo_path: %s", entry->overlay_params.logo_path);
    log_trace("overlay_params.class_path: %s", entry->overlay_params.class_path);
    log_trace("overlay_params.top_left_text: %s", entry->overlay_params.top_left_text);
    log_trace("overlay_params.top_right_bar_text: %s", entry->overlay_params.top_right_bar_text);
    log_trace("transition_in.type: %d", entry->transition_in.type);
    log_trace("transition_in.duration: %d", entry->transition_in.duration);
    log_trace("transition_in.background_color: %x", entry->transition_in.background_color);
    log_trace("transition_in.image_path: %s", entry->transition_in.image_path);
    log_trace("transition_loop.type: %d", entry->transition_loop.type);
    log_trace("transition_loop.duration: %d", entry->transition_loop.duration);
    log_trace("transition_loop.background_color: %x", entry->transition_loop.background_color);
    log_trace("transition_loop.image_path: %s", entry->transition_loop.image_path);
}
#endif // APP_RELEASE

