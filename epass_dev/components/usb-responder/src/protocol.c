/* SPDX-License-Identifier: GPL-3.0-or-later */
/* Copyright (C) 2026 CCU Robotics Association */

#include "cra_epass/usb_protocol.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

uint16_t cra_usb_read_le16(const uint8_t *data) {
    return (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8U));
}

uint32_t cra_usb_read_le32(const uint8_t *data) {
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8U) |
           ((uint32_t)data[2] << 16U) | ((uint32_t)data[3] << 24U);
}

void cra_usb_write_le16(uint8_t *data, uint16_t value) {
    data[0] = (uint8_t)(value & UINT16_C(0xff));
    data[1] = (uint8_t)(value >> 8U);
}

void cra_usb_write_le32(uint8_t *data, uint32_t value) {
    data[0] = (uint8_t)(value & UINT32_C(0xff));
    data[1] = (uint8_t)((value >> 8U) & UINT32_C(0xff));
    data[2] = (uint8_t)((value >> 16U) & UINT32_C(0xff));
    data[3] = (uint8_t)(value >> 24U);
}

uint32_t cra_usb_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = UINT32_MAX;
    size_t i;
    for (i = 0; i < length; ++i) {
        unsigned int bit;
        crc ^= data[i];
        for (bit = 0; bit < 8U; ++bit) {
            uint32_t mask = (uint32_t)-(int32_t)(crc & 1U);
            crc = (crc >> 1U) ^ (UINT32_C(0xedb88320) & mask);
        }
    }
    return ~crc;
}

int cra_usb_peek_frame_size(const uint8_t *data, size_t available,
                            size_t maximum_payload, size_t *frame_size) {
    uint32_t payload_length;
    if (available < CRA_USB_HEADER_SIZE) return 1;
    if (cra_usb_read_le32(data) != CRA_USB_MAGIC ||
        cra_usb_read_le16(data + 4U) != CRA_USB_VERSION) return -1;
    payload_length = cra_usb_read_le32(data + 16U);
    if ((size_t)payload_length > maximum_payload) return -1;
#if SIZE_MAX == UINT32_MAX
    if (payload_length > (uint32_t)(SIZE_MAX - CRA_USB_HEADER_SIZE)) return -1;
#endif
    *frame_size = CRA_USB_HEADER_SIZE + (size_t)payload_length;
    return available < *frame_size ? 1 : 0;
}

int cra_usb_decode_frame(const uint8_t *data, size_t size,
                         size_t maximum_payload, cra_usb_frame_t *frame) {
    size_t frame_size = 0;
    int result = cra_usb_peek_frame_size(data, size, maximum_payload, &frame_size);
    if (result != 0 || frame_size != size) return -1;
    frame->type = cra_usb_read_le16(data + 6U);
    frame->flags = cra_usb_read_le32(data + 8U);
    frame->request_id = cra_usb_read_le32(data + 12U);
    frame->payload_length = cra_usb_read_le32(data + 16U);
    frame->payload = data + CRA_USB_HEADER_SIZE;
    if (cra_usb_crc32(frame->payload, frame->payload_length) !=
        cra_usb_read_le32(data + 20U)) return -1;
    return 0;
}

int cra_usb_encode_frame(const cra_usb_frame_t *frame,
                         uint8_t **encoded, size_t *encoded_size) {
    size_t total;
    uint8_t *data;
    if (frame->payload_length != 0U && frame->payload == NULL) return -1;
#if SIZE_MAX == UINT32_MAX
    if (frame->payload_length > (uint32_t)(SIZE_MAX - CRA_USB_HEADER_SIZE)) return -1;
#endif
    total = CRA_USB_HEADER_SIZE + (size_t)frame->payload_length;
    data = malloc(total);
    if (data == NULL) return -1;
    cra_usb_write_le32(data, CRA_USB_MAGIC);
    cra_usb_write_le16(data + 4U, CRA_USB_VERSION);
    cra_usb_write_le16(data + 6U, frame->type);
    cra_usb_write_le32(data + 8U, frame->flags);
    cra_usb_write_le32(data + 12U, frame->request_id);
    cra_usb_write_le32(data + 16U, frame->payload_length);
    cra_usb_write_le32(data + 20U, cra_usb_crc32(frame->payload, frame->payload_length));
    if (frame->payload_length != 0U)
        (void)memcpy(data + CRA_USB_HEADER_SIZE, frame->payload, frame->payload_length);
    *encoded = data;
    *encoded_size = total;
    return 0;
}

