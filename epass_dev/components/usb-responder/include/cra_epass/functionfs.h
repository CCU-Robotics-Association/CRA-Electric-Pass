/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#ifndef CRA_EPASS_FUNCTIONFS_H
#define CRA_EPASS_FUNCTIONFS_H

#include "cra_epass/usb_service.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

int cra_functionfs_build_descriptors(uint8_t **data, size_t *size);
int cra_functionfs_build_strings(uint8_t **data, size_t *size);
int cra_functionfs_run(const char *mount_path, cra_usb_service_t *service,
                       size_t maximum_payload, bool verbose);

#endif
