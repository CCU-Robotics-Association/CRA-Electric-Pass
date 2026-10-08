#pragma once

#include <stdbool.h>
#include "lvgl.h"

void ui_screen_sleep_tick(lv_display_t *display);
bool ui_screen_sleep_wake_on_key(void);
void ui_screen_sleep_force_wake(void);
