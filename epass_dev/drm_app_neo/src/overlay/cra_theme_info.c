#include "overlay/theme_info.h"

#include "config.h"
#include "driver/drm_warpper.h"
#include "render/fbdraw.h"
#include "utils/log.h"
#include "utils/timer.h"

#include <src/misc/lv_text_private.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

LV_FONT_DECLARE(ui_font_sourcesans_reg_14);
LV_FONT_DECLARE(ui_font_sourceselif_heavy_24);

#define CRA_TOP_PANEL_X 16
#define CRA_TOP_PANEL_Y 16
#define CRA_TOP_PANEL_W 328
#define CRA_TOP_PANEL_H 94

#define CRA_LOGO_X 28
#define CRA_LOGO_Y 27
#define CRA_LOGO_MAX_W 70
#define CRA_LOGO_MAX_H 70

#define CRA_HEADER_X 112
#define CRA_ORG_Y 32
#define CRA_HEADER_Y 60
#define CRA_TOP_LINE_Y 90
#define CRA_TOP_LINE_MAX_W 214

#define CRA_INFO_PANEL_X 16
#define CRA_INFO_PANEL_Y 392
#define CRA_INFO_PANEL_W 328
#define CRA_INFO_PANEL_H 230

#define CRA_TEXT_X 40
#define CRA_MEMBER_NAME_Y 412
#define CRA_ROLE_Y 451
#define CRA_MEMBER_CODE_Y 477
#define CRA_DIVIDER_Y 506
#define CRA_DIVIDER_MAX_W 280
#define CRA_DETAIL_Y 523
#define CRA_FOOTER_Y 596

#define CRA_SCAN_RAIL_X 335
#define CRA_SCAN_RAIL_Y 400
#define CRA_SCAN_RAIL_W 3
#define CRA_SCAN_RAIL_H 214
#define CRA_SCAN_MARK_H 24

#define CRA_PANEL_COLOR 0xB010151Bu
#define CRA_PANEL_EDGE_COLOR 0x7048535Du
#define CRA_TEXT_COLOR 0xFFFFFFFFu
#define CRA_MUTED_TEXT_COLOR 0xFFD4D7DBu

#define CRA_ORG_START_FRAME 8
#define CRA_HEADER_START_FRAME 18
#define CRA_NAME_START_FRAME 32
#define CRA_ROLE_START_FRAME 48
#define CRA_CODE_START_FRAME 58
#define CRA_DETAIL_START_FRAME 68
#define CRA_FOOTER_START_FRAME 96
#define CRA_LINE_START_FRAME 20
#define CRA_LINE_FRAME_COUNT 36
#define CRA_DIVIDER_START_FRAME 52
#define CRA_DIVIDER_FRAME_COUNT 38
#define CRA_LOGO_FADE_START_FRAME 10
#define CRA_LOGO_FADE_STEP 8
#define CRA_SCAN_START_FRAME 90

typedef struct {
    unsigned int organization_text : 1;
    unsigned int header_text : 1;
    unsigned int member_name : 1;
    unsigned int role_text : 1;
    unsigned int member_code : 1;
    unsigned int detail_text : 1;
    unsigned int footer_text : 1;
    unsigned int logo_fade : 1;
    unsigned int top_line : 1;
    unsigned int divider : 1;
    unsigned int scan : 1;
} cra_overlay_update_t;

typedef struct {
    overlay_t* overlay;
    theme_overlay_params_t* params;
    int curr_frame;
    int curr_buffer;

    int organization_cpidx;
    int organization_cpcnt;
    int header_cpidx;
    int header_cpcnt;
    int member_name_cpidx;
    int member_name_cpcnt;
    int role_cpidx;
    int role_cpcnt;
    int member_code_cpidx;
    int member_code_cpcnt;
    int detail_cpidx;
    int detail_cpcnt;
    int footer_cpidx;
    int footer_cpcnt;

    int logo_opacity;
    int top_line_width;
    int divider_width;
    int scan_y[2];

    cra_overlay_update_t buf1_update;
    cra_overlay_update_t buf2_update;
} cra_overlay_worker_data_t;

static uint32_t color_with_alpha(uint32_t color, uint8_t alpha) {
    return (color & 0x00FFFFFFu) | ((uint32_t)alpha << 24);
}

