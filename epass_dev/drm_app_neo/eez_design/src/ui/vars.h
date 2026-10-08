#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations

typedef enum {
    sw_mode_t_SW_MODE_SEQUENCE = 0,
    sw_mode_t_SW_MODE_RANDOM = 1,
    sw_mode_t_SW_MODE_MANUAL = 2
} sw_mode_t;

typedef enum {
    sw_interval_t_SW_INTERVAL_1MIN = 0,
    sw_interval_t_SW_INTERVAL_3MIN = 1,
    sw_interval_t_SW_INTERVAL_5MIN = 2,
    sw_interval_t_SW_INTERVAL_10MIN = 3,
    sw_interval_t_SW_INTERVAL_30MIN = 4
} sw_interval_t;

typedef enum {
    sleep_timeout_t_SLEEP_TIMEOUT_30SEC = 0,
    sleep_timeout_t_SLEEP_TIMEOUT_1MIN = 1,
    sleep_timeout_t_SLEEP_TIMEOUT_3MIN = 2,
    sleep_timeout_t_SLEEP_TIMEOUT_NEVER = 3
} sleep_timeout_t;

typedef enum {
    theme_color_t_THEME_WHITE = 0,
    theme_color_t_THEME_PURPLE = 1,
    theme_color_t_THEME_BLUE = 2,
    theme_color_t_THEME_DARK_RED = 3,
    theme_color_t_THEME_KLEIN_BLUE = 4,
    theme_color_t_THEME_TIFFANY_BLUE = 5,
    theme_color_t_THEME_EMERALD = 6,
    theme_color_t_THEME_CHINA_RED = 7,
    theme_color_t_THEME_GRAPHITE = 8,
    theme_color_t_THEME_SKY_BLUE = 9,
    theme_color_t_THEME_AMBER_YELLOW = 10,
    theme_color_t_THEME_LEMON_YELLOW = 11,
    theme_color_t_THEME_CHAMPAGNE_GOLD = 12
} theme_color_t;

typedef enum {
    curr_screen_t_SCREEN_MAINMENU = 0,
    curr_screen_t_SCREEN_THEME_LIST = 1,
    curr_screen_t_SCREEN_SYSINFO = 2,
    curr_screen_t_SCREEN_SPINNER = 3,
    curr_screen_t_SCREEN_DISPLAYIMG = 4,
    curr_screen_t_SCREEN_FILEMANAGER = 5,
    curr_screen_t_SCREEN_SETTINGS = 6,
    curr_screen_t_SCREEN_WARNING = 7,
    curr_screen_t_SCREEN_CONFIRM = 8,
    curr_screen_t_SCREEN_APPLIST = 9
} curr_screen_t;

typedef enum {
    usb_mode_t_MTP = 0,
    usb_mode_t_SERIAL = 1,
    usb_mode_t_RNDIS = 2,
    usb_mode_t_NONE = 3,
    usb_mode_t_EPMANAGER = 4
} usb_mode_t;

// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_NAND_LABEL = 0,
    FLOW_GLOBAL_VARIABLE_SD_LABEL = 1,
    FLOW_GLOBAL_VARIABLE_NAND_PERCENT = 2,
    FLOW_GLOBAL_VARIABLE_SD_PERCENT = 3,
    FLOW_GLOBAL_VARIABLE_DISPLAYIMG_SIZE_LBL = 4,
    FLOW_GLOBAL_VARIABLE_USB_MODE = 5,
    FLOW_GLOBAL_VARIABLE_WARNING_TITLE = 6,
    FLOW_GLOBAL_VARIABLE_WARNING_DESC = 7,
    FLOW_GLOBAL_VARIABLE_DISPIMG_SHOW_WARNING = 8,
    FLOW_GLOBAL_VARIABLE_DISPIMG_PATH = 9,
    FLOW_GLOBAL_VARIABLE_CONFIRM_TITLE = 10,
    FLOW_GLOBAL_VARIABLE_WARNING_ICON = 11,
    FLOW_GLOBAL_VARIABLE_APPLIST_SHOW_WARNING = 12
};

// Native global variables

extern const char *get_var_epass_version();
extern void set_var_epass_version(const char *value);
extern const char *get_var_sysinfo();
extern void set_var_sysinfo(const char *value);
extern sw_mode_t get_var_sw_mode();
extern void set_var_sw_mode(sw_mode_t value);
extern sw_interval_t get_var_sw_interval();
extern void set_var_sw_interval(sw_interval_t value);
extern sleep_timeout_t get_var_sleep_timeout();
extern void set_var_sleep_timeout(sleep_timeout_t value);
extern theme_color_t get_var_theme_color();
extern void set_var_theme_color(theme_color_t value);
extern int32_t get_var_brightness();
extern void set_var_brightness(int32_t value);
extern const char *get_var_nand_label();
extern void set_var_nand_label(const char *value);
extern const char *get_var_sd_label();
extern void set_var_sd_label(const char *value);
extern int32_t get_var_nand_percent();
extern void set_var_nand_percent(int32_t value);
extern int32_t get_var_sd_percent();
extern void set_var_sd_percent(int32_t value);
extern const char *get_var_displayimg_size_lbl();
extern void set_var_displayimg_size_lbl(const char *value);
extern usb_mode_t get_var_usb_mode();
extern void set_var_usb_mode(usb_mode_t value);
extern const char *get_var_warning_title();
extern void set_var_warning_title(const char *value);
extern const char *get_var_warning_desc();
extern void set_var_warning_desc(const char *value);
extern bool get_var_dispimg_show_warning();
extern void set_var_dispimg_show_warning(bool value);
extern const char *get_var_dispimg_path();
extern void set_var_dispimg_path(const char *value);
extern const char *get_var_confirm_title();
extern void set_var_confirm_title(const char *value);
extern const char *get_var_warning_icon();
extern void set_var_warning_icon(const char *value);
extern bool get_var_applist_show_warning();
extern void set_var_applist_show_warning(bool value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/
