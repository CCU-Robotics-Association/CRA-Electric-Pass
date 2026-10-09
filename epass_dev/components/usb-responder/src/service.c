/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#define _XOPEN_SOURCE 700
#include "cra_epass/usb_service.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <unistd.h>

static int set_response(cra_usb_response_t *response, uint16_t type,
                        const uint8_t *payload, size_t payload_length) {
    if (payload_length > UINT32_MAX) return -1;
    response->type = type;
    response->payload_length = (uint32_t)payload_length;
    response->payload = NULL;
    if (payload_length != 0U) {
        response->payload = malloc(payload_length);
        if (response->payload == NULL) return -1;
        (void)memcpy(response->payload, payload, payload_length);
    }
    return 0;
}

static int set_kv_response(cra_usb_response_t *response, uint16_t type,
                           cra_usb_kv_t *items, size_t count) {
    size_t size = 0;
    uint8_t *payload = NULL;
    if (cra_usb_encode_kv(items, count, &payload, &size) != 0 || size > UINT32_MAX) {
        free(payload); return -1;
    }
    response->type = type;
    response->payload = payload;
    response->payload_length = (uint32_t)size;
    return 0;
}

static int set_error_response(cra_usb_response_t *response, const char *message) {
    cra_usb_kv_t item = {"message", (char *)message};
    return set_kv_response(response, CRA_USB_ERROR, &item, 1);
}

static bool valid_relative_path(const char *path) {
    const char *component = path;
    const char *p;
    if (path == NULL || *path == '\0' || *path == '/' || strlen(path) >= PATH_MAX - 2U)
        return false;
    if (strcmp(path, ".") == 0) return true;
    for (p = path; ; ++p) {
        if (*p == '/' || *p == '\0') {
            size_t length = (size_t)(p - component);
            if (length == 0U || (length == 1U && component[0] == '.') ||
                (length == 2U && component[0] == '.' && component[1] == '.')) return false;
            if (*p == '\0') break;
            component = p + 1;
        }
    }
    return true;
}

static bool inside_root(const cra_usb_service_t *service, const char *path) {
    size_t root_length = strlen(service->root);
    if (strcmp(service->root, "/") == 0) return path[0] == '/';
    return strncmp(path, service->root, root_length) == 0 &&
           (path[root_length] == '\0' || path[root_length] == '/');
}

static int resolve_existing(const cra_usb_service_t *service, const char *relative,
                            char output[PATH_MAX]) {
    char joined[PATH_MAX];
    if (!valid_relative_path(relative)) { errno = EINVAL; return -1; }
    if (snprintf(joined, sizeof(joined), "%s/%s", service->root, relative) >= (int)sizeof(joined)) {
        errno = ENAMETOOLONG; return -1;
    }
    if (realpath(joined, output) == NULL || !inside_root(service, output)) {
        if (errno == 0) errno = EPERM;
        return -1;
    }
    return 0;
}

static int resolve_new(const cra_usb_service_t *service, const char *relative,
                       char output[PATH_MAX]) {
    char copy[PATH_MAX];
    char parent_relative[PATH_MAX];
    char parent[PATH_MAX];
    char *slash;
    const char *name;
    if (!valid_relative_path(relative) || strcmp(relative, ".") == 0) { errno = EINVAL; return -1; }
    (void)snprintf(copy, sizeof(copy), "%s", relative);
    slash = strrchr(copy, '/');
    if (slash == NULL) { (void)snprintf(parent_relative, sizeof(parent_relative), "."); name = copy; }
    else { *slash = '\0'; (void)snprintf(parent_relative, sizeof(parent_relative), "%s", copy); name = slash + 1; }
    if (resolve_existing(service, parent_relative, parent) != 0) return -1;
    if (snprintf(output, PATH_MAX, "%s/%s", parent, name) >= PATH_MAX) { errno = ENAMETOOLONG; return -1; }
    return 0;
}

static bool path_is_sd(const char *relative) {
    return strcmp(relative, "sd") == 0 || strncmp(relative, "sd/", 3U) == 0;
}

