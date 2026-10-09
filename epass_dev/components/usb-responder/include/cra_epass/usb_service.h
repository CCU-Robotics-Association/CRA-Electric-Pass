/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#ifndef CRA_EPASS_USB_SERVICE_H
#define CRA_EPASS_USB_SERVICE_H

#include "cra_epass/usb_protocol.h"

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    char root[PATH_MAX];
    bool allow_command;
    uint32_t default_timeout_ms;
    uint32_t maximum_stdout;
    uint32_t maximum_stderr;
    uint32_t maximum_file_response;
    int upload_fd;
    uint32_t upload_id;
    unsigned int upload_mode;
    char upload_final[PATH_MAX];
    char upload_temporary[PATH_MAX];
} cra_usb_service_t;

typedef struct {
    uint16_t type;
    uint32_t request_id;
    uint8_t *payload;
    uint32_t payload_length;
} cra_usb_response_t;

int cra_usb_service_init(cra_usb_service_t *service, const char *root);
void cra_usb_service_destroy(cra_usb_service_t *service);
int cra_usb_service_handle(cra_usb_service_t *service,
                           const cra_usb_frame_t *request,
                           cra_usb_response_t *response);
void cra_usb_response_destroy(cra_usb_response_t *response);

int cra_usb_execute_command(const uint8_t *payload, size_t payload_size,
                            uint32_t default_timeout_ms,
                            uint32_t maximum_stdout,
                            uint32_t maximum_stderr,
                            uint8_t **result, uint32_t *result_size,
                            char *error, size_t error_size);

#endif
