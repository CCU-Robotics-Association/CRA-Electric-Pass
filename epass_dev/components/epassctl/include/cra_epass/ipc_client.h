/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#ifndef CRA_EPASS_IPC_CLIENT_H
#define CRA_EPASS_IPC_CLIENT_H

#include <stddef.h>

int cra_ipc_exchange(const char *socket_path,
                     const void *request,
                     size_t request_size,
                     void *response,
                     size_t response_capacity,
                     size_t *response_size,
                     char *error,
                     size_t error_capacity);

#endif
