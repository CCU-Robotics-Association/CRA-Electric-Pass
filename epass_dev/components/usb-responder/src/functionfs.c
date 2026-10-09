/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#define _GNU_SOURCE
#include "cra_epass/functionfs.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/usb/ch9.h>
#include <linux/usb/functionfs.h>
#include <poll.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define IO_CHUNK 16384U

typedef struct { uint8_t *data; size_t used; size_t capacity; } byte_buffer_t;

static void log_message(bool verbose, const char *format, ...) {
    va_list args;
    if (!verbose) return;
    va_start(args, format); (void)vfprintf(stderr, format, args); va_end(args);
    (void)fputc('\n', stderr);
}

static int append(byte_buffer_t *buffer, const void *data, size_t size) {
    if (size > SIZE_MAX - buffer->used) return -1;
    if (buffer->used + size > buffer->capacity) {
        size_t capacity = buffer->capacity == 0U ? 256U : buffer->capacity;
        uint8_t *next;
        while (capacity < buffer->used + size) {
            if (capacity > SIZE_MAX / 2U) { capacity = buffer->used + size; break; }
            capacity *= 2U;
        }
        next = realloc(buffer->data, capacity);
        if (next == NULL) return -1;
        buffer->data = next; buffer->capacity = capacity;
    }
    (void)memcpy(buffer->data + buffer->used, data, size); buffer->used += size;
    return 0;
}

static int append_interface(byte_buffer_t *buffer) {
    struct usb_interface_descriptor descriptor;
    (void)memset(&descriptor, 0, sizeof(descriptor));
    descriptor.bLength = sizeof(descriptor);
    descriptor.bDescriptorType = USB_DT_INTERFACE;
    descriptor.bNumEndpoints = 2;
    descriptor.bInterfaceClass = USB_CLASS_VENDOR_SPEC;
    descriptor.iInterface = 1;
    return append(buffer, &descriptor, sizeof(descriptor));
}

static int append_endpoint(byte_buffer_t *buffer, uint8_t address, uint16_t packet_size) {
    struct usb_endpoint_descriptor_no_audio descriptor;
    (void)memset(&descriptor, 0, sizeof(descriptor));
    descriptor.bLength = sizeof(descriptor);
    descriptor.bDescriptorType = USB_DT_ENDPOINT;
    descriptor.bEndpointAddress = address;
    descriptor.bmAttributes = USB_ENDPOINT_XFER_BULK;
    descriptor.wMaxPacketSize = packet_size;
    return append(buffer, &descriptor, sizeof(descriptor));
}

static int append_compatibility_descriptor(byte_buffer_t *buffer) {
    struct usb_os_desc_header header;
    struct usb_ext_compat_desc descriptor;
    (void)memset(&header, 0, sizeof(header));
    header.interface = 0;
    header.dwLength = sizeof(header) + sizeof(descriptor);
    header.bcdVersion = 1;
    header.wIndex = 4;
    header.bCount = 1;
    (void)memset(&descriptor, 0, sizeof(descriptor));
    descriptor.bFirstInterfaceNumber = 0;
    descriptor.Reserved1 = 1;
    (void)memcpy(descriptor.CompatibleID, "WINUSB", 6U);
    return append(buffer, &header, sizeof(header)) == 0 &&
           append(buffer, &descriptor, sizeof(descriptor)) == 0 ? 0 : -1;
}

