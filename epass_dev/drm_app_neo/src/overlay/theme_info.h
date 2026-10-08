#pragma once

#include "overlay/overlay.h"

typedef enum {
    THEME_INFO_TYPE_IMAGE,
    THEME_INFO_TYPE_CRA_PASS,
    THEME_INFO_TYPE_CRA,
    THEME_INFO_TYPE_NONE,
} theme_info_type_t;

typedef struct {
    char member_name[40];
    char member_code[40];
    char role_text[40];
    char detail_text[256];
    char organization_text[80];
    char header_text[80];

    char logo_path[128];
    int logo_w;
    int logo_h;
    uint32_t* logo_addr;

    uint32_t color;
} cra_overlay_options_t;

typedef struct {
    theme_info_type_t type;

    // 通用参数
    int appear_time;

    // image 图像类型
    int duration; // 图像进入时的动画时长
    char image_path[128];
    int image_w;
    int image_h;
    uint32_t* image_addr;

    // CRA E-Pass 动态通行证模板
    char display_name[20];
    char display_code[40];
    char barcode_text[40];
    char staff_text[40];
    
    char class_path[128];
    int class_w;
    int class_h;
    uint32_t* class_addr;

    char aux_text[256];

    char logo_path[128];
    int logo_w;
    int logo_h;
    uint32_t* logo_addr;

    char top_left_text[40];        // 左上角自定义文字
    char top_right_bar_text[40];   // 右侧装饰条自定义文字

    uint32_t color;

    // CRA 带有独立动态效果的长春大学机器人协会模板
    cra_overlay_options_t cra;

} theme_overlay_params_t;


void overlay_theme_info_load_image(theme_overlay_params_t* params);
void overlay_theme_info_free_image(theme_overlay_params_t* params);

void overlay_theme_info_show_image(overlay_t* overlay,theme_overlay_params_t* params);
void overlay_theme_info_show_cra_pass(overlay_t* overlay,theme_overlay_params_t* params);
void overlay_theme_info_show_cra(overlay_t* overlay,theme_overlay_params_t* params);
