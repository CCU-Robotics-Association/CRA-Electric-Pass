#include "ui/cat_pet.h"

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#include <lvgl/lvgl.h>

#include "fonts.h"
#include "screens.h"

#define CAT_ANIMATION_PHASES 240
#define CAT_COMMON_STORY_COUNT 36
#define CAT_CONTEXT_STORY_COUNT 10
#define CAT_MOOD_COUNT 4

typedef struct {
    lv_obj_t *scene;
    lv_obj_t *cat_icon;
    lv_obj_t *shadow;
    lv_obj_t *heart;
    lv_obj_t *moon_glow;
    lv_obj_t *moon_disc;
    lv_obj_t *moon_mask;
    lv_obj_t *sun_glow;
    lv_obj_t *sun_icon;
    lv_obj_t *star_layer;
    lv_obj_t *hill_left;
    lv_obj_t *hill_right;
    lv_obj_t *ground;
    lv_obj_t *story_panel;
    lv_obj_t *mood_badge;
    lv_obj_t *mood_icon;
    lv_obj_t *story_label;
    lv_timer_t *animation_timer;
    uint16_t phase;
    uint16_t clock_ticks;
    uint16_t story_index;
    int8_t preview_time_mode;
    bool active;
    bool night_time;
} cat_pet_ui_t;

static cat_pet_ui_t cat;

static const char *const common_stories[CAT_COMMON_STORY_COUNT] = {
    "猫猫正在巡视它的小小世界",
    "风从窗边吹过，猫猫安静发呆",
    "它忽然停下来，好像正在看你",
    "猫猫把尾巴轻轻绕在身旁",
    "今天的风闻起来很温柔",
    "猫猫正在认真研究一粒灰尘",
    "它找到一个舒服的位置坐下",
    "猫猫眨了眨眼，又开始发呆",
    "一只小猫正在偷偷观察世界",
    "它听见远处传来细小的声音",
    "猫猫觉得今天适合慢慢度过",
    "它把爪子藏好，安静休息",
    "猫猫似乎想起了一件开心事",
    "它正在等待一阵熟悉的脚步声",
    "猫猫抬起头，看了看远方",
    "它轻轻晃着尾巴，心情不错",
    "猫猫决定暂时什么也不做",
    "它在自己的领地里散了一圈步",
    "猫猫发现今天也很值得期待",
    "它正努力保持一脸严肃",
    "猫猫悄悄靠近，又若无其事离开",
    "它在想下一次呼噜要持续多久",
    "猫猫把这里当成了秘密基地",
    "它对着空气认真地点了点头",
    "猫猫正在练习最标准的坐姿",
    "它好像听懂了，又好像没有",
    "猫猫安静陪你待在这里",
    "它把今天的小烦恼藏进尾巴",
    "猫猫刚刚完成一次小小巡逻",
    "它正在等一个温柔的摸摸",
    "猫猫决定把好运分给你一点",
    "它看起来很忙，其实只是在发呆",
    "猫猫眯起眼，享受片刻安静",
    "它的胡须捕捉到了一阵微风",
    "猫猫在心里轻轻说了一声你好",
    "它今天也在认真做一只猫"
};

static const char *const day_stories[CAT_CONTEXT_STORY_COUNT] = {
    "找到一块暖地，准备晒晒太阳",
    "阳光落在身上，猫猫暖洋洋的",
    "猫猫追着窗边移动的光斑",
    "白云慢慢飘过，猫猫抬头看了看",
    "今天的阳光刚好适合打个盹",
    "猫猫把影子留在了草地上",
    "风和太阳商量好，一起来陪猫猫",
    "猫猫在明亮的天空下伸了个懒腰",
    "一束阳光正好停在猫猫脚边",
    "猫猫决定把今天晒得蓬松一点"
};

static const char *const night_stories[CAT_CONTEXT_STORY_COUNT] = {
    "月光落在身边，猫猫安静赏月",
    "星星亮起来了，猫猫还没有困",
    "猫猫在夜色里听见风的声音",
    "月亮慢慢走，猫猫慢慢看",
    "今晚的天空藏着很多小秘密",
    "猫猫守着一小片安静的月光",
    "星光落在尾巴上，亮了一瞬",
    "猫猫抬头数星星，很快数乱了",
    "夜色很安静，呼噜声刚刚好",
    "月亮路过窗边，猫猫向它眨眼"
};

