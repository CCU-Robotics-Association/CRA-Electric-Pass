#pragma once

#include <stdbool.h>

typedef struct {
    bool image_cache_cleared;
    bool launcher_file_removed;
    int launcher_file_error;
} ui_cache_clear_result_t;

void ui_cache_clear(ui_cache_clear_result_t *result);
