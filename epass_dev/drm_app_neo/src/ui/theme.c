#include "ui/theme.h"

#include <stddef.h>

#include "screens.h"
#include "styles.h"

typedef struct {
    uint32_t button_background;
    uint32_t button_focused;
    uint32_t button_foreground;
    uint32_t accent;
} ui_theme_palette_t;

static ui_theme_palette_t get_palette(theme_color_t theme)
{
    switch(theme) {
        case theme_color_t_THEME_PURPLE:
            return (ui_theme_palette_t) {
                .button_background = 0x4B3B99,
                .button_focused = 0x6B57C9,
                .button_foreground = 0xFFFFFF,
                .accent = 0x6B57C9
            };
        case theme_color_t_THEME_BLUE:
            return (ui_theme_palette_t) {
                .button_background = 0x20679F,
                .button_focused = 0x398ED0,
                .button_foreground = 0xFFFFFF,
                .accent = 0x398ED0
            };
        case theme_color_t_THEME_DARK_RED:
            return (ui_theme_palette_t) {
                .button_background = 0x3F1A1E,
                .button_focused = 0x6A3038,
                .button_foreground = 0xFFFFFF,
                .accent = 0x6A3038
            };
        case theme_color_t_THEME_KLEIN_BLUE:
            return (ui_theme_palette_t) {
                .button_background = 0x002FA7,
                .button_focused = 0x174BC2,
                .button_foreground = 0xFFFFFF,
                .accent = 0x002FA7
            };
        case theme_color_t_THEME_TIFFANY_BLUE:
            return (ui_theme_palette_t) {
                .button_background = 0x81D8D0,
                .button_focused = 0x5FC7BE,
                .button_foreground = 0x102A2E,
                .accent = 0x0E9F94
            };
        case theme_color_t_THEME_EMERALD:
            return (ui_theme_palette_t) {
                .button_background = 0x009473,
                .button_focused = 0x00B386,
                .button_foreground = 0xFFFFFF,
                .accent = 0x009473
            };
        case theme_color_t_THEME_CHINA_RED:
            return (ui_theme_palette_t) {
                .button_background = 0xDE2910,
                .button_focused = 0xF0442D,
                .button_foreground = 0xFFFFFF,
                .accent = 0xDE2910
            };
        case theme_color_t_THEME_GRAPHITE:
            return (ui_theme_palette_t) {
                .button_background = 0x383E42,
                .button_focused = 0x565E63,
                .button_foreground = 0xFFFFFF,
                .accent = 0x69747B
            };
        case theme_color_t_THEME_SKY_BLUE:
            return (ui_theme_palette_t) {
                .button_background = 0x87CEEB,
                .button_focused = 0x5FBDD9,
                .button_foreground = 0x102A43,
                .accent = 0x2A92B8
            };
        case theme_color_t_THEME_AMBER_YELLOW:
            return (ui_theme_palette_t) {
                .button_background = 0xFFBF00,
                .button_focused = 0xE0A800,
                .button_foreground = 0x332500,
                .accent = 0xC58B00
            };
        case theme_color_t_THEME_LEMON_YELLOW:
            return (ui_theme_palette_t) {
                .button_background = 0xF4E04D,
                .button_focused = 0xD8C62E,
                .button_foreground = 0x302B00,
                .accent = 0xB89F00
            };
        case theme_color_t_THEME_CHAMPAGNE_GOLD:
            return (ui_theme_palette_t) {
                .button_background = 0xD6B65B,
                .button_focused = 0xBFA04A,
                .button_foreground = 0x2B230D,
                .accent = 0xA98520
            };
        case theme_color_t_THEME_WHITE:
        default:
            return (ui_theme_palette_t) {
                .button_background = 0xFFFFFF,
                .button_focused = 0xE9EEF2,
                .button_foreground = 0x1F2933,
                .accent = 0x4B3B99
            };
    }
}

static void set_button_text_color(lv_obj_t *button, lv_color_t color)
{
    uint32_t child_count = lv_obj_get_child_count(button);
    for(uint32_t i = 0; i < child_count; ++i) {
        lv_obj_t *child = lv_obj_get_child(button, (int32_t)i);
        lv_obj_set_style_text_color(child, color,
                                    LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(child, color,
                                    LV_PART_MAIN | LV_STATE_FOCUSED);
    }
}

static void set_dropdown_focus_color(lv_obj_t *dropdown, lv_color_t accent)
{
    lv_obj_set_style_border_color(dropdown, accent,
                                  LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_outline_color(dropdown, accent,
                                   LV_PART_MAIN | LV_STATE_FOCUSED);
}

void ui_theme_apply(theme_color_t theme)
{
    ui_theme_palette_t palette = get_palette(theme);
    lv_color_t background = lv_color_hex(palette.button_background);
    lv_color_t focused = lv_color_hex(palette.button_focused);
    lv_color_t foreground = lv_color_hex(palette.button_foreground);
    lv_color_t accent = lv_color_hex(palette.accent);

    lv_style_set_bg_color(get_style_main_btn_MAIN_DEFAULT(), background);
    lv_style_set_bg_color(get_style_main_btn_MAIN_FOCUSED(), focused);
    lv_style_set_bg_color(get_style_theme_entry_button_MAIN_FOCUSED(), accent);
    lv_style_set_bg_color(get_style_sd_flag_MAIN_DEFAULT(), accent);

    lv_obj_t *main_buttons[] = {
        objects.theme_list_btn,
        objects.dispimg_btn,
        objects.apps_btn,
        objects.file_btn,
        objects.sett_btn,
        objects.dev_btn
    };
    for(size_t i = 0; i < sizeof(main_buttons) / sizeof(main_buttons[0]); ++i) {
        set_button_text_color(main_buttons[i], foreground);
    }

    lv_obj_set_style_bg_color(objects.brightness_scroller, accent,
                              LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.brightness_scroller, accent,
                              LV_PART_KNOB | LV_STATE_DEFAULT);

    lv_obj_t *dropdowns[] = {
        objects.sleep_timeout_dropdown,
        objects.theme_color_dropdown,
        objects.swmode_dropdown,
        objects.swint_dropdown,
        objects.usbmode_dropdown
    };
    for(size_t i = 0; i < sizeof(dropdowns) / sizeof(dropdowns[0]); ++i) {
        set_dropdown_focus_color(dropdowns[i], accent);
    }

    const lv_color_t power_background = lv_color_hex(0x3F1A1E);
    lv_obj_set_style_bg_color(objects.restart_app_btn, power_background,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.restart_app_btn, power_background,
                              LV_PART_MAIN | LV_STATE_FOCUSED);
    lv_obj_set_style_bg_color(objects.shutdown_btn, power_background,
                              LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(objects.shutdown_btn, power_background,
                              LV_PART_MAIN | LV_STATE_FOCUSED);

    lv_obj_report_style_change(NULL);
}