void cra_usb_free_kv(cra_usb_kv_t *items, size_t count) {
    size_t i;
    for (i = 0; i < count; ++i) {
        free(items[i].key);
        free(items[i].value);
        items[i].key = NULL;
        items[i].value = NULL;
    }
}

int cra_usb_decode_kv(const uint8_t *data, size_t size,
                      cra_usb_kv_t *items, size_t capacity, size_t *count) {
    size_t cursor = 0;
    size_t decoded = 0;
    uint16_t item_count;
    if (size < 2U) return -1;
    item_count = cra_usb_read_le16(data);
    cursor = 2U;
    if ((size_t)item_count > capacity || item_count > CRA_USB_MAX_KV_ITEMS) return -1;
    (void)memset(items, 0, capacity * sizeof(*items));
    while (decoded < item_count) {
        uint16_t key_length;
        uint16_t value_length;
        if (size - cursor < 4U) goto fail;
        key_length = cra_usb_read_le16(data + cursor);
        value_length = cra_usb_read_le16(data + cursor + 2U);
        cursor += 4U;
        if (key_length == 0U || (size_t)key_length + (size_t)value_length > size - cursor)
            goto fail;
        if (memchr(data + cursor, '\0', key_length) != NULL ||
            memchr(data + cursor + key_length, '\0', value_length) != NULL) goto fail;
        items[decoded].key = malloc((size_t)key_length + 1U);
        items[decoded].value = malloc((size_t)value_length + 1U);
        if (items[decoded].key == NULL || items[decoded].value == NULL) goto fail;
        (void)memcpy(items[decoded].key, data + cursor, key_length);
        items[decoded].key[key_length] = '\0';
        cursor += key_length;
        (void)memcpy(items[decoded].value, data + cursor, value_length);
        items[decoded].value[value_length] = '\0';
        cursor += value_length;
        ++decoded;
    }
    if (cursor != size) goto fail;
    *count = decoded;
    return 0;
fail:
    cra_usb_free_kv(items, decoded + (decoded < capacity ? 1U : 0U));
    *count = 0;
    return -1;
}

int cra_usb_encode_kv(const cra_usb_kv_t *items, size_t count,
                      uint8_t **encoded, size_t *encoded_size) {
    size_t total = 2U;
    size_t cursor = 2U;
    size_t i;
    uint8_t *data;
    if (count > CRA_USB_MAX_KV_ITEMS || count > UINT16_MAX) return -1;
    for (i = 0; i < count; ++i) {
        size_t key_length;
        size_t value_length;
        if (items[i].key == NULL || items[i].value == NULL) return -1;
        key_length = strlen(items[i].key);
        value_length = strlen(items[i].value);
        if (key_length == 0U || key_length > UINT16_MAX || value_length > UINT16_MAX ||
            key_length + value_length > SIZE_MAX - total - 4U) return -1;
        total += 4U + key_length + value_length;
    }
    data = malloc(total);
    if (data == NULL) return -1;
    cra_usb_write_le16(data, (uint16_t)count);
    for (i = 0; i < count; ++i) {
        uint16_t key_length = (uint16_t)strlen(items[i].key);
        uint16_t value_length = (uint16_t)strlen(items[i].value);
        cra_usb_write_le16(data + cursor, key_length);
        cra_usb_write_le16(data + cursor + 2U, value_length);
        cursor += 4U;
        (void)memcpy(data + cursor, items[i].key, key_length);
        cursor += key_length;
        (void)memcpy(data + cursor, items[i].value, value_length);
        cursor += value_length;
    }
    *encoded = data;
    *encoded_size = total;
    return 0;
}

const char *cra_usb_find_kv(const cra_usb_kv_t *items, size_t count,
                            const char *key) {
    size_t i;
    for (i = 0; i < count; ++i) if (strcmp(items[i].key, key) == 0) return items[i].value;
    return NULL;
}