static bool advance_typewriter(
    int curr_frame,
    int start_frame,
    int frame_per_codepoint,
    int* cpidx,
    int cpcnt
) {
    if(curr_frame < start_frame || *cpidx >= cpcnt) {
        return false;
    }
    if(((curr_frame - start_frame) % frame_per_codepoint) != 0) {
        return false;
    }

    (*cpidx)++;
    return true;
}

static void init_template_cra_overlay(uint32_t* vaddr, theme_overlay_params_t* params) {
    memset(vaddr, 0, OVERLAY_WIDTH * OVERLAY_HEIGHT * sizeof(uint32_t));

    fbdraw_fb_t fb = {
        .vaddr = vaddr,
        .width = OVERLAY_WIDTH,
        .height = OVERLAY_HEIGHT,
    };
    fbdraw_rect_t rect;
    const uint32_t accent = params->cra.color;

    rect = (fbdraw_rect_t){
        CRA_TOP_PANEL_X,
        CRA_TOP_PANEL_Y,
        CRA_TOP_PANEL_W,
        CRA_TOP_PANEL_H,
    };
    fbdraw_fill_rect(&fb, &rect, CRA_PANEL_COLOR);

    rect = (fbdraw_rect_t){
        CRA_TOP_PANEL_X,
        CRA_TOP_PANEL_Y,
        3,
        CRA_TOP_PANEL_H,
    };
    fbdraw_fill_rect(&fb, &rect, accent);

    rect = (fbdraw_rect_t){
        CRA_INFO_PANEL_X,
        CRA_INFO_PANEL_Y,
        CRA_INFO_PANEL_W,
        CRA_INFO_PANEL_H,
    };
    fbdraw_fill_rect(&fb, &rect, CRA_PANEL_COLOR);

    rect = (fbdraw_rect_t){
        CRA_INFO_PANEL_X,
        CRA_INFO_PANEL_Y,
        CRA_INFO_PANEL_W,
        1,
    };
    fbdraw_fill_rect(&fb, &rect, CRA_PANEL_EDGE_COLOR);

    rect = (fbdraw_rect_t){
        CRA_INFO_PANEL_X,
        CRA_INFO_PANEL_Y + CRA_INFO_PANEL_H - 1,
        CRA_INFO_PANEL_W,
        1,
    };
    fbdraw_fill_rect(&fb, &rect, CRA_PANEL_EDGE_COLOR);

    rect = (fbdraw_rect_t){
        CRA_INFO_PANEL_X + 8,
        CRA_MEMBER_NAME_Y,
        4,
        176,
    };
    fbdraw_fill_rect(&fb, &rect, color_with_alpha(accent, 224));

    rect = (fbdraw_rect_t){
        CRA_SCAN_RAIL_X,
        CRA_SCAN_RAIL_Y,
        CRA_SCAN_RAIL_W,
        CRA_SCAN_RAIL_H,
    };
    fbdraw_fill_rect(&fb, &rect, color_with_alpha(accent, 64));
}

static void draw_logo(
    fbdraw_fb_t* dst,
    const cra_overlay_options_t* cra,
    int opacity
) {
    if(!cra->logo_addr || cra->logo_w <= 0 || cra->logo_h <= 0 || opacity <= 0) {
        return;
    }

    const int width = cra->logo_w < CRA_LOGO_MAX_W ? cra->logo_w : CRA_LOGO_MAX_W;
    const int height = cra->logo_h < CRA_LOGO_MAX_H ? cra->logo_h : CRA_LOGO_MAX_H;
    fbdraw_fb_t src = {
        .vaddr = cra->logo_addr,
        .width = cra->logo_w,
        .height = cra->logo_h,
    };
    fbdraw_rect_t src_rect = {0, 0, width, height};
    fbdraw_rect_t dst_rect = {CRA_LOGO_X, CRA_LOGO_Y, width, height};

    fbdraw_alpha_opacity_rect(
        &src,
        dst,
        &src_rect,
        &dst_rect,
        opacity > 255 ? 255 : (uint8_t)opacity
    );
}