static int append_guid_descriptor(byte_buffer_t *buffer) {
    static const char property_name[] = "DeviceInterfaceGUIDs";
    static const char property_value[] = "{77A44C17-5B2B-4AC8-9D02-CC05D28BDF61}";
    struct usb_os_desc_header header;
    struct usb_ext_prop_desc descriptor;
    uint8_t name_utf16[sizeof(property_name) * 2U];
    uint8_t value_utf16[(sizeof(property_value) + 1U) * 2U];
    uint32_t value_size = sizeof(value_utf16);
    uint32_t descriptor_size = sizeof(descriptor) + sizeof(name_utf16) +
                               sizeof(value_size) + sizeof(value_utf16);
    size_t i;
    (void)memset(name_utf16, 0, sizeof(name_utf16));
    (void)memset(value_utf16, 0, sizeof(value_utf16));
    for (i = 0; i < sizeof(property_name); ++i) name_utf16[i * 2U] = (uint8_t)property_name[i];
    for (i = 0; i < sizeof(property_value); ++i) value_utf16[i * 2U] = (uint8_t)property_value[i];
    (void)memset(&header, 0, sizeof(header));
    header.interface = 0;
    header.dwLength = (uint32_t)sizeof(header) + descriptor_size;
    header.bcdVersion = 1;
    header.wIndex = 5;
    header.wCount = 1;
    (void)memset(&descriptor, 0, sizeof(descriptor));
    descriptor.dwSize = descriptor_size;
    descriptor.dwPropertyDataType = 7;
    descriptor.wPropertyNameLength = sizeof(name_utf16);
    if (append(buffer, &header, sizeof(header)) != 0 ||
        append(buffer, &descriptor, sizeof(descriptor)) != 0 ||
        append(buffer, name_utf16, sizeof(name_utf16)) != 0 ||
        append(buffer, &value_size, sizeof(value_size)) != 0 ||
        append(buffer, value_utf16, sizeof(value_utf16)) != 0) return -1;
    return 0;
}

int cra_functionfs_build_descriptors(uint8_t **data, size_t *size) {
    struct usb_functionfs_descs_head_v2 header;
    uint32_t counts[] = {3, 3, 2};
    byte_buffer_t buffer = {0};
    (void)memset(&header, 0, sizeof(header));
    header.magic = FUNCTIONFS_DESCRIPTORS_MAGIC_V2;
    header.flags = FUNCTIONFS_HAS_FS_DESC | FUNCTIONFS_HAS_HS_DESC | FUNCTIONFS_HAS_MS_OS_DESC;
    if (append(&buffer, &header, sizeof(header)) != 0 || append(&buffer, counts, sizeof(counts)) != 0 ||
        append_interface(&buffer) != 0 || append_endpoint(&buffer, USB_DIR_IN | 1U, 64U) != 0 ||
        append_endpoint(&buffer, USB_DIR_OUT | 2U, 64U) != 0 || append_interface(&buffer) != 0 ||
        append_endpoint(&buffer, USB_DIR_IN | 1U, 512U) != 0 ||
        append_endpoint(&buffer, USB_DIR_OUT | 2U, 512U) != 0 ||
        append_compatibility_descriptor(&buffer) != 0 || append_guid_descriptor(&buffer) != 0 ||
        buffer.used > UINT32_MAX) { free(buffer.data); return -1; }
    cra_usb_write_le32(buffer.data + 4U, (uint32_t)buffer.used);
    *data = buffer.data; *size = buffer.used; return 0;
}

int cra_functionfs_build_strings(uint8_t **data, size_t *size) {
    static const char name[] = "CRA Electric Pass";
    struct usb_functionfs_strings_head header;
    uint16_t language = 0x0409;
    byte_buffer_t buffer = {0};
    (void)memset(&header, 0, sizeof(header));
    header.magic = FUNCTIONFS_STRINGS_MAGIC;
    header.str_count = 1;
    header.lang_count = 1;
    if (append(&buffer, &header, sizeof(header)) != 0 ||
        append(&buffer, &language, sizeof(language)) != 0 ||
        append(&buffer, name, sizeof(name)) != 0 || buffer.used > UINT32_MAX) {
        free(buffer.data); return -1;
    }
    cra_usb_write_le32(buffer.data + 4U, (uint32_t)buffer.used);
    *data = buffer.data; *size = buffer.used; return 0;
}

static int write_all(int fd, const uint8_t *data, size_t size) {
    while (size != 0U) {
        size_t chunk = size > IO_CHUNK ? IO_CHUNK : size;
        ssize_t written = write(fd, data, chunk);
        if (written < 0) { if (errno == EINTR) continue; return -1; }
        if (written == 0) { errno = EIO; return -1; }
        data += written; size -= (size_t)written;
    }
    return 0;
}

static int send_response(int endpoint, const cra_usb_response_t *response) {
    cra_usb_frame_t frame = {response->type, 0, response->request_id,
                             response->payload_length, response->payload};
    uint8_t *encoded = NULL;
    size_t size = 0;
    if (cra_usb_encode_frame(&frame, &encoded, &size) != 0) return -1;
    int result = write_all(endpoint, encoded, size);
    if (result == 0 && size != 0U && size % 64U == 0U && write(endpoint, NULL, 0) < 0)
        result = -1;
    free(encoded); return result;
}