static bool sd_is_mmc_mounted(void) {
    FILE *mounts = fopen("/proc/self/mounts", "r");
    char source[256];
    char target[256];
    char type[64];
    bool mounted = false;
    if (mounts == NULL) return false;
    while (fscanf(mounts, "%255s %255s %63s %*s %*d %*d", source, target, type) == 3) {
        if (strcmp(target, "/sd") == 0 && strstr(source, "mmc") != NULL) { mounted = true; break; }
    }
    (void)fclose(mounts);
    return mounted;
}

static int validate_storage(const cra_usb_service_t *service, const char *relative,
                            const char *desired) {
    bool sd = path_is_sd(relative);
    if (desired != NULL && ((strcmp(desired, "sd") == 0) != sd)) { errno = EXDEV; return -1; }
    if (desired != NULL && strcmp(desired, "sd") != 0 && strcmp(desired, "nand") != 0) {
        errno = EINVAL; return -1;
    }
    if (sd && strcmp(service->root, "/") == 0 && !sd_is_mmc_mounted()) { errno = ENODEV; return -1; }
    return 0;
}

static int decode_request_kv(const cra_usb_frame_t *request, cra_usb_kv_t *items,
                             size_t *count, cra_usb_response_t *response) {
    if (cra_usb_decode_kv(request->payload, request->payload_length, items,
                          CRA_USB_MAX_KV_ITEMS, count) != 0) {
        (void)set_error_response(response, "invalid key/value payload");
        return -1;
    }
    return 0;
}

static int status_ok(cra_usb_response_t *response) {
    cra_usb_kv_t item = {"status", "ok"};
    return set_kv_response(response, CRA_USB_STATUS, &item, 1);
}

static int recursive_delete(const char *path) {
    struct stat info;
    if (lstat(path, &info) != 0) return -1;
    if (!S_ISDIR(info.st_mode) || S_ISLNK(info.st_mode)) return unlink(path);
    DIR *directory = opendir(path);
    struct dirent *entry;
    if (directory == NULL) return -1;
    while ((entry = readdir(directory)) != NULL) {
        char child[PATH_MAX];
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) >= (int)sizeof(child) ||
            recursive_delete(child) != 0) { int saved = errno; (void)closedir(directory); errno = saved; return -1; }
    }
    (void)closedir(directory);
    return rmdir(path);
}

static int append_name(char **text, size_t *used, const char *name) {
    size_t length = strlen(name);
    char *next = realloc(*text, *used + length + 2U);
    if (next == NULL) return -1;
    *text = next;
    (void)memcpy(next + *used, name, length);
    next[*used + length] = '\n';
    next[*used + length + 1U] = '\0';
    *used += length + 1U;
    return 0;
}

static int handle_file_list(cra_usb_service_t *service, const char *relative,
                            cra_usb_response_t *response) {
    char path[PATH_MAX];
    char *files = calloc(1, 1U);
    char *dirs = calloc(1, 1U);
    size_t files_used = 0, dirs_used = 0;
    DIR *directory;
    struct dirent *entry;
    if (files == NULL || dirs == NULL || resolve_existing(service, relative, path) != 0) goto fail;
    directory = opendir(path);
    if (directory == NULL) goto fail;
    while ((entry = readdir(directory)) != NULL) {
        char child[PATH_MAX];
        struct stat info;
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (snprintf(child, sizeof(child), "%s/%s", path, entry->d_name) >= (int)sizeof(child) ||
            lstat(child, &info) != 0 ||
            (S_ISDIR(info.st_mode) ? append_name(&dirs, &dirs_used, entry->d_name) :
                                    append_name(&files, &files_used, entry->d_name)) != 0) {
            (void)closedir(directory); goto fail;
        }
    }
    (void)closedir(directory);
    cra_usb_kv_t items[] = {{"files", files}, {"dirs", dirs}};
    int result = set_kv_response(response, CRA_USB_STATUS, items, 2);
    free(files); free(dirs);
    return result;
fail:
    { char message[192]; (void)snprintf(message, sizeof(message), "list failed: %s", strerror(errno));
      free(files); free(dirs); return set_error_response(response, message); }
}

