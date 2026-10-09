/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#define _POSIX_C_SOURCE 200809L
#include "cra_epass/config_state.h"

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define DEVICE_METADATA_OFFSET 0xFA000

static struct termios saved_terminal;
static bool terminal_is_raw;

static void restore_terminal(void) {
    if (terminal_is_raw) (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &saved_terminal);
    terminal_is_raw = false;
    (void)fputs("\033[?25h\033[0m", stdout); (void)fflush(stdout);
}

static int raw_terminal(void) {
    struct termios mode;
    if (!isatty(STDIN_FILENO) || tcgetattr(STDIN_FILENO, &saved_terminal) != 0) return -1;
    mode = saved_terminal;
    mode.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    mode.c_iflag &= (tcflag_t)~(IXON | ICRNL);
    mode.c_cc[VMIN] = 1; mode.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &mode) != 0) return -1;
    terminal_is_raw = true; (void)atexit(restore_terminal); return 0;
}

enum key { KEY_OTHER, KEY_UP, KEY_DOWN, KEY_SELECT, KEY_SAVE, KEY_QUIT };

static enum key read_key(void) {
    unsigned char ch;
    if (read(STDIN_FILENO, &ch, 1) != 1) return KEY_QUIT;
    if (ch == '\r' || ch == '\n' || ch == ' ') return KEY_SELECT;
    if (ch == 's' || ch == 'S') return KEY_SAVE;
    if (ch == 'q' || ch == 'Q' || ch == 4U) return KEY_QUIT;
    if (ch == 'k' || ch == 'K') return KEY_UP;
    if (ch == 'j' || ch == 'J') return KEY_DOWN;
    if (ch == 0x1bU) {
        unsigned char sequence[2];
        if (read(STDIN_FILENO, sequence, 2) == 2 && sequence[0] == '[') {
            if (sequence[1] == 'A') return KEY_UP;
            if (sequence[1] == 'B') return KEY_DOWN;
        }
    }
    return KEY_OTHER;
}

static void detect_device(char *description, size_t size) {
    int fd = open("/dev/mtdblock0", O_RDONLY | O_CLOEXEC);
    char data[1025];
    ssize_t received = -1;
    const char *revision = NULL;
    const char *screen = NULL;
    if (fd >= 0 && lseek(fd, DEVICE_METADATA_OFFSET, SEEK_SET) >= 0 &&
        (received = read(fd, data, sizeof(data) - 1U)) > 0) {
        data[received] = '\0';
        revision = strstr(data, "device_rev=");
        screen = strstr(data, "screen=");
    }
    if (fd >= 0) (void)close(fd);
    if (revision != NULL && strncmp(revision + 11U, "0.6", 3U) == 0) {
        char screen_name[16] = "unknown";
        if (screen != NULL) (void)sscanf(screen + 7U, "%15s", screen_name);
        (void)snprintf(description, size, "CRA Electric Pass v0.6 · screen %s", screen_name);
    } else (void)snprintf(description, size, "CRA Electric Pass v0.6 · metadata unavailable");
}

static int save_config(const char *path, const cra_uenv_t *uenv,
                       const cra_config_state_t *state, char *message, size_t size) {
    char *interfaces = cra_config_build_line(state, CRA_CONFIG_INTERFACE);
    char *extensions = cra_config_build_line(state, CRA_CONFIG_EXTENSION);
    int result;
    if (interfaces == NULL || extensions == NULL) {
        free(interfaces); free(extensions); (void)snprintf(message, size, "out of memory"); return -1;
    }
    result = cra_uenv_save(path, uenv, interfaces, extensions, message, size);
    free(interfaces); free(extensions);
    if (result == 0) (void)snprintf(message, size, "Saved. Reboot to apply the new overlays.");
    return result;
}

static bool confirm_conflicts(const char *id) {
    (void)printf("\033[2J\033[H\033[1;31mConflict detected\033[0m\n\n"
                 "Enabling %s requires disabling conflicting options.\n"
                 "Continue? [y/N] ", id);
    (void)fflush(stdout);
    for (;;) {
        unsigned char ch;
        if (read(STDIN_FILENO, &ch, 1) != 1) return false;
        if (ch == 'y' || ch == 'Y') return true;
        if (ch == 'n' || ch == 'N' || ch == '\r' || ch == '\n' || ch == 0x1bU) return false;
    }
}

static void render(const cra_config_state_t *state, size_t selected,
                   const char *device, const char *path, const char *message,
                   bool dirty) {
    size_t count, i;
    const cra_config_option_t *options = cra_config_options(&count);
    (void)printf("\033[2J\033[H\033[?25l\033[1;35mCRA DEVICE CONFIG\033[0m\n"
                 "%s\nConfig: %s%s\n\n", device, path, dirty ? "  [modified]" : "");
    for (i = 0; i < count; ++i) {
        const char *category = options[i].category == CRA_CONFIG_INTERFACE ? "IF " : "EXT";
        (void)printf("%s%s [%c] %-17s \033[2m%s%s\033[0m\n",
            i == selected ? "\033[7m" : "", category,
            state->enabled[i] ? 'x' : ' ', options[i].id, options[i].description,
            options[i].available_on_v06 ? "" : " (unavailable on v0.6)");
        if (i == selected) (void)fputs("\033[0m", stdout);
    }
    (void)printf("\n\033[36m↑/↓ or j/k\033[0m move · \033[36mEnter/Space\033[0m toggle · "
                 "\033[36mS\033[0m save · \033[36mQ\033[0m exit\n%s\n", message);
    (void)fflush(stdout);
}

