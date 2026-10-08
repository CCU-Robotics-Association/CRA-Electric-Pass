#pragma once

#include "theme/theme.h"

int theme_try_load(
    theme_t *manager,
    theme_entry_t *entry,
    const char *path,
    theme_source_t source,
    int index
);
int theme_scan_assets(theme_t *manager, const char *dirpath, theme_source_t source);

#ifndef APP_RELEASE
void theme_log_entry(const theme_entry_t *entry);
#else
#define theme_log_entry(entry) do {} while(0)
#endif // APP_RELEASE
