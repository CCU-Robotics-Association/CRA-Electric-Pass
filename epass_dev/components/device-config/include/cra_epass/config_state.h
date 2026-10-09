/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#ifndef CRA_EPASS_CONFIG_STATE_H
#define CRA_EPASS_CONFIG_STATE_H

#include "cra_epass/uenv.h"

#include <stdbool.h>
#include <stddef.h>

typedef enum { CRA_CONFIG_INTERFACE, CRA_CONFIG_EXTENSION } cra_config_category_t;

typedef struct {
    const char *id;
    cra_config_category_t category;
    const char *description;
    bool available_on_v06;
    const char *const *requires_all;
    size_t requires_all_count;
    const char *const *requires_one;
    size_t requires_one_count;
    const char *const *conflicts;
    size_t conflict_count;
} cra_config_option_t;

typedef struct {
    bool *enabled;
    char **unknown_interfaces;
    size_t unknown_interface_count;
    char **unknown_extensions;
    size_t unknown_extension_count;
} cra_config_state_t;

const cra_config_option_t *cra_config_options(size_t *count);
int cra_config_find_option(const char *id);
int cra_config_state_init(cra_config_state_t *state, const cra_uenv_t *uenv);
void cra_config_state_destroy(cra_config_state_t *state);
int cra_config_enable(cra_config_state_t *state, size_t index,
                      bool resolve_conflicts, char *error, size_t error_size);
int cra_config_disable(cra_config_state_t *state, size_t index,
                       char *error, size_t error_size);
char *cra_config_build_line(const cra_config_state_t *state,
                            cra_config_category_t category);

#endif
