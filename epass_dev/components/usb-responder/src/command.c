/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#define _POSIX_C_SOURCE 200809L
#include "cra_epass/usb_service.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static uint64_t monotonic_ms(void) {
    struct timespec now;
    (void)clock_gettime(CLOCK_MONOTONIC, &now);
    return (uint64_t)now.tv_sec * UINT64_C(1000) + (uint64_t)now.tv_nsec / UINT64_C(1000000);
}

static void set_error(char *error, size_t size, const char *message) {
    if (size != 0U) (void)snprintf(error, size, "%s", message);
}

static int set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return flags < 0 ? -1 : fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static int append_limited(uint8_t **buffer, size_t *used, size_t limit,
                          const uint8_t *data, size_t length) {
    size_t keep = length;
    uint8_t *next;
    if (*used >= limit) return 0;
    if (keep > limit - *used) keep = limit - *used;
    next = realloc(*buffer, *used + keep);
    if (next == NULL && keep != 0U) return -1;
    *buffer = next;
    if (keep != 0U) (void)memcpy(next + *used, data, keep);
    *used += keep;
    return 0;
}

static int drain_pipe(int fd, uint8_t **buffer, size_t *used, size_t limit,
                      bool *open) {
    uint8_t chunk[4096];
    for (;;) {
        ssize_t got = read(fd, chunk, sizeof(chunk));
        if (got > 0) {
            if (append_limited(buffer, used, limit, chunk, (size_t)got) != 0) return -1;
        } else if (got == 0) {
            (void)close(fd);
            *open = false;
            return 0;
        } else if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        else if (errno != EINTR) return -1;
    }
}

