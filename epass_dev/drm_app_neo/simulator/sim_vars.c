#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "vars.h"
#include "ui/theme.h"

#define STRING_VAR(name, size, initial)                                      \
    static char name##_value[size] = initial;                                \
    const char *get_var_##name(void)                                         \
    {                                                                         \
        return name##_value;                                                  \
    }                                                                         \
    void set_var_##name(const char *value)                                   \
    {                                                                         \
        snprintf(name##_value, sizeof(name##_value), "%s",                   \
                 value != NULL ? value : "");                                \
    }

STRING_VAR(epass_version, 64, "v1.0 - beta")
STRING_VAR(sysinfo, 2048,
           "长春大学机器人协会电子通行证程序\n"
           "CCU Robotics Association - Electric Pass\n"
           "版本号: v1.0 - beta\n"
           "本项目献以历届长春大学机器人协会成员\n"
           "祝愿持有此通行证的各位:\n"
           "一扫沉疴，不等炬火\n"
           "勇敢立破，初心不改\n"
           "风华正茂，成就非凡\n"
           "升则飞腾于宇宙，隐则潜伏于波涛\n"
           "基于白银 伊卡洛斯开源自由硬件二次开发设计\n"
           "https://github.com/CCU-Robotics-Association/CRA-Electric-Pass\n"
           "本人对于硬件及嵌入式领域了解尚浅，故此项目诚待后辈优化\n"
           "关于此项目疑问及更新改进建议欢迎联系1255084501@qq.com")
STRING_VAR(nand_label, 64, "NAND 41.2 MB / 64.0 MB")
STRING_VAR(sd_label, 64, "SD 3.8 GB / 7.4 GB")
STRING_VAR(warning_title, 64, "桌面模拟")
STRING_VAR(warning_desc, 256, "此操作只在模拟器中演示，不会访问实体设备。")
STRING_VAR(confirm_title, 128, "确认执行模拟操作？")
STRING_VAR(warning_icon, 32, "\xEF\x81\xB1")

static sw_mode_t sw_mode_value = sw_mode_t_SW_MODE_SEQUENCE;
static sw_interval_t sw_interval_value = sw_interval_t_SW_INTERVAL_3MIN;
static sleep_timeout_t sleep_timeout_value = sleep_timeout_t_SLEEP_TIMEOUT_3MIN;
static theme_color_t theme_color_value = theme_color_t_THEME_WHITE;
static usb_mode_t usb_mode_value = usb_mode_t_RNDIS;
static int32_t brightness_value = 5;
static int32_t nand_percent_value = 64;
static int32_t sd_percent_value = 51;
static bool applist_show_warning_value = true;

sw_mode_t get_var_sw_mode(void)
{
    return sw_mode_value;
}

void set_var_sw_mode(sw_mode_t value)
{
    sw_mode_value = value;
}

sw_interval_t get_var_sw_interval(void)
{
    return sw_interval_value;
}

void set_var_sw_interval(sw_interval_t value)
{
    sw_interval_value = value;
}

sleep_timeout_t get_var_sleep_timeout(void)
{
    return sleep_timeout_value;
}

void set_var_sleep_timeout(sleep_timeout_t value)
{
    sleep_timeout_value = value;
}

theme_color_t get_var_theme_color(void)
{
    return theme_color_value;
}

void set_var_theme_color(theme_color_t value)
{
    theme_color_value = value;
    ui_theme_apply(value);
}

usb_mode_t get_var_usb_mode(void)
{
    return usb_mode_value;
}

void set_var_usb_mode(usb_mode_t value)
{
    usb_mode_value = value;
}

int32_t get_var_brightness(void)
{
    return brightness_value;
}

void set_var_brightness(int32_t value)
{
    brightness_value = value;
}

int32_t get_var_nand_percent(void)
{
    return nand_percent_value;
}

void set_var_nand_percent(int32_t value)
{
    nand_percent_value = value;
}

int32_t get_var_sd_percent(void)
{
    return sd_percent_value;
}

void set_var_sd_percent(int32_t value)
{
    sd_percent_value = value;
}

bool get_var_applist_show_warning(void)
{
    return applist_show_warning_value;
}

void set_var_applist_show_warning(bool value)
{
    applist_show_warning_value = value;
}
