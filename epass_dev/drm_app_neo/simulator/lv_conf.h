#ifndef EPASS_SIMULATOR_LV_CONF_H
#define EPASS_SIMULATOR_LV_CONF_H

/* Reuse the device display/font/widget configuration, then enable only the
 * SDL desktop backend for this separate target. */
#include "../src/lv_conf.h"

#undef LV_USE_SDL
#define LV_USE_SDL 1

#define LV_SDL_INCLUDE_PATH <SDL2/SDL.h>
#define LV_SDL_RENDER_MODE LV_DISPLAY_RENDER_MODE_DIRECT
#define LV_SDL_BUF_COUNT 2
#define LV_SDL_ACCELERATED 1
#define LV_SDL_FULLSCREEN 0
#define LV_SDL_DIRECT_EXIT 1
#define LV_SDL_MOUSEWHEEL_MODE 0

#endif /* EPASS_SIMULATOR_LV_CONF_H */