int cra_usb_execute_command(const uint8_t *payload, size_t payload_size,
                            uint32_t default_timeout_ms,
                            uint32_t maximum_stdout,
                            uint32_t maximum_stderr,
                            uint8_t **result, uint32_t *result_size,
                            char *error, size_t error_size) {
    uint32_t timeout;
    uint32_t stdout_limit;
    uint32_t stderr_limit;
    uint32_t command_length;
    char *command = NULL;
    int stdout_pipe[2] = {-1, -1};
    int stderr_pipe[2] = {-1, -1};
    uint8_t *stdout_data = NULL;
    uint8_t *stderr_data = NULL;
    size_t stdout_used = 0;
    size_t stderr_used = 0;
    bool stdout_open = true;
    bool stderr_open = true;
    bool timed_out = false;
    int status = 0;
    pid_t child;
    uint64_t started;

    if (payload_size < 16U) { set_error(error, error_size, "short command request"); return -1; }
    timeout = cra_usb_read_le32(payload);
    stdout_limit = cra_usb_read_le32(payload + 4U);
    stderr_limit = cra_usb_read_le32(payload + 8U);
    command_length = cra_usb_read_le32(payload + 12U);
    if ((size_t)command_length != payload_size - 16U || command_length == 0U ||
        memchr(payload + 16U, '\0', command_length) != NULL) {
        set_error(error, error_size, "invalid command request"); return -1;
    }
    if (timeout == 0U) timeout = default_timeout_ms;
    if (timeout == 0U || timeout > 300000U) timeout = 300000U;
    if (stdout_limit == 0U || stdout_limit > maximum_stdout) stdout_limit = maximum_stdout;
    if (stderr_limit == 0U || stderr_limit > maximum_stderr) stderr_limit = maximum_stderr;
    command = malloc((size_t)command_length + 1U);
    if (command == NULL) goto memory_error;
    (void)memcpy(command, payload + 16U, command_length);
    command[command_length] = '\0';
    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0) goto system_error;

    child = fork();
    if (child < 0) goto system_error;
    if (child == 0) {
        (void)setpgid(0, 0);
        (void)close(stdout_pipe[0]);
        (void)close(stderr_pipe[0]);
        if (dup2(stdout_pipe[1], STDOUT_FILENO) < 0 ||
            dup2(stderr_pipe[1], STDERR_FILENO) < 0) _exit(126);
        (void)close(stdout_pipe[1]);
        (void)close(stderr_pipe[1]);
        execl("/bin/sh", "sh", "-c", command, (char *)NULL);
        _exit(127);
    }

    free(command); command = NULL;
    (void)close(stdout_pipe[1]); stdout_pipe[1] = -1;
    (void)close(stderr_pipe[1]); stderr_pipe[1] = -1;
    if (set_nonblocking(stdout_pipe[0]) != 0 || set_nonblocking(stderr_pipe[0]) != 0)
        goto parent_system_error;
    started = monotonic_ms();
    while (stdout_open || stderr_open || waitpid(child, &status, WNOHANG) == 0) {
        struct pollfd fds[2];
        nfds_t count = 0;
        uint64_t elapsed = monotonic_ms() - started;
        if (!timed_out && elapsed >= timeout) {
            timed_out = true;
            (void)kill(-child, SIGTERM);
        }
        if (timed_out && elapsed >= (uint64_t)timeout + 250U) (void)kill(-child, SIGKILL);
        if (stdout_open) fds[count++] = (struct pollfd){stdout_pipe[0], POLLIN | POLLHUP, 0};
        if (stderr_open) fds[count++] = (struct pollfd){stderr_pipe[0], POLLIN | POLLHUP, 0};
        if (count != 0U) (void)poll(fds, count, 25);
        if (stdout_open && drain_pipe(stdout_pipe[0], &stdout_data, &stdout_used,
                                      stdout_limit, &stdout_open) != 0) goto parent_system_error;
        if (stderr_open && drain_pipe(stderr_pipe[0], &stderr_data, &stderr_used,
                                      stderr_limit, &stderr_open) != 0) goto parent_system_error;
        if (!stdout_open && !stderr_open) {
            pid_t waited = waitpid(child, &status, WNOHANG);
            if (waited == child) break;
        }
    }
    if (waitpid(child, &status, WNOHANG) == 0) (void)waitpid(child, &status, 0);
    {
        uint64_t duration = monotonic_ms() - started;
        size_t total = 20U + stdout_used + stderr_used;
        int32_t exit_code = WIFEXITED(status) ? WEXITSTATUS(status) :
                            WIFSIGNALED(status) ? 128 + WTERMSIG(status) : 255;
        uint8_t *encoded;
        if (total > UINT32_MAX) goto memory_error;
        encoded = malloc(total);
        if (encoded == NULL) goto memory_error;
        cra_usb_write_le32(encoded, (uint32_t)exit_code);
        encoded[4] = timed_out ? 1U : 0U;
        encoded[5] = encoded[6] = encoded[7] = 0U;
        cra_usb_write_le32(encoded + 8U, duration > UINT32_MAX ? UINT32_MAX : (uint32_t)duration);
        cra_usb_write_le32(encoded + 12U, (uint32_t)stdout_used);
        cra_usb_write_le32(encoded + 16U, (uint32_t)stderr_used);
        if (stdout_used != 0U) (void)memcpy(encoded + 20U, stdout_data, stdout_used);
        if (stderr_used != 0U) (void)memcpy(encoded + 20U + stdout_used, stderr_data, stderr_used);
        free(stdout_data); free(stderr_data);
        *result = encoded;
        *result_size = (uint32_t)total;
        return 0;
    }

parent_system_error:
    (void)kill(-child, SIGKILL);
    (void)waitpid(child, NULL, 0);
system_error:
    set_error(error, error_size, strerror(errno));
    if (stdout_pipe[0] >= 0) (void)close(stdout_pipe[0]);
    if (stdout_pipe[1] >= 0) (void)close(stdout_pipe[1]);
    if (stderr_pipe[0] >= 0) (void)close(stderr_pipe[0]);
    if (stderr_pipe[1] >= 0) (void)close(stderr_pipe[1]);
    free(command); free(stdout_data); free(stderr_data);
    return -1;
memory_error:
    set_error(error, error_size, "out of memory");
    free(command); free(stdout_data); free(stderr_data);
    return -1;
}
