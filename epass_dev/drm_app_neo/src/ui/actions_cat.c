#include "ui.h"

#include "ui/scr_transition.h"
#include "vars.h"

/*
 * EEZ 项目中的 Cat 页面仍沿用旧版 DisplayImg 动作标识。
 * 动作名属于生成接口，实际功能仅负责进入 Cat 页面，不再扫描或显示
 * 旧版图片目录中的内容。
 */
void action_show_dispimg(lv_event_t *event)
{
    lv_obj_t *target = lv_event_get_target(event);
    lv_obj_remove_state(target, LV_STATE_PRESSED);
    ui_schedule_screen_transition(curr_screen_t_SCREEN_DISPLAYIMG);
}

void action_displayimg_key(lv_event_t *event)
{
    (void)event;
}