static int handle_stat(cra_usb_service_t *service, const char *relative,
                       cra_usb_response_t *response) {
    char path[PATH_MAX], owner[128], permissions[16], size[32];
    const char *type = "other";
    struct stat info;
    struct passwd *user;
    struct group *group;
    if (resolve_new(service, relative, path) != 0 || lstat(path, &info) != 0) goto fail;
    user = getpwuid(info.st_uid); group = getgrgid(info.st_gid);
    if (user != NULL && group != NULL) (void)snprintf(owner, sizeof(owner), "%s:%s", user->pw_name, group->gr_name);
    else (void)snprintf(owner, sizeof(owner), "%lu:%lu", (unsigned long)info.st_uid, (unsigned long)info.st_gid);
    (void)snprintf(permissions, sizeof(permissions), "%04o", (unsigned int)(info.st_mode & 07777U));
    (void)snprintf(size, sizeof(size), "%lld", (long long)info.st_size);
    if (S_ISREG(info.st_mode)) type = "file"; else if (S_ISDIR(info.st_mode)) type = "dir";
    else if (S_ISLNK(info.st_mode)) type = "link";
    cra_usb_kv_t items[] = {{"owner", owner}, {"perm", permissions}, {"size", size}, {"type", (char *)type}};
    return set_kv_response(response, CRA_USB_STATUS, items, 4);
fail:
    { char message[192]; (void)snprintf(message, sizeof(message), "stat failed: %s", strerror(errno));
      return set_error_response(response, message); }
}

static int mkdir_parents(cra_usb_service_t *service, const char *relative) {
    char copy[PATH_MAX];
    char *cursor;
    if (!valid_relative_path(relative) || strcmp(relative, ".") == 0) { errno = EINVAL; return -1; }
    (void)snprintf(copy, sizeof(copy), "%s", relative);
    for (cursor = copy; ; ++cursor) {
        if (*cursor == '/' || *cursor == '\0') {
            char saved = *cursor;
            char path[PATH_MAX];
            *cursor = '\0';
            if (resolve_new(service, copy, path) == 0) {
                if (mkdir(path, 0755) != 0 && errno != EEXIST) return -1;
            } else if (errno != ENOENT) return -1;
            *cursor = saved;
            if (saved == '\0') break;
        }
    }
    return 0;
}

static int handle_kv_file_operation(cra_usb_service_t *service,
                                    const cra_usb_frame_t *request,
                                    cra_usb_response_t *response) {
    cra_usb_kv_t items[CRA_USB_MAX_KV_ITEMS];
    size_t count = 0;
    const char *path;
    const char *desired;
    char resolved[PATH_MAX];
    int result = -1;
    if (decode_request_kv(request, items, &count, response) != 0) return 0;
    path = cra_usb_find_kv(items, count, "path");
    desired = cra_usb_find_kv(items, count, "desire_storage");
    if (path == NULL) { result = set_error_response(response, "missing path"); goto done; }
    if (request->type == CRA_USB_FILE_LIST) result = handle_file_list(service, path, response);
    else if (request->type == CRA_USB_FILE_STAT) result = handle_stat(service, path, response);
    else if (request->type == CRA_USB_FILE_GET) {
        struct stat info;
        int fd = -1;
        uint8_t *data = NULL;
        if (resolve_existing(service, path, resolved) != 0 || stat(resolved, &info) != 0)
            goto file_error;
        if (!S_ISREG(info.st_mode) || info.st_size < 0) { errno = EINVAL; goto file_error; }
        if ((uint64_t)info.st_size > service->maximum_file_response) { errno = EFBIG; goto file_error; }
        fd = open(resolved, O_RDONLY | O_CLOEXEC);
        if (fd < 0) goto file_error;
        data = malloc((size_t)info.st_size);
        if (data == NULL && info.st_size != 0) { errno = ENOMEM; (void)close(fd); goto file_error; }
        size_t used = 0;
        while (used < (size_t)info.st_size) {
            ssize_t got = read(fd, data + used, (size_t)info.st_size - used);
            if (got < 0 && errno == EINTR) continue;
            if (got <= 0) { if (got == 0) errno = EIO; (void)close(fd); free(data); goto file_error; }
            used += (size_t)got;
        }
        (void)close(fd);
        result = set_response(response, CRA_USB_FILE_GET, data, used);
        free(data);
    } else if (request->type == CRA_USB_FILE_DELETE) {
        if (validate_storage(service, path, desired) != 0 || resolve_new(service, path, resolved) != 0 ||
            recursive_delete(resolved) != 0) goto file_error;
        result = status_ok(response);
    } else if (request->type == CRA_USB_FILE_MKDIR) {
        const char *parents = cra_usb_find_kv(items, count, "parents");
        bool recursive = parents != NULL && (strcmp(parents, "1") == 0 || strcasecmp(parents, "true") == 0 || strcasecmp(parents, "yes") == 0);
        if (validate_storage(service, path, desired) != 0) goto file_error;
        if ((recursive ? mkdir_parents(service, path) :
             (resolve_new(service, path, resolved) == 0 ? mkdir(resolved, 0755) : -1)) != 0) goto file_error;
        result = status_ok(response);
    }
    goto done;
file_error:
    { char message[192]; (void)snprintf(message, sizeof(message), "file operation failed: %s", strerror(errno));
      result = set_error_response(response, message); }
done:
    cra_usb_free_kv(items, count);
    return result;
}

