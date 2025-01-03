/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdint.h>
#include <stdio.h>
#include "esp_amp.h"
#include "esp_amp_platform.h"

#include "event.h"
#include "rpc_service.h"

static esp_amp_rpmsg_dev_t rpmsg_dev;
static esp_amp_rpc_server_stg_t rpc_server_stg;

static uint8_t req_buf[128];
static uint8_t resp_buf[128];
static uint8_t srv_tbl_stg[sizeof(esp_amp_rpc_service_t) * RPC_SRV_NUM];

int main(void)
{
    printf("SUB: Hello!!\r\n");

    assert(esp_amp_init() == 0);
    assert(esp_amp_rpmsg_sub_init(&rpmsg_dev, true, true) == 0);

    /* notify link up with main core */
    esp_amp_event_notify(EVENT_SUBCORE_READY);

    esp_amp_rpc_server_cfg_t cfg = {
        .rpmsg_dev = &rpmsg_dev,
        .server_id = RPC_DEMO_SERVER,
        .stg = &rpc_server_stg,
        .req_buf_len = sizeof(req_buf),
        .resp_buf_len = sizeof(resp_buf),
        .req_buf = req_buf,
        .resp_buf = resp_buf,
        .srv_tbl_len = 2,
        .srv_tbl_stg = srv_tbl_stg,
    };
    esp_amp_rpc_server_t server = esp_amp_rpc_server_init(&cfg);
    assert(server != NULL);
    printf("SUB: rpc server init successfully\r\n");

    /* add services */
    assert(esp_amp_rpc_server_add_service(server, RPC_CMD_ID_ADD, rpc_cmd_handler_add) == 0);
    assert(esp_amp_rpc_server_add_service(server, RPC_CMD_ID_PRINTF, rpc_cmd_handler_printf) == 0);

    int i = 0;
    while (true) {
        while (esp_amp_rpmsg_poll(&rpmsg_dev) == 0);

        if (++i % 1000 == 0) {
            printf("SUB: running...\r\n");
        }
        esp_amp_platform_delay_us(1000);
    }

    printf("SUB: Bye!!\r\n");
    abort();
}
