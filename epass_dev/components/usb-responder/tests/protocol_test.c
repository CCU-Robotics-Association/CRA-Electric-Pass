/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "cra_epass/usb_protocol.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    static const uint8_t check[] = "123456789";
    cra_usb_kv_t source[] = {{"service", "cra_usb_responder"}, {"version", "1"}};
    cra_usb_kv_t decoded[CRA_USB_MAX_KV_ITEMS];
    cra_usb_frame_t frame = {CRA_USB_STATUS, 0, 42, 0, NULL};
    cra_usb_frame_t parsed;
    uint8_t *kv = NULL;
    uint8_t *wire = NULL;
    size_t kv_size = 0;
    size_t wire_size = 0;
    size_t frame_size = 0;
    size_t decoded_count = 0;

    assert(cra_usb_crc32(check, sizeof(check) - 1U) == UINT32_C(0xcbf43926));
    assert(cra_usb_encode_kv(source, 2, &kv, &kv_size) == 0);
    frame.payload = kv;
    frame.payload_length = (uint32_t)kv_size;
    assert(cra_usb_encode_frame(&frame, &wire, &wire_size) == 0);
    assert(cra_usb_peek_frame_size(wire, CRA_USB_HEADER_SIZE - 1U, 4096, &frame_size) == 1);
    assert(cra_usb_peek_frame_size(wire, wire_size, 4096, &frame_size) == 0);
    assert(frame_size == wire_size);
    assert(cra_usb_decode_frame(wire, wire_size, 4096, &parsed) == 0);
    assert(parsed.type == CRA_USB_STATUS && parsed.request_id == 42);
    assert(cra_usb_decode_kv(parsed.payload, parsed.payload_length, decoded,
                             CRA_USB_MAX_KV_ITEMS, &decoded_count) == 0);
    assert(decoded_count == 2);
    assert(strcmp(cra_usb_find_kv(decoded, decoded_count, "service"),
                  "cra_usb_responder") == 0);
    cra_usb_free_kv(decoded, decoded_count);

    wire[20] ^= 1U;
    assert(cra_usb_decode_frame(wire, wire_size, 4096, &parsed) != 0);
    free(wire);
    free(kv);
    return 0;
}