static int handle_rename(cra_usb_service_t *service, const cra_usb_frame_t *request,
                         cra_usb_response_t *response) {
    cra_usb_kv_t items[CRA_USB_MAX_KV_ITEMS]; size_t count = 0;
    char from_path[PATH_MAX], to_path[PATH_MAX];
    int result;
    if (decode_request_kv(request, items, &count, response) != 0) return 0;
    const char *from = cra_usb_find_kv(items, count, "from");
    const char *to = cra_usb_find_kv(items, count, "to");
    const char *desired = cra_usb_find_kv(items, count, "desire_storage");
    if (from != NULL && to != NULL && path_is_sd(from) != path_is_sd(to)) errno = EXDEV;
    if (from == NULL || to == NULL || path_is_sd(from) != path_is_sd(to) ||
        validate_storage(service, to, desired) != 0 || resolve_new(service, from, from_path) != 0 ||
        resolve_new(service, to, to_path) != 0 || rename(from_path, to_path) != 0) {
        char message[192]; (void)snprintf(message, sizeof(message), "rename failed: %s", strerror(errno));
        result = set_error_response(response, message);
    } else result = status_ok(response);
    cra_usb_free_kv(items, count);
    return result;
}

static int handle_put_begin(cra_usb_service_t *service, const cra_usb_frame_t *request,
                            cra_usb_response_t *response) {
    cra_usb_kv_t items[CRA_USB_MAX_KV_ITEMS]; size_t count = 0;
    const char *path, *desired, *mode_text;
    char final_path[PATH_MAX];
    unsigned long mode = 0644;
    int result;
    if (decode_request_kv(request, items, &count, response) != 0) return 0;
    path = cra_usb_find_kv(items, count, "path"); desired = cra_usb_find_kv(items, count, "desire_storage");
    mode_text = cra_usb_find_kv(items, count, "perm");
    if (mode_text != NULL) { char *end; errno = 0; mode = strtoul(mode_text, &end, 8); if (errno || *end || mode > 07777U) { errno = EINVAL; goto fail; } }
    if (path == NULL || validate_storage(service, path, desired) != 0 || resolve_new(service, path, final_path) != 0) goto fail;
    if (service->upload_fd >= 0) { (void)close(service->upload_fd); (void)unlink(service->upload_temporary); }
    if (snprintf(service->upload_temporary, sizeof(service->upload_temporary), "%s.cra-part-%08x", final_path, request->request_id) >= (int)sizeof(service->upload_temporary)) { errno = ENAMETOOLONG; goto fail; }
    service->upload_fd = open(service->upload_temporary, O_WRONLY | O_CREAT | O_EXCL | O_CLOEXEC, 0600);
    if (service->upload_fd < 0) goto fail;
    (void)snprintf(service->upload_final, sizeof(service->upload_final), "%s", final_path);
    service->upload_id = request->request_id; service->upload_mode = (unsigned int)mode;
    result = status_ok(response); goto done;
fail:
    { char message[192]; (void)snprintf(message, sizeof(message), "begin upload failed: %s", strerror(errno));
      result = set_error_response(response, message); }
done:
    cra_usb_free_kv(items, count); return result;
}