static void cra_overlay_worker(void* userdata, int skipped_frames) {
    cra_overlay_worker_data_t* data = (cra_overlay_worker_data_t*)userdata;

    if(data->overlay->request_abort) {
        app_timer_cancel(data->overlay->overlay_timer_handle);
        data->overlay->overlay_timer_handle = 0;
        log_debug("CRA overlay worker: request abort");
        return;
    }

    data->curr_frame += skipped_frames;

    if(advance_typewriter(
        data->curr_frame,
        CRA_ORG_START_FRAME,
        2,
        &data->organization_cpidx,
        data->organization_cpcnt
    )) {
        data->buf1_update.organization_text = 1;
        data->buf2_update.organization_text = 1;
    }
    if(advance_typewriter(
        data->curr_frame,
        CRA_HEADER_START_FRAME,
        2,
        &data->header_cpidx,
        data->header_cpcnt
    )) {
        data->buf1_update.header_text = 1;
        data->buf2_update.header_text = 1;
    }
    if(advance_typewriter(
        data->curr_frame,
        CRA_NAME_START_FRAME,
        3,
        &data->member_name_cpidx,
        data->member_name_cpcnt
    )) {
        data->buf1_update.member_name = 1;
        data->buf2_update.member_name = 1;
    }
    if(advance_typewriter(
        data->curr_frame,
        CRA_ROLE_START_FRAME,
        2,
        &data->role_cpidx,
        data->role_cpcnt
    )) {
        data->buf1_update.role_text = 1;
        data->buf2_update.role_text = 1;
    }
    if(advance_typewriter(
        data->curr_frame,
        CRA_CODE_START_FRAME,
        2,
        &data->member_code_cpidx,
        data->member_code_cpcnt
    )) {
        data->buf1_update.member_code = 1;
        data->buf2_update.member_code = 1;
    }
    if(advance_typewriter(
        data->curr_frame,
        CRA_DETAIL_START_FRAME,
        2,
        &data->detail_cpidx,
        data->detail_cpcnt
    )) {
        data->buf1_update.detail_text = 1;
        data->buf2_update.detail_text = 1;
    }
    if(advance_typewriter(
        data->curr_frame,
        CRA_FOOTER_START_FRAME,
        2,
        &data->footer_cpidx,
        data->footer_cpcnt
    )) {
        data->buf1_update.footer_text = 1;
        data->buf2_update.footer_text = 1;
    }

    if(data->curr_frame >= CRA_LOGO_FADE_START_FRAME && data->logo_opacity < 255) {
        data->logo_opacity += CRA_LOGO_FADE_STEP;
        if(data->logo_opacity > 255) {
            data->logo_opacity = 255;
        }
        data->buf1_update.logo_fade = 1;
        data->buf2_update.logo_fade = 1;
    }

    if(data->curr_frame >= CRA_LINE_START_FRAME &&
       data->top_line_width < CRA_TOP_LINE_MAX_W) {
        int frame = data->curr_frame - CRA_LINE_START_FRAME + 1;
        if(frame > CRA_LINE_FRAME_COUNT) {
            frame = CRA_LINE_FRAME_COUNT;
        }
        data->top_line_width =
            CRA_TOP_LINE_MAX_W * frame / CRA_LINE_FRAME_COUNT;
        data->buf1_update.top_line = 1;
        data->buf2_update.top_line = 1;
    }

    if(data->curr_frame >= CRA_DIVIDER_START_FRAME &&
       data->divider_width < CRA_DIVIDER_MAX_W) {
        int frame = data->curr_frame - CRA_DIVIDER_START_FRAME + 1;
        if(frame > CRA_DIVIDER_FRAME_COUNT) {
            frame = CRA_DIVIDER_FRAME_COUNT;
        }
        data->divider_width =
            CRA_DIVIDER_MAX_W * frame / CRA_DIVIDER_FRAME_COUNT;
        data->buf1_update.divider = 1;
        data->buf2_update.divider = 1;
    }

    if(data->curr_frame >= CRA_SCAN_START_FRAME) {
        data->buf1_update.scan = 1;
        data->buf2_update.scan = 1;
    }

    cra_overlay_update_t* update =
        data->curr_buffer == 0 ? &data->buf1_update : &data->buf2_update;
    drm_warpper_queue_item_t* item;
    drm_warpper_dequeue_free_item(
        data->overlay->drm_warpper,
        DRM_WARPPER_LAYER_OVERLAY,
        &item
    );

    fbdraw_fb_t fb = {
        .vaddr = (uint32_t*)item->mount.arg0,
        .width = OVERLAY_WIDTH,
        .height = OVERLAY_HEIGHT,
    };
    fbdraw_rect_t rect;
    const cra_overlay_options_t* cra = &data->params->cra;
    const uint32_t accent = cra->color;

    if(update->organization_text) {
        rect = (fbdraw_rect_t){CRA_HEADER_X, CRA_ORG_Y, 218, 20};
        fbdraw_text_range(
            &fb,
            &rect,
            cra->organization_text,
            &ui_font_sourcesans_reg_14,
            CRA_TEXT_COLOR,
            0,
            0,
            data->organization_cpidx + 1
        );
        update->organization_text = 0;
    }

    if(update->header_text) {
        rect = (fbdraw_rect_t){CRA_HEADER_X, CRA_HEADER_Y, 218, 20};
        fbdraw_text_range(
            &fb,
            &rect,
            cra->header_text,
            &ui_font_sourcesans_reg_14,
            color_with_alpha(accent, 255),
            0,
            0,
            data->header_cpidx + 1
        );
        update->header_text = 0;
    }

    if(update->member_name) {
        rect = (fbdraw_rect_t){CRA_TEXT_X, CRA_MEMBER_NAME_Y, 286, 32};
        fbdraw_text_range(
            &fb,
            &rect,
            cra->member_name,
            &ui_font_sourceselif_heavy_24,
            CRA_TEXT_COLOR,
            0,
            0,
            data->member_name_cpidx + 1
        );
        update->member_name = 0;
    }

    if(update->role_text) {
        rect = (fbdraw_rect_t){CRA_TEXT_X, CRA_ROLE_Y, 286, 20};
        fbdraw_text_range(
            &fb,
            &rect,
            cra->role_text,
            &ui_font_sourcesans_reg_14,
            color_with_alpha(accent, 255),
            0,
            0,
            data->role_cpidx + 1
        );
        update->role_text = 0;
    }

    if(update->member_code) {
        rect = (fbdraw_rect_t){CRA_TEXT_X, CRA_MEMBER_CODE_Y, 286, 20};
        fbdraw_text_range(
            &fb,
            &rect,
            cra->member_code,
            &ui_font_sourcesans_reg_14,
            CRA_MUTED_TEXT_COLOR,
            0,
            0,
            data->member_code_cpidx + 1
        );
        update->member_code = 0;
    }

    if(update->detail_text) {
        rect = (fbdraw_rect_t){CRA_TEXT_X, CRA_DETAIL_Y, 282, 66};
        fbdraw_text_range(
            &fb,
            &rect,
            cra->detail_text,
            &ui_font_sourcesans_reg_14,
            CRA_TEXT_COLOR,
            20,
            0,
            data->detail_cpidx + 1
        );
        update->detail_text = 0;
    }

    if(update->footer_text) {
        static const char footer[] = "CRA · ELECTRIC PASS";
        rect = (fbdraw_rect_t){CRA_TEXT_X, CRA_FOOTER_Y, 220, 18};
        fbdraw_text_range(
            &fb,
            &rect,
            footer,
            &ui_font_sourcesans_reg_14,
            CRA_MUTED_TEXT_COLOR,
            0,
            0,
            data->footer_cpidx + 1
        );
        update->footer_text = 0;
    }

    if(update->logo_fade) {
        if(cra->logo_addr) {
            draw_logo(&fb, cra, data->logo_opacity);
        }
        else {
            rect = (fbdraw_rect_t){CRA_LOGO_X + 14, CRA_LOGO_Y + 20, 52, 30};
            fbdraw_fill_rect(&fb, &rect, CRA_PANEL_COLOR);
            fbdraw_text(
                &fb,
                &rect,
                "CRA",
                &ui_font_sourceselif_heavy_24,
                color_with_alpha(CRA_TEXT_COLOR, (uint8_t)data->logo_opacity),
                0,
                0
            );
        }
        update->logo_fade = 0;
    }

    if(update->top_line) {
        rect = (fbdraw_rect_t){
            CRA_HEADER_X,
            CRA_TOP_LINE_Y,
            data->top_line_width,
            2,
        };
        fbdraw_fill_rect(&fb, &rect, accent);
        update->top_line = 0;
    }

    if(update->divider) {
        rect = (fbdraw_rect_t){
            CRA_TEXT_X,
            CRA_DIVIDER_Y,
            data->divider_width,
            1,
        };
        fbdraw_fill_rect(&fb, &rect, color_with_alpha(accent, 224));
        update->divider = 0;
    }

    if(update->scan) {
        int old_y = data->scan_y[data->curr_buffer];
        if(old_y >= CRA_SCAN_RAIL_Y) {
            rect = (fbdraw_rect_t){
                CRA_SCAN_RAIL_X,
                old_y,
                CRA_SCAN_RAIL_W,
                CRA_SCAN_MARK_H,
            };
            fbdraw_fill_rect(&fb, &rect, color_with_alpha(accent, 64));
        }

        const int travel = CRA_SCAN_RAIL_H - CRA_SCAN_MARK_H;
        const int scan_y =
            CRA_SCAN_RAIL_Y + ((data->curr_frame - CRA_SCAN_START_FRAME) % travel);
        rect = (fbdraw_rect_t){
            CRA_SCAN_RAIL_X,
            scan_y,
            CRA_SCAN_RAIL_W,
            CRA_SCAN_MARK_H,
        };
        fbdraw_fill_rect(&fb, &rect, color_with_alpha(accent, 255));
        data->scan_y[data->curr_buffer] = scan_y;
        update->scan = 0;
    }

    drm_warpper_enqueue_display_item(
        data->overlay->drm_warpper,
        DRM_WARPPER_LAYER_OVERLAY,
        item
    );
    data->curr_buffer = !data->curr_buffer;
    data->curr_frame++;
}

