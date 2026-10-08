#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl/lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _groups_t {
    lv_group_t *navigation;
} groups_t;

extern groups_t groups;

void ui_create_groups();

typedef struct _objects_t {
    lv_obj_t *mainmenu;
    lv_obj_t *theme_list;
    lv_obj_t *sysinfo;
    lv_obj_t *spinner;
    lv_obj_t *displayimg;
    lv_obj_t *filemanager;
    lv_obj_t *settings;
    lv_obj_t *warning;
    lv_obj_t *confirm;
    lv_obj_t *applist;
    lv_obj_t *theme_list_btn;
    lv_obj_t *dispimg_btn;
    lv_obj_t *apps_btn;
    lv_obj_t *file_btn;
    lv_obj_t *sett_btn;
    lv_obj_t *dev_btn;
    lv_obj_t *brightness_scroller;
    lv_obj_t *restart_app_btn;
    lv_obj_t *shutdown_btn;
    lv_obj_t *obj0;
    lv_obj_t *theme_list_container;
    lv_obj_t *obj1;
    lv_obj_t *obj1__theme_btn;
    lv_obj_t *obj1__theme_icon;
    lv_obj_t *obj1__theme_description;
    lv_obj_t *obj1__theme_name;
    lv_obj_t *obj1__sd_flag_1;
    lv_obj_t *obj2;
    lv_obj_t *obj2__theme_btn;
    lv_obj_t *obj2__theme_icon;
    lv_obj_t *obj2__theme_description;
    lv_obj_t *obj2__theme_name;
    lv_obj_t *obj2__sd_flag_1;
    lv_obj_t *refresh_theme_list_btn;
    lv_obj_t *obj3;
    lv_obj_t *mainmenu_btn;
    lv_obj_t *obj4;
    lv_obj_t *obj5;
    lv_obj_t *obj6;
    lv_obj_t *back_btn;
    lv_obj_t *obj7;
    lv_obj_t *obj8;
    lv_obj_t *obj9;
    lv_obj_t *obj10;
    lv_obj_t *obj11;
    lv_obj_t *obj12;
    lv_obj_t *obj13;
    lv_obj_t *dispimg_container;
    lv_obj_t *file_container;
    lv_obj_t *dummybtn;
    lv_obj_t *lowbat_trip;
    lv_obj_t *no_intro_block;
    lv_obj_t *no_overlay_block;
    lv_obj_t *sleep_timeout_dropdown;
    lv_obj_t *theme_color_dropdown;
    lv_obj_t *swmode_dropdown;
    lv_obj_t *swint_dropdown;
    lv_obj_t *usbmode_dropdown;
    lv_obj_t *obj14;
    lv_obj_t *back_btn_1;
    lv_obj_t *obj15;
    lv_obj_t *obj16;
    lv_obj_t *obj17;
    lv_obj_t *obj18;
    lv_obj_t *obj19;
    lv_obj_t *back_btn_2;
    lv_obj_t *obj20;
    lv_obj_t *obj21;
    lv_obj_t *app_container;
    lv_obj_t *obj22;
    lv_obj_t *obj22__appbtn;
    lv_obj_t *obj22__applogo;
    lv_obj_t *obj22__appdesc;
    lv_obj_t *obj22__appname;
    lv_obj_t *obj22__bgfg_flag;
    lv_obj_t *obj22__sd_flag;
    lv_obj_t *obj23;
    lv_obj_t *obj23__appbtn;
    lv_obj_t *obj23__applogo;
    lv_obj_t *obj23__appdesc;
    lv_obj_t *obj23__appname;
    lv_obj_t *obj23__bgfg_flag;
    lv_obj_t *obj23__sd_flag;
    lv_obj_t *applist_back_btn;
    lv_obj_t *obj24;
    lv_obj_t *applist_no_app_label;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAINMENU = 1,
    SCREEN_ID_THEME_LIST = 2,
    SCREEN_ID_SYSINFO = 3,
    SCREEN_ID_SPINNER = 4,
    SCREEN_ID_DISPLAYIMG = 5,
    SCREEN_ID_FILEMANAGER = 6,
    SCREEN_ID_SETTINGS = 7,
    SCREEN_ID_WARNING = 8,
    SCREEN_ID_CONFIRM = 9,
    SCREEN_ID_APPLIST = 10,
};

void create_screen_mainmenu();
void tick_screen_mainmenu();

void create_screen_theme_list();
void tick_screen_theme_list();

void create_screen_sysinfo();
void tick_screen_sysinfo();

void create_screen_spinner();
void tick_screen_spinner();

void create_screen_displayimg();
void tick_screen_displayimg();

void create_screen_filemanager();
void tick_screen_filemanager();

void create_screen_settings();
void tick_screen_settings();

void create_screen_warning();
void tick_screen_warning();

void create_screen_confirm();
void tick_screen_confirm();

void create_screen_applist();
void tick_screen_applist();

void create_user_widget_theme_entry(lv_obj_t *parent_obj, int startWidgetIndex);
void tick_user_widget_theme_entry(int startWidgetIndex);

void create_user_widget_app_entry(lv_obj_t *parent_obj, int startWidgetIndex);
void tick_user_widget_app_entry(int startWidgetIndex);

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/
