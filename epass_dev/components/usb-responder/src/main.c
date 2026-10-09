/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#include "cra_epass/functionfs.h"

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void usage(const char *program) {
    (void)fprintf(stderr,
        "usage: %s --ffs <mount> [-v|--verbose] [--timeout-ms N] "
        "[--max-stdout N] [--max-stderr N] [--no-command]\n", program);
}

static int parse_u32(const char *text, uint32_t *value) {
    char *end = NULL;
    unsigned long parsed;
    if (*text == '-') return -1;
    errno = 0; parsed = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed > UINT32_MAX) return -1;
    *value = (uint32_t)parsed; return 0;
}

int main(int argc, char **argv) {
    const char *mount_path = NULL;
    bool verbose = false;
    cra_usb_service_t service;
    int i;
    if (cra_usb_service_init(&service, "/") != 0) { perror("service init"); return 2; }
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--ffs") == 0 && i + 1 < argc) mount_path = argv[++i];
        else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) verbose = true;
        else if (strcmp(argv[i], "--timeout-ms") == 0 && i + 1 < argc &&
                 parse_u32(argv[++i], &service.default_timeout_ms) == 0) {}
        else if (strcmp(argv[i], "--max-stdout") == 0 && i + 1 < argc &&
                 parse_u32(argv[++i], &service.maximum_stdout) == 0) {}
        else if (strcmp(argv[i], "--max-stderr") == 0 && i + 1 < argc &&
                 parse_u32(argv[++i], &service.maximum_stderr) == 0) {}
        else if (strcmp(argv[i], "--no-command") == 0) service.allow_command = false;
        else { usage(argv[0]); cra_usb_service_destroy(&service); return 1; }
    }
    if (mount_path == NULL) { usage(argv[0]); cra_usb_service_destroy(&service); return 1; }
    if (service.maximum_stdout > 8U * 1024U * 1024U) service.maximum_stdout = 8U * 1024U * 1024U;
    if (service.maximum_stderr > 8U * 1024U * 1024U) service.maximum_stderr = 8U * 1024U * 1024U;
    int result = cra_functionfs_run(mount_path, &service, 8U * 1024U * 1024U, verbose);
    if (result != 0) perror("usb_responder");
    cra_usb_service_destroy(&service);
    return result == 0 ? 0 : 2;
}