static const uint32_t scene_moods[CAT_MOOD_COUNT] = {
    0xF4DA, 0xF579, 0xF583, 0xF597
};

static void codepoint_to_utf8(uint32_t codepoint, char output[5])
{
    output[0] = (char)(0xE0U | (codepoint >> 12));
    output[1] = (char)(0x80U | ((codepoint >> 6) & 0x3FU));
    output[2] = (char)(0x80U | (codepoint & 0x3FU));
    output[3] = '\0';
}

static lv_obj_t *make_label(lv_obj_t *parent, const char *text,
                            const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);
    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(label, color, LV_PART_MAIN);
    return label;
}

static lv_obj_t *make_icon(lv_obj_t *parent, uint32_t codepoint,
                           lv_color_t color)
{
    char glyph[5];
    codepoint_to_utf8(codepoint, glyph);
    return make_label(parent, glyph, &ui_font_fontawesome, color);
}

static lv_obj_t *make_small_icon(lv_obj_t *parent, uint32_t codepoint,
                                 lv_color_t color)
{
    char glyph[5];
    codepoint_to_utf8(codepoint, glyph);
    return make_label(parent, glyph, &ui_font_fontawesome_small, color);
}

static void set_icon(lv_obj_t *label, uint32_t codepoint)
{
    char glyph[5];
    codepoint_to_utf8(codepoint, glyph);
    lv_label_set_text(label, glyph);
}

static void style_panel(lv_obj_t *obj, lv_color_t color, int radius)
{
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, color, LV_PART_MAIN);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(obj, radius, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
}

static lv_obj_t *make_circle(lv_obj_t *parent, int x, int y, int size,
                             lv_color_t color, lv_opa_t opacity)
{
    lv_obj_t *circle = lv_obj_create(parent);
    lv_obj_set_pos(circle, x, y);
    lv_obj_set_size(circle, size, size);
    style_panel(circle, color, LV_RADIUS_CIRCLE);
    lv_obj_set_style_bg_opa(circle, opacity, LV_PART_MAIN);
    return circle;
}

static void create_star(lv_obj_t *parent, int x, int y, int size,
                        lv_opa_t opacity)
{
    (void)make_circle(parent, x, y, size, lv_color_hex(0xF6F1DC), opacity);
}

static void update_scene_copy(void)
{
    const char *story;
    if ((cat.story_index % 4U) == 2U) {
        const unsigned int context_index =
            (cat.story_index / 4U) % CAT_CONTEXT_STORY_COUNT;
        story = cat.night_time ? night_stories[context_index]
                               : day_stories[context_index];
    }
    else {
        story = common_stories[cat.story_index % CAT_COMMON_STORY_COUNT];
    }
    lv_label_set_text(cat.story_label, story);
    set_icon(cat.mood_icon, scene_moods[cat.story_index % CAT_MOOD_COUNT]);
}

