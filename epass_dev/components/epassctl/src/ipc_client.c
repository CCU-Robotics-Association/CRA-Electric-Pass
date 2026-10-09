/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#include "cra_epass/ipc_client.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <unistd.h>

static int fail(char *error, size_t capacity, const char *operation) {
    if (capacity > 0U) {
        (void)snprintf(error, capacity, "%s: %s", operation, strerror(errno));
    }
    return -1;
}

int cra_ipc_exchange(const char *socket_path,
                     const void *request,
                     size_t request_size,
                     void *response,
                     size_t response_capacity,
                     size_t *response_size,
                     char *error,
                     size_t error_capacity) {
    struct sockaddr_un address;
    struct timeval timeout = { .tv_sec = 5, .tv_usec = 0 };
    struct iovec iov;
    struct msghdr message;
    ssize_t received;
    int fd;

    if (strlen(socket_path) >= sizeof(address.sun_path)) {
        errno = ENAMETOOLONG;
        return fail(error, error_capacity, "socket path");
    }

    fd = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
    if (fd < 0) return fail(error, error_capacity, "socket");
    (void)setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    (void)setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    (void)memcpy(address.sun_path, socket_path, strlen(socket_path) + 1U);
    if (connect(fd, (const struct sockaddr *)&address, sizeof(address)) < 0) {
        int saved = errno;
        (void)close(fd);
        errno = saved;
        return fail(error, error_capacity, "connect");
    }

    if (send(fd, request, request_size, 0) != (ssize_t)request_size) {
        int saved = errno;
        (void)close(fd);
        errno = saved == 0 ? EIO : saved;
        return fail(error, error_capacity, "send");
    }

    memset(&message, 0, sizeof(message));
    iov.iov_base = response;
    iov.iov_len = response_capacity;
    message.msg_iov = &iov;
    message.msg_iovlen = 1;
    received = recvmsg(fd, &message, 0);
    if (received < 0) {
        int saved = errno;
        (void)close(fd);
        errno = saved;
        return fail(error, error_capacity, "receive");
    }
    (void)close(fd);

    if ((message.msg_flags & MSG_TRUNC) != 0) {
        errno = EMSGSIZE;
        return fail(error, error_capacity, "response");
    }
    if (received == 0) {
        errno = ECONNRESET;
        return fail(error, error_capacity, "response");
    }
    *response_size = (size_t)received;
    return 0;
}
