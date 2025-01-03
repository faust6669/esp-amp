/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_err.h"
#include "esp_amp.h"

#include "event.h"
#include "rpc_service.h"

static esp_amp_rpmsg_dev_t rpmsg_dev;
static esp_amp_rpc_server_stg_t rpc_server_stg;
static uint8_t req_buf[128];
static uint8_t resp_buf[128];
static uint8_t srv_tbl_stg[sizeof(esp_amp_rpc_service_t) * RPC_SRV_NUM];

void app_main(void)
{
    /* init esp amp */
    assert(esp_amp_init() == 0);
    assert(esp_amp_rpmsg_main_init(&rpmsg_dev, 32, 128, false, false) == 0);
    esp_amp_rpmsg_intr_enable(&rpmsg_dev);

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
    printf("rpc server init successfully\n");

    /* add services */
    assert(esp_amp_rpc_server_add_service(server, RPC_CMD_ID_ADD, rpc_cmd_handler_add) == 0);
    assert(esp_amp_rpc_server_add_service(server, RPC_CMD_ID_PRINTF, rpc_cmd_handler_printf) == 0);

    /* Load firmware & start subcore */
    const esp_partition_t *sub_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, NULL);
    ESP_ERROR_CHECK(esp_amp_load_sub_from_partition(sub_partition));
    ESP_ERROR_CHECK(esp_amp_start_subcore());

    /* wait for link up */
    assert((esp_amp_event_wait(EVENT_SUBCORE_READY, true, true, 10000) & EVENT_SUBCORE_READY) == EVENT_SUBCORE_READY);

    while (1) {
        esp_amp_rpc_server_run(server, portMAX_DELAY);
    }
}
