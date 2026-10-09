/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#ifndef CRA_EPASS_USB_PROTOCOL_H
#define CRA_EPASS_USB_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define CRA_USB_MAGIC UINT32_C(0x45504153)
#define CRA_USB_VERSION UINT16_C(1)
#define CRA_USB_HEADER_SIZE 24U
#define CRA_USB_MAX_KV_ITEMS 32U

enum cra_usb_message_type {
    CRA_USB_HELLO = 1,
    CRA_USB_STATUS = 2,
    CRA_USB_ERROR = 3,
    CRA_USB_FILE_PUT_BEGIN = 10,
    CRA_USB_FILE_PUT_CHUNK = 11,
    CRA_USB_FILE_PUT_END = 12,
    CRA_USB_FILE_GET = 13,
    CRA_USB_FILE_LIST = 14,
    CRA_USB_FILE_DELETE = 15,
    CRA_USB_FILE_RENAME = 16,
    CRA_USB_FILE_MKDIR = 17,
    CRA_USB_FILE_STAT = 18,
    CRA_USB_COMMAND_EXEC = 20,
    CRA_USB_COMMAND_RESULT = 21,
    CRA_USB_DEVINFO = 30
};

typedef struct {
    uint16_t type;
    uint32_t flags;
    uint32_t request_id;
    uint32_t payload_length;
    const uint8_t *payload;
} cra_usb_frame_t;

typedef struct {
    char *key;
    char *value;
} cra_usb_kv_t;

uint16_t cra_usb_read_le16(const uint8_t *data);
uint32_t cra_usb_read_le32(const uint8_t *data);
void cra_usb_write_le16(uint8_t *data, uint16_t value);
void cra_usb_write_le32(uint8_t *data, uint32_t value);
uint32_t cra_usb_crc32(const uint8_t *data, size_t length);

int cra_usb_peek_frame_size(const uint8_t *data, size_t available,
                            size_t maximum_payload, size_t *frame_size);
int cra_usb_decode_frame(const uint8_t *data, size_t size,
                         size_t maximum_payload, cra_usb_frame_t *frame);
int cra_usb_encode_frame(const cra_usb_frame_t *frame,
                         uint8_t **encoded, size_t *encoded_size);

int cra_usb_decode_kv(const uint8_t *data, size_t size,
                      cra_usb_kv_t *items, size_t capacity, size_t *count);
int cra_usb_encode_kv(const cra_usb_kv_t *items, size_t count,
                      uint8_t **encoded, size_t *encoded_size);
void cra_usb_free_kv(cra_usb_kv_t *items, size_t count);
const char *cra_usb_find_kv(const cra_usb_kv_t *items, size_t count,
                            const char *key);

#endif
