#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include <lvgl/lvgl.h>
#include "lvgl/src/drivers/sdl/lv_sdl_keyboard.h"
#include "lvgl/src/drivers/sdl/lv_sdl_mouse.h"
#include "lvgl/src/drivers/sdl/lv_sdl_window.h"

#include "fonts.h"
#include "screens.h"
#include "ui.h"
#include "ui/cat_pet.h"
#include "ui/theme.h"

#define DISPLAY_WIDTH  360
#define DISPLAY_HEIGHT 640

static const uint32_t icon_codepoints[] = {
    0xF7A9, 0xF794, 0xF013, 0xF007, 0xF3CF, 0xF15B,
    0xF240, 0xF241, 0xF242, 0xF243, 0xF244, 0xF071,
    0xF0E7, 0xF3E0, 0xF187, 0xF302, 0xF185, 0xF6BE,
    0xF601, 0xF00D, 0xF00C, 0x003F, 0xF132, 0xF5E1,
    0xF850, 0xF165, 0xF164, 0xF134, 0xF14C, 0xF004,
    0xF583, 0xF4DA, 0xF597, 0xF770, 0xE13C,
    0xF579, 0xF5B3, 0xF7C2
};

static lv_obj_t *icon_gallery_screen;
static bool icon_gallery_active;

static void codepoint_to_utf8(uint32_t codepoint, char output[5])
{
    if (codepoint <= 0x7FU) {
        output[0] = (char)codepoint;
        output[1] = '\0';
    }
    else if (codepoint <= 0x7FFU) {
        output[0] = (char)(0xC0U | (codepoint >> 6));
        output[1] = (char)(0x80U | (codepoint & 0x3FU));
        output[2] = '\0';
    }
    else {
        output[0] = (char)(0xE0U | (codepoint >> 12));
        output[1] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
        output[2] = (char)(0x80U | (codepoint & 0x3FU));
        output[3] = '\0';
    }
}

static lv_obj_t *create_text_label(lv_obj_t *parent, const char *text,
                                    const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
    return label;
}

static void create_icon_gallery(void)
{
    icon_gallery_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(icon_gallery_screen, lv_color_hex(0x0D1217), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(icon_gallery_screen, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_remove_flag(icon_gallery_screen, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = create_text_label(icon_gallery_screen,
                                        "图标总览（38）",
                                        &ui_font_sourceselif_heavy_24,
                                        lv_color_hex(0xF5F7FA));
    lv_obj_set_pos(title, 10, 7);

    lv_obj_t *hint = create_text_label(icon_gallery_screen,
                                       "滚轮浏览 · Esc 返回 · I 再次打开",
                                       &ui_font_sourcesans_reg_14,
                                       lv_color_hex(0xAAB8C5));
    lv_obj_set_pos(hint, 10, 39);

    lv_obj_t *grid = lv_obj_create(icon_gallery_screen);
    lv_obj_set_pos(grid, 7, 65);
    lv_obj_set_size(grid, 346, 568);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_flex_align(grid, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START,
                         LV_FLEX_ALIGN_START);
    lv_obj_set_scroll_dir(grid, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_bg_color(grid, lv_color_hex(0x111820), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(grid, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(grid, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(grid, 8, LV_PART_MAIN);
    lv_obj_set_style_pad_all(grid, 7, LV_PART_MAIN);
    lv_obj_set_style_pad_row(grid, 7, LV_PART_MAIN);
    lv_obj_set_style_pad_column(grid, 7, LV_PART_MAIN);

    for (size_t i = 0; i < sizeof(icon_codepoints) / sizeof(icon_codepoints[0]); ++i) {
        lv_obj_t *card = lv_obj_create(grid);
        lv_obj_set_size(card, 102, 94);
        lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, LV_PART_MAIN);
        lv_obj_set_style_border_width(card, 0, LV_PART_MAIN);
        lv_obj_set_style_radius(card, 8, LV_PART_MAIN);
        lv_obj_set_style_pad_all(card, 0, LV_PART_MAIN);

        char glyph[5];
        codepoint_to_utf8(icon_codepoints[i], glyph);
        lv_obj_t *icon = create_text_label(card, glyph, &ui_font_fontawesome,
                                           lv_color_hex(0x1F2933));
        lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 2);

        char code_label[16];
        (void)snprintf(code_label, sizeof(code_label), "U+%04lX",
                       (unsigned long)icon_codepoints[i]);
        lv_obj_t *code = create_text_label(card, code_label,
                                           &ui_font_sourcesans_reg_14,
                                           lv_color_hex(0x34495E));
        lv_obj_align(code, LV_ALIGN_BOTTOM_MID, 0, 0);
    }
}

static void show_icon_gallery(void)
{
    if (icon_gallery_screen == NULL) {
        create_icon_gallery();
    }

    icon_gallery_active = true;
    lv_screen_load_anim(icon_gallery_screen, LV_SCR_LOAD_ANIM_FADE_IN, 150, 0, false);
}

static void keyboard_event_cb(lv_event_t *event)
{
    if (lv_event_get_code(event) != LV_EVENT_KEY) {
        return;
    }

    lv_indev_t *indev = lv_event_get_target(event);
    if (lv_indev_get_state(indev) != LV_INDEV_STATE_PRESSED) {
        return;
    }

    uint32_t key = lv_indev_get_key(indev);
    if (cat_pet_is_active()) {
        if (key == LV_KEY_ESC || key == LV_KEY_BACKSPACE) {
            loadScreen(SCREEN_ID_MAINMENU);
        }
        else {
            cat_pet_key_event(key);
        }
        return;
    }

    if (key == LV_KEY_ESC || key == LV_KEY_BACKSPACE) {
        icon_gallery_active = false;
        loadScreen(SCREEN_ID_MAINMENU);
    }
    else if ((key == 'i' || key == 'I') && !icon_gallery_active) {
        show_icon_gallery();
    }
}

int main(void)
{
    lv_init();

    lv_display_t *display = lv_sdl_window_create(DISPLAY_WIDTH, DISPLAY_HEIGHT);
    if (display == NULL) {
        return 1;
    }

    lv_sdl_window_set_title(display, "CRA Electric Pass - UI Simulator");
    lv_sdl_window_set_resizeable(display, false);

    lv_sdl_mouse_create();
    lv_indev_t *keyboard = lv_sdl_keyboard_create();

    ui_create_groups();
    if (keyboard != NULL) {
        lv_indev_set_group(keyboard, groups.navigation);
        lv_indev_add_event_cb(keyboard, keyboard_event_cb, LV_EVENT_KEY, NULL);
    }

    ui_init();
    cat_pet_init();
    ui_theme_apply(get_var_theme_color());

    for (;;) {
        ui_tick();
        uint32_t wait_ms = lv_timer_handler();
        if (wait_ms < 1U) {
            wait_ms = 1U;
        }
        else if (wait_ms > 16U) {
            wait_ms = 16U;
        }
        lv_delay_ms(wait_ms);
    }
}