static int handle_put_chunk(cra_usb_service_t *service, const cra_usb_frame_t *request,
                            cra_usb_response_t *response) {
    const uint8_t *data = request->payload;
    size_t length = request->payload_length;
    if (service->upload_fd < 0) return set_error_response(response, "no active upload");
    if (length >= 4U && cra_usb_read_le32(data) == service->upload_id) { data += 4U; length -= 4U; }
    else if (request->request_id != service->upload_id) return set_error_response(response, "upload id mismatch");
    while (length != 0U) { ssize_t written = write(service->upload_fd, data, length); if (written < 0) { if (errno == EINTR) continue; goto fail; } data += written; length -= (size_t)written; }
    return status_ok(response);
fail:
    { char message[192]; (void)snprintf(message, sizeof(message), "upload write failed: %s", strerror(errno)); return set_error_response(response, message); }
}

static int handle_put_end(cra_usb_service_t *service, const cra_usb_frame_t *request,
                          cra_usb_response_t *response) {
    uint32_t id = request->request_id;
    if (request->payload_length == 4U) id = cra_usb_read_le32(request->payload);
    if (service->upload_fd < 0 || id != service->upload_id) return set_error_response(response, "upload id mismatch");
    if (fsync(service->upload_fd) != 0 || close(service->upload_fd) != 0) { service->upload_fd = -1; goto fail; }
    service->upload_fd = -1;
    if (chmod(service->upload_temporary, service->upload_mode) != 0 ||
        rename(service->upload_temporary, service->upload_final) != 0) goto fail;
    service->upload_temporary[0] = '\0'; service->upload_final[0] = '\0';
    return status_ok(response);
fail:
    { char message[192]; (void)unlink(service->upload_temporary); service->upload_temporary[0] = '\0';
      (void)snprintf(message, sizeof(message), "finish upload failed: %s", strerror(errno)); return set_error_response(response, message); }
}

static char *read_small_file(const char *path, char *buffer, size_t capacity) {
    FILE *file = fopen(path, "r");
    size_t size;
    if (file == NULL) { buffer[0] = '\0'; return buffer; }
    size = fread(buffer, 1, capacity - 1U, file); (void)fclose(file);
    while (size != 0U && (buffer[size - 1U] == '\0' || buffer[size - 1U] == '\n' || buffer[size - 1U] == '\r' || buffer[size - 1U] == ' ')) --size;
    buffer[size] = '\0'; return buffer;
}

static char *read_app_version(char *buffer, size_t capacity) {
    static const char command[] = "/root/epass_drm_app version";
    uint8_t payload[16U + sizeof(command) - 1U];
    uint8_t *result = NULL;
    uint32_t result_size = 0;
    char error[64];
    uint32_t output_size;
    buffer[0] = '\0';
    cra_usb_write_le32(payload, 3000U);
    cra_usb_write_le32(payload + 4U, (uint32_t)(capacity - 1U));
    cra_usb_write_le32(payload + 8U, 256U);
    cra_usb_write_le32(payload + 12U, sizeof(command) - 1U);
    (void)memcpy(payload + 16U, command, sizeof(command) - 1U);
    if (cra_usb_execute_command(payload, sizeof(payload), 3000U,
                                (uint32_t)(capacity - 1U), 256U,
                                &result, &result_size, error, sizeof(error)) != 0 ||
        result_size < 20U || cra_usb_read_le32(result) != 0U || result[4] != 0U) {
        free(result); return buffer;
    }
    output_size = cra_usb_read_le32(result + 12U);
    if (output_size > result_size - 20U) output_size = result_size - 20U;
    if (output_size >= capacity) output_size = (uint32_t)capacity - 1U;
    (void)memcpy(buffer, result + 20U, output_size);
    while (output_size != 0U && (buffer[output_size - 1U] == '\n' ||
           buffer[output_size - 1U] == '\r' || buffer[output_size - 1U] == ' ')) --output_size;
    buffer[output_size] = '\0';
    free(result); return buffer;
}

