/* SPDX-License-Identifier: GPL-3.0-or-later */

#define _XOPEN_SOURCE 700
#include "cra_epass/usb_service.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static cra_usb_response_t request_kv(cra_usb_service_t *service, uint16_t type,
                                     uint32_t id, cra_usb_kv_t *items, size_t count) {
    cra_usb_frame_t request = {0};
    cra_usb_response_t response;
    uint8_t *payload = NULL;
    size_t payload_size = 0;
    assert(cra_usb_encode_kv(items, count, &payload, &payload_size) == 0);
    request.type = type;
    request.request_id = id;
    request.payload = payload;
    request.payload_length = (uint32_t)payload_size;
    assert(cra_usb_service_handle(service, &request, &response) == 0);
    free(payload);
    return response;
}

static void assert_status(cra_usb_response_t *response) {
    cra_usb_kv_t values[CRA_USB_MAX_KV_ITEMS];
    size_t count = 0;
    assert(response->type == CRA_USB_STATUS);
    assert(cra_usb_decode_kv(response->payload, response->payload_length, values,
                             CRA_USB_MAX_KV_ITEMS, &count) == 0);
    assert(strcmp(cra_usb_find_kv(values, count, "status"), "ok") == 0);
    cra_usb_free_kv(values, count);
    cra_usb_response_destroy(response);
}

int main(void) {
    char root[] = "/tmp/cra-usb-service-XXXXXX";
    cra_usb_service_t service;
    cra_usb_response_t response;
    cra_usb_frame_t chunk = {0};
    static const uint8_t contents[] = "CRA Electric Pass\n";
    static const char command[] = "printf out; printf err >&2";

    assert(mkdtemp(root) != NULL);
    assert(cra_usb_service_init(&service, root) == 0);

    cra_usb_kv_t mkdir_items[] = {{"path", "assets"}, {"parents", "1"}};
    response = request_kv(&service, CRA_USB_FILE_MKDIR, 1, mkdir_items, 2);
    assert_status(&response);

    cra_usb_kv_t begin_items[] = {{"path", "assets/test.txt"}, {"perm", "0640"}};
    response = request_kv(&service, CRA_USB_FILE_PUT_BEGIN, 17, begin_items, 2);
    assert_status(&response);

    chunk.type = CRA_USB_FILE_PUT_CHUNK;
    chunk.request_id = 17;
    chunk.payload = contents;
    chunk.payload_length = (uint32_t)(sizeof(contents) - 1U);
    assert(cra_usb_service_handle(&service, &chunk, &response) == 0);
    assert_status(&response);

    chunk.type = CRA_USB_FILE_PUT_END;
    chunk.payload = NULL;
    chunk.payload_length = 0;
    assert(cra_usb_service_handle(&service, &chunk, &response) == 0);
    assert_status(&response);

    cra_usb_kv_t get_items[] = {{"path", "assets/test.txt"}};
    response = request_kv(&service, CRA_USB_FILE_GET, 18, get_items, 1);
    assert(response.type == CRA_USB_FILE_GET);
    assert(response.payload_length == sizeof(contents) - 1U);
    assert(memcmp(response.payload, contents, sizeof(contents) - 1U) == 0);
    cra_usb_response_destroy(&response);

    cra_usb_kv_t bad_items[] = {{"path", "../outside"}};
    response = request_kv(&service, CRA_USB_FILE_GET, 19, bad_items, 1);
    assert(response.type == CRA_USB_ERROR);
    cra_usb_response_destroy(&response);

    cra_usb_kv_t rename_items[] = {{"from", "assets/test.txt"}, {"to", "assets/moved.txt"}};
    response = request_kv(&service, CRA_USB_FILE_RENAME, 20, rename_items, 2);
    assert_status(&response);

    cra_usb_kv_t delete_items[] = {{"path", "assets"}};
    response = request_kv(&service, CRA_USB_FILE_DELETE, 21, delete_items, 1);
    assert_status(&response);

    {
        uint8_t command_payload[16U + sizeof(command) - 1U];
        cra_usb_write_le32(command_payload, 1000U);
        cra_usb_write_le32(command_payload + 4U, 1024U);
        cra_usb_write_le32(command_payload + 8U, 1024U);
        cra_usb_write_le32(command_payload + 12U, sizeof(command) - 1U);
        memcpy(command_payload + 16U, command, sizeof(command) - 1U);
        chunk.type = CRA_USB_COMMAND_EXEC;
        chunk.request_id = 22;
        chunk.payload = command_payload;
        chunk.payload_length = sizeof(command_payload);
        assert(cra_usb_service_handle(&service, &chunk, &response) == 0);
        assert(response.type == CRA_USB_COMMAND_RESULT);
        assert(cra_usb_read_le32(response.payload) == 0U);
        assert(response.payload[4] == 0U);
        assert(cra_usb_read_le32(response.payload + 12U) == 3U);
        assert(cra_usb_read_le32(response.payload + 16U) == 3U);
        assert(memcmp(response.payload + 20U, "out", 3U) == 0);
        assert(memcmp(response.payload + 23U, "err", 3U) == 0);
        cra_usb_response_destroy(&response);
    }

    assert(rmdir(root) == 0);
    cra_usb_service_destroy(&service);
    return 0;
}
