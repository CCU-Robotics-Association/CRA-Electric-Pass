/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#ifndef CRA_EPASS_UENV_H
#define CRA_EPASS_UENV_H

#include <stddef.h>

typedef struct {
    char **lines;
    size_t line_count;
    long interface_line;
    long extension_line;
    char **interfaces;
    size_t interface_count;
    char **extensions;
    size_t extension_count;
} cra_uenv_t;

int cra_uenv_load(const char *path, cra_uenv_t *uenv, char *error, size_t error_size);
int cra_uenv_save(const char *path, const cra_uenv_t *uenv,
                  const char *interfaces, const char *extensions,
                  char *error, size_t error_size);
void cra_uenv_destroy(cra_uenv_t *uenv);

#endif