static void set_moon_hidden(bool hidden)
{
    lv_obj_t *parts[] = {cat.moon_glow, cat.moon_disc, cat.moon_mask};
    for (unsigned int i = 0; i < sizeof(parts) / sizeof(parts[0]); ++i) {
        if (hidden) {
            lv_obj_add_flag(parts[i], LV_OBJ_FLAG_HIDDEN);
        }
        else {
            lv_obj_remove_flag(parts[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void apply_day_night_palette(bool is_night)
{
    const lv_color_t sky = lv_color_hex(is_night ? 0x101D24 : 0x9BD5E1);
    const lv_color_t border = lv_color_hex(is_night ? 0x354750 : 0xCDEDF1);
    const lv_color_t hill_left = lv_color_hex(is_night ? 0x173039 : 0x6FAF9B);
    const lv_color_t hill_right = lv_color_hex(is_night ? 0x182B36 : 0x5A96A0);
    const lv_color_t ground = lv_color_hex(is_night ? 0x0C151A : 0x426F61);
    const lv_color_t panel = lv_color_hex(is_night ? 0x17242B : 0xEDF5F2);
    const lv_color_t panel_border =
        lv_color_hex(is_night ? 0x344750 : 0xB7D4CD);
    const lv_color_t story_foreground =
        lv_color_hex(is_night ? 0xF8FAFC : 0x182932);

    lv_obj_set_style_bg_color(cat.scene, sky, LV_PART_MAIN);
    lv_obj_set_style_border_color(cat.scene, border, LV_PART_MAIN);
    lv_obj_set_style_bg_color(cat.hill_left, hill_left, LV_PART_MAIN);
    lv_obj_set_style_bg_color(cat.hill_right, hill_right, LV_PART_MAIN);
    lv_obj_set_style_bg_color(cat.ground, ground, LV_PART_MAIN);
    lv_obj_set_style_bg_color(cat.story_panel, panel, LV_PART_MAIN);
    lv_obj_set_style_border_color(cat.story_panel, panel_border, LV_PART_MAIN);
    lv_obj_set_style_text_color(cat.cat_icon, lv_color_hex(0xFFFFFF),
                                LV_PART_MAIN);
    lv_obj_set_style_text_color(cat.story_label, story_foreground,
                                LV_PART_MAIN);
}

static void update_sky(void)
{
    const time_t now = time(NULL);
    struct tm local_time;
    if (now == (time_t)-1 || localtime_r(&now, &local_time) == NULL) {
        cat.night_time = true;
        set_moon_hidden(false);
        lv_obj_add_flag(cat.sun_glow, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(cat.sun_icon, LV_OBJ_FLAG_HIDDEN);
        return;
    }

    int minutes = local_time.tm_hour * 60 + local_time.tm_min;
    if (cat.preview_time_mode == 0) {
        minutes = 12 * 60;
    }
    else if (cat.preview_time_mode == 1) {
        minutes = 0;
    }
    const bool is_night = minutes >= 19 * 60 || minutes < 6 * 60;
    cat.night_time = is_night;
    apply_day_night_palette(is_night);

    if (!is_night) {
        set_moon_hidden(true);
        lv_obj_add_flag(cat.star_layer, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(cat.sun_glow, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(cat.sun_icon, LV_OBJ_FLAG_HIDDEN);

        /* 06:00 -> 19:00: left horizon, zenith, then right horizon. */
        const int day_minutes = minutes - 6 * 60;
        const int sun_x = 35 + (day_minutes * 210 / 780);
        int distance_from_noon = day_minutes - 390;
        if (distance_from_noon < 0) {
            distance_from_noon = -distance_from_noon;
        }
        const int sun_y = 20 + (distance_from_noon * 62 / 390);
        lv_obj_set_pos(cat.sun_glow, sun_x - 7, sun_y - 7);
        lv_obj_set_pos(cat.sun_icon, sun_x - 13, sun_y - 14);
        update_scene_copy();
        return;
    }
    set_moon_hidden(false);
    lv_obj_remove_flag(cat.star_layer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cat.sun_glow, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(cat.sun_icon, LV_OBJ_FLAG_HIDDEN);

    /* 19:00 -> 06:00: right horizon, zenith, then left horizon. */
    const int night_minutes = minutes >= 19 * 60
                                  ? minutes - 19 * 60
                                  : minutes + 5 * 60;
    const int moon_x = 245 - (night_minutes * 210 / 660);
    int distance_from_midnight = night_minutes - 330;
    if (distance_from_midnight < 0) {
        distance_from_midnight = -distance_from_midnight;
    }
    const int moon_y = 20 + (distance_from_midnight * 62 / 330);

    lv_obj_set_pos(cat.moon_glow, moon_x - 6, moon_y - 6);
    lv_obj_set_pos(cat.moon_disc, moon_x, moon_y);

    /* Approximate lunar phase from the 2000-01-06 18:14 UTC new moon. */
    const double days_since_new_moon =
        difftime(now, (time_t)947182440) / 86400.0;
    const double lunar_cycles = days_since_new_moon / 29.530588853;
    const double lunar_phase = lunar_cycles - (long long)lunar_cycles;
    const double illumination = lunar_phase <= 0.5
                                    ? lunar_phase * 2.0
                                    : (1.0 - lunar_phase) * 2.0;
    const int mask_offset = (int)(illumination * 49.0);
    const int mask_x = lunar_phase <= 0.5
                           ? moon_x + mask_offset
                           : moon_x - mask_offset;
    lv_obj_set_pos(cat.moon_mask, mask_x, moon_y);
    update_scene_copy();
}

static void update_animation(void)
{
    static const int8_t bob_table[12] = {
        0, -1, -2, -3, -4, -3, -2, -1, 0, 1, 2, 1
    };

    const int travel_phase = cat.phase % 120U;
    const int travel = travel_phase < 60 ? travel_phase : 120 - travel_phase;
    const int cat_x = 35 + (travel * 3 / 2);
    const int bob = bob_table[(cat.phase / 2U) % 12U];
    const int sway = ((cat.phase / 4U) % 9U) - 4;

    lv_obj_set_pos(cat.cat_icon, cat_x, 166 + bob);
    lv_obj_set_style_transform_rotation(cat.cat_icon, sway * 6, LV_PART_MAIN);

    lv_obj_set_pos(cat.shadow, cat_x + 37, 347 - (bob / 2));
    lv_obj_set_width(cat.shadow, 78 + (bob < 0 ? bob : -bob));

    const int heart_phase = cat.phase % 50U;
    lv_obj_set_pos(cat.heart, cat_x + 128, 183 - heart_phase);
    lv_obj_set_style_opa(cat.heart,
                         (lv_opa_t)(230 - (heart_phase * 4)), LV_PART_MAIN);
    lv_obj_set_style_transform_scale(cat.heart,
                                     105 + (heart_phase / 3), LV_PART_MAIN);

    cat.clock_ticks++;
    if (cat.clock_ticks >= 600U) {
        cat.clock_ticks = 0;
        update_sky();
    }
}

static void animation_timer_cb(lv_timer_t *timer)
{
    (void)timer;
    if (!cat.active) {
        return;
    }

    cat.phase = (uint16_t)((cat.phase + 1U) % CAT_ANIMATION_PHASES);
    if ((cat.phase % 60U) == 0U) {
        cat.story_index++;
        update_scene_copy();
    }
    update_animation();
}

static void screen_loaded_cb(lv_event_t *event)
{
    (void)event;
    cat.active = true;
    cat.phase = 0;
    cat.clock_ticks = 0;
    update_sky();
    update_scene_copy();
    update_animation();
    lv_timer_resume(cat.animation_timer);
}

static void screen_unloaded_cb(lv_event_t *event)
{
    (void)event;
    cat.active = false;
    lv_timer_pause(cat.animation_timer);
}

void cat_pet_init(void)
{
    lv_obj_t *container = objects.dispimg_container;
    if (container == NULL || objects.displayimg == NULL) {
        return;
    }

    cat.active = false;
    cat.phase = 0;
    cat.clock_ticks = 0;
    cat.story_index = 0;
    cat.preview_time_mode = -1;
    cat.night_time = true;

    lv_obj_clean(container);
    lv_obj_move_background(container);

    lv_obj_t *scene = lv_obj_create(container);
    cat.scene = scene;
    lv_obj_set_pos(scene, 18, 62);
    lv_obj_set_size(scene, 324, 570);
    style_panel(scene, lv_color_hex(0x101D24), 14);
    lv_obj_set_style_border_width(scene, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(scene, lv_color_hex(0x354750), LV_PART_MAIN);

    /* Night-sky decorations: static and deliberately cheap to render. */
    cat.moon_glow = make_circle(scene, 239, 16, 57,
                                lv_color_hex(0xF2E8BE), LV_OPA_20);
    cat.moon_disc = make_circle(scene, 245, 22, 45,
                                lv_color_hex(0xF2E8BE), LV_OPA_COVER);
    cat.moon_mask = make_circle(scene, 245, 22, 45,
                                lv_color_hex(0x101D24), LV_OPA_COVER);
    cat.sun_glow = make_circle(scene, 28, 75, 59,
                               lv_color_hex(0xFFD36B), LV_OPA_20);
    cat.sun_icon = make_icon(scene, 0xF185, lv_color_hex(0xFFD36B));
    lv_obj_set_style_transform_scale(cat.sun_icon, 180, LV_PART_MAIN);

    cat.star_layer = lv_obj_create(scene);
    lv_obj_set_pos(cat.star_layer, 0, 0);
    lv_obj_set_size(cat.star_layer, 324, 180);
    lv_obj_remove_flag(cat.star_layer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_opa(cat.star_layer, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(cat.star_layer, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(cat.star_layer, 0, LV_PART_MAIN);
    create_star(cat.star_layer, 219, 38, 5, LV_OPA_80);
    create_star(cat.star_layer, 292, 82, 4, LV_OPA_60);
    create_star(cat.star_layer, 193, 103, 3, LV_OPA_70);
    create_star(cat.star_layer, 276, 135, 5, LV_OPA_50);
    create_star(cat.star_layer, 43, 116, 4, LV_OPA_60);

    /* Soft hills and a floor line give the moving glyph a small stage. */
    cat.hill_left = make_circle(scene, -45, 308, 190,
                                lv_color_hex(0x173039), LV_OPA_COVER);
    cat.hill_right = make_circle(scene, 180, 325, 170,
                                 lv_color_hex(0x182B36), LV_OPA_COVER);

    lv_obj_t *ground = lv_obj_create(scene);
    cat.ground = ground;
    lv_obj_set_pos(ground, 0, 389);
    lv_obj_set_size(ground, 324, 181);
    style_panel(ground, lv_color_hex(0x0C151A), 0);
    lv_obj_set_style_border_width(ground, 2, LV_PART_MAIN);
    lv_obj_set_style_border_side(ground, LV_BORDER_SIDE_TOP, LV_PART_MAIN);
    lv_obj_set_style_border_color(ground, lv_color_hex(0x4B3B99),
                                  LV_PART_MAIN);

    cat.shadow = lv_obj_create(scene);
    lv_obj_set_pos(cat.shadow, 72, 347);
    lv_obj_set_size(cat.shadow, 78, 7);
    style_panel(cat.shadow, lv_color_hex(0x000000), LV_RADIUS_CIRCLE);
    lv_obj_set_style_bg_opa(cat.shadow, LV_OPA_20, LV_PART_MAIN);

    cat.cat_icon = make_icon(scene, 0xF6BE, lv_color_hex(0xF8FAFC));
    lv_obj_set_style_transform_scale(cat.cat_icon, 560, LV_PART_MAIN);
    lv_obj_set_pos(cat.cat_icon, 35, 166);

    cat.heart = make_icon(scene, 0xF004, lv_color_hex(0xEF4444));
    lv_obj_set_style_transform_scale(cat.heart, 105, LV_PART_MAIN);
    lv_obj_set_pos(cat.heart, 163, 183);

    lv_obj_t *story_panel = lv_obj_create(scene);
    cat.story_panel = story_panel;
    lv_obj_set_pos(story_panel, 16, 514);
    lv_obj_set_size(story_panel, 292, 44);
    style_panel(story_panel, lv_color_hex(0x17242B), 12);
    lv_obj_set_style_border_width(story_panel, 1, LV_PART_MAIN);
    lv_obj_set_style_border_color(story_panel, lv_color_hex(0x344750),
                                  LV_PART_MAIN);

    cat.mood_badge = lv_obj_create(story_panel);
    lv_obj_set_pos(cat.mood_badge, 5, 4);
    lv_obj_set_size(cat.mood_badge, 36, 36);
    style_panel(cat.mood_badge, lv_color_hex(0x4B3B99), LV_RADIUS_CIRCLE);

    cat.mood_icon = make_small_icon(cat.mood_badge, scene_moods[0],
                                    lv_color_hex(0xFFFFFF));
    lv_obj_center(cat.mood_icon);

    cat.story_label = make_label(story_panel, common_stories[0],
                                 &ui_font_sourcesans_reg_14,
                                 lv_color_hex(0xE8EEF2));
    lv_obj_set_pos(cat.story_label, 47, 13);
    lv_obj_set_width(cat.story_label, 236);
    lv_label_set_long_mode(cat.story_label, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(cat.story_label, LV_TEXT_ALIGN_CENTER,
                                LV_PART_MAIN);

    lv_obj_add_event_cb(objects.displayimg, screen_loaded_cb,
                        LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_add_event_cb(objects.displayimg, screen_unloaded_cb,
                        LV_EVENT_SCREEN_UNLOADED, NULL);

    cat.animation_timer = lv_timer_create(animation_timer_cb, 100, NULL);
    lv_timer_pause(cat.animation_timer);
}

void cat_pet_key_event(uint32_t key)
{
    if (key == 'd' || key == 'D') {
        cat.preview_time_mode = cat.night_time ? 0 : 1;
        update_sky();
    }
    else if (key == 'a' || key == 'A') {
        cat.preview_time_mode = -1;
        update_sky();
    }
}

bool cat_pet_is_active(void)
{
    return cat.active;
}