static int handle_devinfo(cra_usb_response_t *response) {
    struct utsname uts;
    struct statvfs nand_info, sd_info;
    char model[256], rootfs[1024], app[1024], nand_total[32], nand_free[32], sd_total[32], sd_free[32];
    bool sd = sd_is_mmc_mounted();
    (void)memset(&uts, 0, sizeof(uts));
    (void)uname(&uts);
    (void)memset(&nand_info, 0, sizeof(nand_info));
    (void)statvfs("/", &nand_info);
    (void)memset(&sd_info, 0, sizeof(sd_info)); if (sd) (void)statvfs("/sd", &sd_info);
    (void)snprintf(nand_total, sizeof(nand_total), "%llu", (unsigned long long)nand_info.f_blocks * nand_info.f_frsize);
    (void)snprintf(nand_free, sizeof(nand_free), "%llu", (unsigned long long)nand_info.f_bavail * nand_info.f_frsize);
    (void)snprintf(sd_total, sizeof(sd_total), "%llu", (unsigned long long)sd_info.f_blocks * sd_info.f_frsize);
    (void)snprintf(sd_free, sizeof(sd_free), "%llu", (unsigned long long)sd_info.f_bavail * sd_info.f_frsize);
    cra_usb_kv_t items[] = {
        {"model", read_small_file("/proc/device-tree/model", model, sizeof(model))},
        {"kernel", uts.release}, {"rootfs", read_small_file("/etc/os-release", rootfs, sizeof(rootfs))},
        {"app", read_app_version(app, sizeof(app))}, {"sd_mounted", sd ? "1" : "0"},
        {"nand_total_bytes", nand_total}, {"nand_free_bytes", nand_free},
        {"sd_total_bytes", sd_total}, {"sd_free_bytes", sd_free}
    };
    return set_kv_response(response, CRA_USB_DEVINFO, items, sizeof(items) / sizeof(items[0]));
}

int cra_usb_service_init(cra_usb_service_t *service, const char *root) {
    (void)memset(service, 0, sizeof(*service)); service->upload_fd = -1;
    if (realpath(root, service->root) == NULL) return -1;
    service->allow_command = true;
    service->default_timeout_ms = 30000U;
    service->maximum_stdout = 1024U * 1024U;
    service->maximum_stderr = 1024U * 1024U;
    service->maximum_file_response = 8U * 1024U * 1024U;
    return 0;
}

void cra_usb_service_destroy(cra_usb_service_t *service) {
    if (service->upload_fd >= 0) (void)close(service->upload_fd);
    if (service->upload_temporary[0] != '\0') (void)unlink(service->upload_temporary);
    service->upload_fd = -1;
}

void cra_usb_response_destroy(cra_usb_response_t *response) {
    free(response->payload); (void)memset(response, 0, sizeof(*response));
}

int cra_usb_service_handle(cra_usb_service_t *service, const cra_usb_frame_t *request,
                           cra_usb_response_t *response) {
    int result;
    (void)memset(response, 0, sizeof(*response)); response->request_id = request->request_id;
    switch (request->type) {
    case CRA_USB_HELLO: {
        cra_usb_kv_t items[] = {{"service", "cra_usb_responder"}, {"version", "1"}};
        result = set_kv_response(response, CRA_USB_STATUS, items, 2); break;
    }
    case CRA_USB_FILE_PUT_BEGIN: result = handle_put_begin(service, request, response); break;
    case CRA_USB_FILE_PUT_CHUNK: result = handle_put_chunk(service, request, response); break;
    case CRA_USB_FILE_PUT_END: result = handle_put_end(service, request, response); break;
    case CRA_USB_FILE_GET: case CRA_USB_FILE_LIST: case CRA_USB_FILE_DELETE:
    case CRA_USB_FILE_MKDIR: case CRA_USB_FILE_STAT:
        result = handle_kv_file_operation(service, request, response); break;
    case CRA_USB_FILE_RENAME: result = handle_rename(service, request, response); break;
    case CRA_USB_COMMAND_EXEC:
        if (!service->allow_command) result = set_error_response(response, "command execution disabled");
        else {
            char error[192]; response->type = CRA_USB_COMMAND_RESULT;
            result = cra_usb_execute_command(request->payload, request->payload_length,
                service->default_timeout_ms, service->maximum_stdout, service->maximum_stderr,
                &response->payload, &response->payload_length, error, sizeof(error));
            if (result != 0) result = set_error_response(response, error);
        }
        break;
    case CRA_USB_DEVINFO: result = handle_devinfo(response); break;
    default: result = set_error_response(response, "unsupported message type"); break;
    }
    return result;
}
