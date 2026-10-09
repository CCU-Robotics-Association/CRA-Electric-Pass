/* SPDX-License-Identifier: GPL-3.0-or-later */

#include "cra_epass/ipc_v1.h"

#include <assert.h>

int main(void) {
    assert(cra_ipc_request_size(CRA_IPC_REQ_UI_WARNING) == 216U);
    assert(cra_ipc_request_size(CRA_IPC_REQ_THEME_SET_BLOCKED_AUTO_SWITCH) == 5U);
    assert(cra_ipc_request_size(CRA_IPC_REQ_OVERLAY_SCHEDULE_TRANSITION_VIDEO) == 272U);
    assert(cra_ipc_response_size(CRA_IPC_REQ_THEME_GET_INFO) == 452U);
    assert(cra_ipc_request_size(999) == 0U);
    assert(cra_ipc_response_size(999) == 0U);
    return 0;
}
