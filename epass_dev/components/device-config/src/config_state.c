/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#define _POSIX_C_SOURCE 200809L
#include "cra_epass/config_state.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *const conflict_adc123[] = {"adc_pa1", "uart1", "i2s0_pa", "i2s0_pe"};
static const char *const conflict_adc1[] = {"adc_pa123", "i2s0_pa", "i2s0_pe"};
static const char *const conflict_i2s_pa[] = {"i2s0_pe", "adc_pa123", "adc_pa1", "uart1"};
static const char *const conflict_i2s_pe[] = {"i2s0_pa", "adc_pa123", "adc_pa1"};
static const char *const conflict_uart1[] = {"adc_pa123", "i2s0_pa"};
static const char *const conflict_uart2[] = {"spi1"};
static const char *const conflict_spi1[] = {"uart2"};
static const char *const conflict_i2c[] = {"es8311_sound"};
static const char *const require_i2c[] = {"i2c0"};
static const char *const require_i2s[] = {"i2s0_pa", "i2s0_pe"};
static const char *const conflict_cardkb[] = {"es8311_sound"};
static const char *const conflict_es8311[] = {"i2c0", "cardkb"};

static const cra_config_option_t options[] = {
    {"adc_pa123", CRA_CONFIG_INTERFACE, "PA1/PA2/PA3 as ADC inputs", true, NULL, 0, NULL, 0, conflict_adc123, 4},
    {"adc_pa1", CRA_CONFIG_INTERFACE, "PA1 as an ADC input", true, NULL, 0, NULL, 0, conflict_adc1, 3},
    {"i2c0", CRA_CONFIG_INTERFACE, "I2C0 on PD0/PD12", true, NULL, 0, NULL, 0, conflict_i2c, 1},
    {"i2s0_pa", CRA_CONFIG_INTERFACE, "I2S0 on the PA routing", true, NULL, 0, NULL, 0, conflict_i2s_pa, 4},
    {"i2s0_pe", CRA_CONFIG_INTERFACE, "I2S0 on the PE routing", true, NULL, 0, NULL, 0, conflict_i2s_pe, 3},
    {"spi1", CRA_CONFIG_INTERFACE, "SPI1 on PE7/PE8/PE9/PE10", true, NULL, 0, NULL, 0, conflict_spi1, 1},
    {"uart1", CRA_CONFIG_INTERFACE, "UART1 on PA2/PA3", true, NULL, 0, NULL, 0, conflict_uart1, 2},
    {"uart2", CRA_CONFIG_INTERFACE, "UART2 on PA7/PA8", true, NULL, 0, NULL, 0, conflict_uart2, 1},
    {"usbhost", CRA_CONFIG_INTERFACE, "USB host mode", true, NULL, 0, NULL, 0, NULL, 0},
    {"usbhs", CRA_CONFIG_INTERFACE, "USB high-speed mode", true, NULL, 0, NULL, 0, NULL, 0},
    {"cardkb", CRA_CONFIG_EXTENSION, "M5Stack CardKB (requires I2C0)", true,
        require_i2c, 1, NULL, 0, conflict_cardkb, 1},
    {"es8311_sound", CRA_CONFIG_EXTENSION, "ES8311 sound (requires one I2S0 routing)", true,
        NULL, 0, require_i2s, 2, conflict_es8311, 2},
    {"lsm6ds3_pre0.4", CRA_CONFIG_EXTENSION, "Legacy IMU; PE2 conflicts with power-off", false,
        require_i2c, 1, NULL, 0, NULL, 0}
};

const cra_config_option_t *cra_config_options(size_t *count) {
    *count = sizeof(options) / sizeof(options[0]); return options;
}

int cra_config_find_option(const char *id) {
    size_t i; for (i = 0; i < sizeof(options) / sizeof(options[0]); ++i)
        if (strcmp(options[i].id, id) == 0) return (int)i;
    return -1;
}

static int push_copy(char ***values, size_t *count, const char *text) {
    char **next = realloc(*values, (*count + 1U) * sizeof(**values));
    if (next == NULL) return -1;
    *values = next; next[*count] = strdup(text);
    if (next[*count] == NULL) return -1;
    ++*count; return 0;
}

static void free_values(char **values, size_t count) {
    size_t i; for (i = 0; i < count; ++i) free(values[i]); free(values);
}

int cra_config_state_init(cra_config_state_t *state, const cra_uenv_t *uenv) {
    size_t count = sizeof(options) / sizeof(options[0]);
    size_t i;
    (void)memset(state, 0, sizeof(*state));
    state->enabled = calloc(count, sizeof(*state->enabled));
    if (state->enabled == NULL) return -1;
    for (i = 0; i < uenv->interface_count; ++i) {
        int index = cra_config_find_option(uenv->interfaces[i]);
        if (index >= 0 && options[index].category == CRA_CONFIG_INTERFACE) state->enabled[index] = true;
        else if (push_copy(&state->unknown_interfaces, &state->unknown_interface_count,
                           uenv->interfaces[i]) != 0) goto fail;
    }
    for (i = 0; i < uenv->extension_count; ++i) {
        int index = cra_config_find_option(uenv->extensions[i]);
        if (index >= 0 && options[index].category == CRA_CONFIG_EXTENSION) state->enabled[index] = true;
        else if (push_copy(&state->unknown_extensions, &state->unknown_extension_count,
                           uenv->extensions[i]) != 0) goto fail;
    }
    return 0;
fail:
    cra_config_state_destroy(state); return -1;
}