static int data_loop(int endpoint_in, int endpoint_out, cra_usb_service_t *service,
                     size_t maximum_payload, bool verbose) {
    byte_buffer_t receive = {0};
    uint8_t chunk[IO_CHUNK];
    for (;;) {
        ssize_t got = read(endpoint_out, chunk, sizeof(chunk));
        if (got < 0) {
            if (errno == EINTR) continue;
            if (errno == ESHUTDOWN || errno == ENODEV || errno == EPIPE) { free(receive.data); return 0; }
            free(receive.data); return -1;
        }
        if (got == 0) continue;
        if (append(&receive, chunk, (size_t)got) != 0) { free(receive.data); return -1; }
        while (receive.used >= CRA_USB_HEADER_SIZE) {
            size_t frame_size = 0;
            int state = cra_usb_peek_frame_size(receive.data, receive.used, maximum_payload, &frame_size);
            if (state > 0) break;
            if (state < 0) { log_message(verbose, "invalid USB frame header"); free(receive.data); errno = EPROTO; return -1; }
            cra_usb_frame_t request;
            cra_usb_response_t response;
            if (cra_usb_decode_frame(receive.data, frame_size, maximum_payload, &request) != 0) {
                log_message(verbose, "invalid USB frame CRC"); free(receive.data); errno = EPROTO; return -1;
            }
            if (cra_usb_service_handle(service, &request, &response) != 0 ||
                send_response(endpoint_in, &response) != 0) {
                cra_usb_response_destroy(&response); free(receive.data); return -1;
            }
            cra_usb_response_destroy(&response);
            receive.used -= frame_size;
            if (receive.used != 0U) (void)memmove(receive.data, receive.data + frame_size, receive.used);
        }
    }
}

static int open_endpoint(const char *mount_path, const char *name, int flags) {
    char path[512];
    if (snprintf(path, sizeof(path), "%s/%s", mount_path, name) >= (int)sizeof(path)) {
        errno = ENAMETOOLONG; return -1;
    }
    return open(path, flags | O_CLOEXEC);
}

int cra_functionfs_run(const char *mount_path, cra_usb_service_t *service,
                       size_t maximum_payload, bool verbose) {
    uint8_t *descriptors = NULL, *strings = NULL;
    size_t descriptors_size = 0, strings_size = 0;
    int control = -1;
    int result = -1;
    if (cra_functionfs_build_descriptors(&descriptors, &descriptors_size) != 0 ||
        cra_functionfs_build_strings(&strings, &strings_size) != 0) goto done;
    control = open_endpoint(mount_path, "ep0", O_RDWR);
    if (control < 0 || write_all(control, descriptors, descriptors_size) != 0 ||
        write_all(control, strings, strings_size) != 0) goto done;
    for (;;) {
        struct usb_functionfs_event events[8];
        ssize_t bytes = read(control, events, sizeof(events));
        size_t count, i;
        if (bytes < 0) { if (errno == EINTR) continue; goto done; }
        if (bytes == 0) { result = 0; goto done; }
        count = (size_t)bytes / sizeof(events[0]);
        for (i = 0; i < count; ++i) {
            log_message(verbose, "FunctionFS event %u", events[i].type);
            if (events[i].type == FUNCTIONFS_ENABLE) {
                int endpoint_in = open_endpoint(mount_path, "ep1", O_WRONLY);
                int endpoint_out = open_endpoint(mount_path, "ep2", O_RDONLY);
                if (endpoint_in < 0 || endpoint_out < 0) {
                    if (endpoint_in >= 0) (void)close(endpoint_in);
                    if (endpoint_out >= 0) (void)close(endpoint_out);
                    goto done;
                }
                result = data_loop(endpoint_in, endpoint_out, service, maximum_payload, verbose);
                (void)close(endpoint_in); (void)close(endpoint_out);
                if (result != 0 && errno != ESHUTDOWN && errno != ENODEV && errno != EPIPE) goto done;
            }
        }
    }
done:
    if (control >= 0) (void)close(control);
    free(descriptors); free(strings); return result;
}
