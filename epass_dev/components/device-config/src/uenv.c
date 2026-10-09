/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#define _POSIX_C_SOURCE 200809L
#include "cra_epass/uenv.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void set_error(char *error, size_t size, const char *format, ...) {
    va_list args;
    if (size == 0U) return;
    va_start(args, format); (void)vsnprintf(error, size, format, args); va_end(args);
}

static int push_string(char ***values, size_t *count, const char *text, size_t length) {
    char **next = realloc(*values, (*count + 1U) * sizeof(**values));
    char *copy;
    if (next == NULL) return -1;
    *values = next;
    copy = malloc(length + 1U);
    if (copy == NULL) return -1;
    (void)memcpy(copy, text, length); copy[length] = '\0';
    next[*count] = copy; ++*count; return 0;
}

static void free_strings(char **values, size_t count) {
    size_t i; for (i = 0; i < count; ++i) free(values[i]); free(values);
}

static const char *value_after_key(const char *line, const char *key) {
    size_t key_length = strlen(key);
    while (*line != '\0' && isspace((unsigned char)*line) && *line != '\n' && *line != '\r') ++line;
    return strncmp(line, key, key_length) == 0 ? line + key_length : NULL;
}

static int parse_tokens(const char *value, char ***tokens, size_t *count) {
    while (value != NULL && *value != '\0') {
        const char *start;
        while (*value != '\0' && isspace((unsigned char)*value)) ++value;
        if (*value == '\0') break;
        start = value;
        while (*value != '\0' && !isspace((unsigned char)*value)) ++value;
        if (push_string(tokens, count, start, (size_t)(value - start)) != 0) return -1;
    }
    return 0;
}

int cra_uenv_load(const char *path, cra_uenv_t *uenv, char *error, size_t error_size) {
    FILE *file;
    char *line = NULL;
    size_t capacity = 0;
    ssize_t length;
    (void)memset(uenv, 0, sizeof(*uenv));
    uenv->interface_line = -1; uenv->extension_line = -1;
    file = fopen(path, "rb");
    if (file == NULL) { set_error(error, error_size, "cannot open %s: %s", path, strerror(errno)); return -1; }
    while ((length = getline(&line, &capacity, file)) >= 0) {
        const char *value;
        if (push_string(&uenv->lines, &uenv->line_count, line, (size_t)length) != 0) goto memory_error;
        if (uenv->interface_line < 0 && (value = value_after_key(line, "interface=")) != NULL) {
            uenv->interface_line = (long)(uenv->line_count - 1U);
            if (parse_tokens(value, &uenv->interfaces, &uenv->interface_count) != 0) goto memory_error;
        } else if (uenv->extension_line < 0 && (value = value_after_key(line, "ext=")) != NULL) {
            uenv->extension_line = (long)(uenv->line_count - 1U);
            if (parse_tokens(value, &uenv->extensions, &uenv->extension_count) != 0) goto memory_error;
        }
    }
    free(line);
    if (ferror(file)) { set_error(error, error_size, "cannot read %s: %s", path, strerror(errno)); (void)fclose(file); cra_uenv_destroy(uenv); return -1; }
    (void)fclose(file); return 0;
memory_error:
    free(line); (void)fclose(file); cra_uenv_destroy(uenv);
    set_error(error, error_size, "out of memory"); return -1;
}

void cra_uenv_destroy(cra_uenv_t *uenv) {
    free_strings(uenv->lines, uenv->line_count);
    free_strings(uenv->interfaces, uenv->interface_count);
    free_strings(uenv->extensions, uenv->extension_count);
    (void)memset(uenv, 0, sizeof(*uenv));
    uenv->interface_line = -1; uenv->extension_line = -1;
}

static int write_setting(FILE *file, const char *key, const char *value) {
    return fprintf(file, "%s%s\n", key, value) < 0 ? -1 : 0;
}

int cra_uenv_save(const char *path, const cra_uenv_t *uenv,
                  const char *interfaces, const char *extensions,
                  char *error, size_t error_size) {
    char temporary[1024];
    struct stat original;
    FILE *file;
    int fd;
    size_t i;
    bool ended_with_newline = true;
    if (uenv->line_count != 0U) {
        const char *last = uenv->lines[uenv->line_count - 1U];
        size_t last_length = strlen(last);
        ended_with_newline = last_length != 0U && last[last_length - 1U] == '\n';
    }
    if (snprintf(temporary, sizeof(temporary), "%s.cra.XXXXXX", path) >= (int)sizeof(temporary)) {
        set_error(error, error_size, "path is too long"); return -1;
    }
    fd = mkstemp(temporary);
    if (fd < 0) { set_error(error, error_size, "cannot create temporary file: %s", strerror(errno)); return -1; }
    if (stat(path, &original) == 0) (void)fchmod(fd, original.st_mode & 07777U);
    file = fdopen(fd, "wb");
    if (file == NULL) { int saved = errno; (void)close(fd); (void)unlink(temporary); set_error(error, error_size, "%s", strerror(saved)); return -1; }
    for (i = 0; i < uenv->line_count; ++i) {
        if ((long)i == uenv->interface_line) { if (write_setting(file, "interface=", interfaces) != 0) goto write_error; }
        else if ((long)i == uenv->extension_line) { if (write_setting(file, "ext=", extensions) != 0) goto write_error; }
        else if (fputs(uenv->lines[i], file) == EOF) goto write_error;
    }
    if (uenv->interface_line < 0) {
        if (!ended_with_newline && fputc('\n', file) == EOF) goto write_error;
        if (write_setting(file, "interface=", interfaces) != 0) goto write_error;
        ended_with_newline = true;
    }
    if (uenv->extension_line < 0) {
        if (!ended_with_newline && fputc('\n', file) == EOF) goto write_error;
        if (write_setting(file, "ext=", extensions) != 0) goto write_error;
    }
    if (fflush(file) != 0 || fsync(fileno(file)) != 0 || fclose(file) != 0) {
        set_error(error, error_size, "cannot flush configuration: %s", strerror(errno)); (void)unlink(temporary); return -1;
    }
    if (rename(temporary, path) != 0) {
        set_error(error, error_size, "cannot replace %s: %s", path, strerror(errno)); (void)unlink(temporary); return -1;
    }
    return 0;
write_error:
    set_error(error, error_size, "cannot write configuration: %s", strerror(errno));
    (void)fclose(file); (void)unlink(temporary); return -1;
}
