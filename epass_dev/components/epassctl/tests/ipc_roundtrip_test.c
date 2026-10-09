/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "cra_epass/ipc_client.h"

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

typedef struct {
    int listener;
    uint32_t expected;
    uint32_t response;
} server_args_t;

static void *serve_one(void *opaque) {
    server_args_t *args = opaque;
    uint32_t request = 0;
    int client = accept(args->listener, NULL, NULL);
    assert(client >= 0);
    assert(recv(client, &request, sizeof(request), 0) == (ssize_t)sizeof(request));
    assert(request == args->expected);
    assert(send(client, &args->response, sizeof(args->response), 0) ==
           (ssize_t)sizeof(args->response));
    assert(close(client) == 0);
    return NULL;
}

int main(void) {
    char path[sizeof(((struct sockaddr_un *)0)->sun_path)];
    char error[128] = {0};
    struct sockaddr_un address;
    server_args_t args = {0};
    pthread_t thread;
    uint32_t request = UINT32_C(0x43524151);
    uint32_t response = 0;
    size_t response_size = 0;

    (void)snprintf(path, sizeof(path), "/tmp/cra-epassctl-test-%ld.sock", (long)getpid());
    args.listener = socket(AF_UNIX, SOCK_SEQPACKET | SOCK_CLOEXEC, 0);
    assert(args.listener >= 0);
    args.expected = request;
    args.response = UINT32_C(0x43524152);

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    (void)memcpy(address.sun_path, path, strlen(path) + 1U);
    (void)unlink(path);
    assert(bind(args.listener, (const struct sockaddr *)&address, sizeof(address)) == 0);
    assert(listen(args.listener, 1) == 0);
    assert(pthread_create(&thread, NULL, serve_one, &args) == 0);

    assert(cra_ipc_exchange(path, &request, sizeof(request), &response,
                            sizeof(response), &response_size, error, sizeof(error)) == 0);
    assert(response_size == sizeof(response));
    assert(response == args.response);
    assert(pthread_join(thread, NULL) == 0);
    assert(close(args.listener) == 0);
    assert(unlink(path) == 0);
    return 0;
}