static void cra_overlay_worker_timer_cb(void* userdata, bool is_last) {
    cra_overlay_worker_data_t* data = (cra_overlay_worker_data_t*)userdata;
    overlay_worker_schedule(data->overlay, cra_overlay_worker, data);
}

void overlay_theme_info_show_cra(overlay_t* overlay, theme_overlay_params_t* params) {
    log_info("overlay_theme_info_show_cra");

    drm_warpper_set_layer_alpha(
        overlay->drm_warpper,
        DRM_WARPPER_LAYER_OVERLAY,
        255
    );
    drm_warpper_set_layer_coord(
        overlay->drm_warpper,
        DRM_WARPPER_LAYER_OVERLAY,
        0,
        OVERLAY_HEIGHT
    );

    drm_warpper_queue_item_t* item;
    for(int i = 0; i < 2; i++) {
        drm_warpper_dequeue_free_item(
            overlay->drm_warpper,
            DRM_WARPPER_LAYER_OVERLAY,
            &item
        );
        init_template_cra_overlay((uint32_t*)item->mount.arg0, params);
        drm_warpper_enqueue_display_item(
            overlay->drm_warpper,
            DRM_WARPPER_LAYER_OVERLAY,
            item
        );
    }

    static cra_overlay_worker_data_t data;
    memset(&data, 0, sizeof(data));
    data.overlay = overlay;
    data.params = params;
    data.organization_cpcnt =
        lv_text_get_encoded_length(params->cra.organization_text);
    data.header_cpcnt = lv_text_get_encoded_length(params->cra.header_text);
    data.member_name_cpcnt =
        lv_text_get_encoded_length(params->cra.member_name);
    data.role_cpcnt = lv_text_get_encoded_length(params->cra.role_text);
    data.member_code_cpcnt =
        lv_text_get_encoded_length(params->cra.member_code);
    data.detail_cpcnt = lv_text_get_encoded_length(params->cra.detail_text);
    data.footer_cpcnt = lv_text_get_encoded_length("CRA · ELECTRIC PASS");
    data.scan_y[0] = -1;
    data.scan_y[1] = -1;

    overlay->request_abort = 0;
    overlay->overlay_used = 1;

    app_timer_create(
        &overlay->overlay_timer_handle,
        0,
        OVERLAY_ANIMATION_STEP_TIME,
        -1,
        cra_overlay_worker_timer_cb,
        &data
    );

    layer_animation_ease_in_out_move(
        overlay->layer_animation,
        DRM_WARPPER_LAYER_OVERLAY,
        0,
        OVERLAY_HEIGHT,
        0,
        0,
        1 * 1000 * 1000,
        0
    );
}