static int interactive(const char *path, const cra_uenv_t *uenv,
                       cra_config_state_t *state) {
    size_t count, selected = 0;
    char device[128], message[256] = "Only overlays present in this repository are listed.";
    bool dirty = false;
    (void)cra_config_options(&count); detect_device(device, sizeof(device));
    if (raw_terminal() != 0) { (void)fprintf(stderr, "interactive mode requires a terminal\n"); return 1; }
    for (;;) {
        enum key key;
        render(state, selected, device, path, message, dirty);
        key = read_key();
        if (key == KEY_UP) selected = selected == 0U ? count - 1U : selected - 1U;
        else if (key == KEY_DOWN) selected = (selected + 1U) % count;
        else if (key == KEY_SELECT) {
            const cra_config_option_t *options = cra_config_options(&count);
            int result;
            if (state->enabled[selected]) result = cra_config_disable(state, selected, message, sizeof(message));
            else {
                result = cra_config_enable(state, selected, false, message, sizeof(message));
                if (result == 1 && confirm_conflicts(options[selected].id))
                    result = cra_config_enable(state, selected, true, message, sizeof(message));
            }
            if (result == 0) { dirty = true; (void)snprintf(message, sizeof(message), "%s %s.",
                options[selected].id, state->enabled[selected] ? "enabled" : "disabled"); }
        } else if (key == KEY_SAVE) {
            if (save_config(path, uenv, state, message, sizeof(message)) == 0) dirty = false;
        } else if (key == KEY_QUIT) {
            if (!dirty) break;
            (void)printf("\033[2J\033[HUnsaved changes. Exit anyway? [y/N] "); (void)fflush(stdout);
            unsigned char ch; if (read(STDIN_FILENO, &ch, 1) == 1 && (ch == 'y' || ch == 'Y')) break;
        }
    }
    restore_terminal(); return 0;
}

static void usage(const char *program) {
    (void)fprintf(stderr,
        "usage: %s [--uenv PATH] [list|show|enable ID|disable ID]\n"
        "With no command, an interactive v0.6 configuration screen is opened.\n", program);
}

int main(int argc, char **argv) {
    const char *path = getenv("CRA_UENV_PATH");
    const char *command = NULL, *id = NULL;
    cra_uenv_t uenv;
    cra_config_state_t state;
    char error[256];
    int i, result = 0;
    if (path == NULL || *path == '\0') path = "/boot/uEnv.txt";
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--uenv") == 0 && i + 1 < argc) path = argv[++i];
        else if (command == NULL) command = argv[i];
        else if (id == NULL) id = argv[i];
        else { usage(argv[0]); return 1; }
    }
    if (cra_uenv_load(path, &uenv, error, sizeof(error)) != 0) { (void)fprintf(stderr, "error: %s\n", error); return 1; }
    if (cra_config_state_init(&state, &uenv) != 0) { cra_uenv_destroy(&uenv); (void)fprintf(stderr, "error: out of memory\n"); return 1; }
    if (command == NULL) result = interactive(path, &uenv, &state);
    else if (strcmp(command, "list") == 0) {
        size_t count, n; const cra_config_option_t *options = cra_config_options(&count);
        for (n = 0; n < count; ++n) (void)printf("%s\t%s\t%s\n", options[n].id,
            options[n].category == CRA_CONFIG_INTERFACE ? "interface" : "extension",
            options[n].available_on_v06 ? "available" : "unavailable");
    } else if (strcmp(command, "show") == 0) {
        char *interfaces = cra_config_build_line(&state, CRA_CONFIG_INTERFACE);
        char *extensions = cra_config_build_line(&state, CRA_CONFIG_EXTENSION);
        if (interfaces == NULL || extensions == NULL) result = 1;
        else (void)printf("interface=%s\next=%s\n", interfaces, extensions);
        free(interfaces); free(extensions);
    } else if ((strcmp(command, "enable") == 0 || strcmp(command, "disable") == 0) && id != NULL) {
        int index = cra_config_find_option(id);
        if (index < 0) { (void)fprintf(stderr, "error: unknown option %s\n", id); result = 1; }
        else {
            int changed = strcmp(command, "enable") == 0 ?
                cra_config_enable(&state, (size_t)index, true, error, sizeof(error)) :
                cra_config_disable(&state, (size_t)index, error, sizeof(error));
            if (changed != 0 || save_config(path, &uenv, &state, error, sizeof(error)) != 0) {
                (void)fprintf(stderr, "error: %s\n", error); result = 1;
            } else (void)puts(error);
        }
    } else { usage(argv[0]); result = 1; }
    cra_config_state_destroy(&state); cra_uenv_destroy(&uenv); return result;
}