void cra_config_state_destroy(cra_config_state_t *state) {
    free(state->enabled);
    free_values(state->unknown_interfaces, state->unknown_interface_count);
    free_values(state->unknown_extensions, state->unknown_extension_count);
    (void)memset(state, 0, sizeof(*state));
}

static void set_error(char *error, size_t size, const char *format, const char *id) {
    if (size != 0U) (void)snprintf(error, size, format, id);
}

static int enable_recursive(cra_config_state_t *state, size_t index,
                            bool resolve_conflicts, unsigned int depth,
                            char *error, size_t error_size) {
    const cra_config_option_t *option;
    size_t i;
    if (depth > 16U || index >= sizeof(options) / sizeof(options[0])) return -1;
    option = &options[index];
    if (!option->available_on_v06) { set_error(error, error_size, "%s is unavailable on v0.6", option->id); return -1; }
    for (i = 0; i < option->requires_one_count; ++i) {
        int required = cra_config_find_option(option->requires_one[i]);
        if (required >= 0 && state->enabled[required]) break;
    }
    if (option->requires_one_count != 0U && i == option->requires_one_count) {
        set_error(error, error_size, "%s requires one I2S0 routing to be enabled first", option->id); return -1;
    }
    for (i = 0; i < option->conflict_count; ++i) {
        int conflict = cra_config_find_option(option->conflicts[i]);
        if (conflict >= 0 && state->enabled[conflict]) {
            if (!resolve_conflicts) { set_error(error, error_size, "%s conflicts with an enabled option", option->id); return 1; }
            state->enabled[conflict] = false;
        }
    }
    for (i = 0; i < option->requires_all_count; ++i) {
        int required = cra_config_find_option(option->requires_all[i]);
        if (required < 0 || enable_recursive(state, (size_t)required, resolve_conflicts,
                                              depth + 1U, error, error_size) != 0) return -1;
    }
    state->enabled[index] = true; return 0;
}

int cra_config_enable(cra_config_state_t *state, size_t index,
                      bool resolve_conflicts, char *error, size_t error_size) {
    return enable_recursive(state, index, resolve_conflicts, 0, error, error_size);
}

int cra_config_disable(cra_config_state_t *state, size_t index,
                       char *error, size_t error_size) {
    size_t i, j;
    if (index >= sizeof(options) / sizeof(options[0])) return -1;
    for (i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        if (!state->enabled[i]) continue;
        for (j = 0; j < options[i].requires_all_count; ++j) {
            if (strcmp(options[i].requires_all[j], options[index].id) == 0) {
                set_error(error, error_size, "%s is required by an enabled extension", options[index].id); return -1;
            }
        }
        if (options[i].requires_one_count != 0U) {
            size_t active = 0;
            bool depends_on_target = false;
            for (j = 0; j < options[i].requires_one_count; ++j) {
                int required = cra_config_find_option(options[i].requires_one[j]);
                if (required == (int)index) depends_on_target = true;
                if (required >= 0 && state->enabled[required]) ++active;
            }
            if (depends_on_target && active == 1U) {
                set_error(error, error_size, "%s is the only routing used by an enabled extension", options[index].id); return -1;
            }
        }
    }
    state->enabled[index] = false; return 0;
}

static int append_token(char **line, size_t *used, const char *token) {
    size_t length = strlen(token);
    char *next = realloc(*line, *used + length + (*used == 0U ? 1U : 2U));
    if (next == NULL) return -1;
    *line = next;
    if (*used != 0U) next[(*used)++] = ' ';
    (void)memcpy(next + *used, token, length); *used += length; next[*used] = '\0'; return 0;
}

char *cra_config_build_line(const cra_config_state_t *state,
                            cra_config_category_t category) {
    char *line = calloc(1, 1U); size_t used = 0; size_t i;
    char **unknown = category == CRA_CONFIG_INTERFACE ? state->unknown_interfaces : state->unknown_extensions;
    size_t unknown_count = category == CRA_CONFIG_INTERFACE ? state->unknown_interface_count : state->unknown_extension_count;
    if (line == NULL) return NULL;
    for (i = 0; i < sizeof(options) / sizeof(options[0]); ++i)
        if (options[i].category == category && state->enabled[i] &&
            append_token(&line, &used, options[i].id) != 0) { free(line); return NULL; }
    for (i = 0; i < unknown_count; ++i)
        if (append_token(&line, &used, unknown[i]) != 0) { free(line); return NULL; }
    return line;
}
