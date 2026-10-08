#include "ui/screen_sleep.h"

#include "utils/log.h"
#include "utils/settings.h"

extern settings_t g_settings;

static bool screen_sleeping = false;

static uint32_t sleep_timeout_ms(sleep_timeout_t timeout)
{
    switch(timeout) {
        case sleep_timeout_t_SLEEP_TIMEOUT_30SEC:
            return 30U * 1000U;
        case sleep_timeout_t_SLEEP_TIMEOUT_1MIN:
            return 60U * 1000U;
        case sleep_timeout_t_SLEEP_TIMEOUT_3MIN:
            return 3U * 60U * 1000U;
        case sleep_timeout_t_SLEEP_TIMEOUT_NEVER:
        default:
            return 0;
    }
}

void ui_screen_sleep_force_wake(void)
{
    if(!screen_sleeping) {
        return;
    }

    settings_apply_brightness(g_settings.brightness);
    lv_display_trigger_activity(NULL);
    screen_sleeping = false;
    log_info("screen wake");
}

bool ui_screen_sleep_wake_on_key(void)
{
    if(!screen_sleeping) {
        return false;
    }

    ui_screen_sleep_force_wake();
    return true;
}

void ui_screen_sleep_tick(lv_display_t *display)
{
    if(screen_sleeping) {
        return;
    }

    uint32_t timeout = sleep_timeout_ms(g_settings.sleep_timeout);
    if(timeout == 0 || lv_display_get_inactive_time(display) < timeout) {
        return;
    }

    settings_apply_brightness(0);
    screen_sleeping = true;
    log_info("screen sleep");
}
