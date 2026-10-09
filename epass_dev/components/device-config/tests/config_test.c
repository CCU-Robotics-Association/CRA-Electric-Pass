/* SPDX-License-Identifier: GPL-3.0-or-later */

#define _POSIX_C_SOURCE 200809L
#include "cra_epass/config_state.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    char path[] = "/tmp/cra-device-config-XXXXXX";
    static const char original[] =
        "# keep this comment exactly\nbootargs=test value\ninterface=i2c0 unknown_if\next=cardkb unknown_ext\n";
    cra_uenv_t uenv, reloaded;
    cra_config_state_t state;
    char error[256];
    char *interfaces, *extensions;
    int fd = mkstemp(path);
    assert(fd >= 0);
    assert(write(fd, original, sizeof(original) - 1U) == (ssize_t)(sizeof(original) - 1U));
    assert(close(fd) == 0);
    assert(cra_uenv_load(path, &uenv, error, sizeof(error)) == 0);
    assert(cra_config_state_init(&state, &uenv) == 0);
    assert(state.enabled[cra_config_find_option("i2c0")]);
    assert(state.enabled[cra_config_find_option("cardkb")]);

    assert(cra_config_enable(&state, (size_t)cra_config_find_option("i2s0_pa"),
                             true, error, sizeof(error)) == 0);
    assert(cra_config_enable(&state, (size_t)cra_config_find_option("es8311_sound"),
                             true, error, sizeof(error)) == 0);
    assert(!state.enabled[cra_config_find_option("i2c0")]);
    assert(!state.enabled[cra_config_find_option("cardkb")]);
    assert(cra_config_disable(&state, (size_t)cra_config_find_option("i2s0_pa"),
                              error, sizeof(error)) != 0);
    assert(cra_config_enable(&state, (size_t)cra_config_find_option("lsm6ds3_pre0.4"),
                             true, error, sizeof(error)) != 0);

    interfaces = cra_config_build_line(&state, CRA_CONFIG_INTERFACE);
    extensions = cra_config_build_line(&state, CRA_CONFIG_EXTENSION);
    assert(interfaces != NULL && extensions != NULL);
    assert(strstr(interfaces, "i2s0_pa") != NULL && strstr(interfaces, "unknown_if") != NULL);
    assert(strstr(extensions, "es8311_sound") != NULL && strstr(extensions, "unknown_ext") != NULL);
    assert(cra_uenv_save(path, &uenv, interfaces, extensions, error, sizeof(error)) == 0);
    free(interfaces); free(extensions);
    cra_config_state_destroy(&state); cra_uenv_destroy(&uenv);

    assert(cra_uenv_load(path, &reloaded, error, sizeof(error)) == 0);
    assert(strcmp(reloaded.lines[0], "# keep this comment exactly\n") == 0);
    assert(strcmp(reloaded.lines[1], "bootargs=test value\n") == 0);
    cra_uenv_destroy(&reloaded);
    assert(unlink(path) == 0);
    return 0;
}
