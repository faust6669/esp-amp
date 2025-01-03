/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_err.h"
#include "esp_amp.h"

#include "event.h"
#include "rpc_service.h"

static esp_amp_rpmsg_dev_t rpmsg_dev;
static esp_amp_rpc_client_stg_t rpc_client_stg[3];

static void cmd_add_cb(esp_amp_rpc_client_t client, esp_amp_rpc_cmd_t *cmd, void *arg)
{
    TaskHandle_t task = (TaskHandle_t)arg;
    BaseType_t need_yield = false;

    vTaskNotifyGiveFromISR(task, &need_yield);
    portYIELD_FROM_ISR(need_yield);
}

/* blocking demo with response */
static int rpc_cmd_add(esp_amp_rpc_client_t client, int a, int b, int *ret)
{
    /* construct request data */
    add_params_in_t in_params = {
        .a = a,
        .b = b,
    };

    add_params_out_t out_params;

    esp_amp_rpc_cmd_t cmd = {
        .cmd_id = RPC_CMD_ID_ADD,
        .req_len = sizeof(in_params),
        .resp_len = sizeof(out_params),
        .req_data = (uint8_t *) &in_params,
        .resp_data = (uint8_t *) &out_params,
        .cb = cmd_add_cb,
        .cb_arg = xTaskGetCurrentTaskHandle(),
    };

    int err = esp_amp_rpc_client_execute_cmd(client, &cmd);
    if (err == ESP_AMP_RPC_OK) {
        ulTaskNotifyTake(true, pdMS_TO_TICKS(1000)); /* wait up to 1000ms */
        if (cmd.status == ESP_AMP_RPC_STATUS_OK) {
            *ret = out_params.ret;
        } else {
            printf("client: rpc cmd failed, status: %x\n", cmd.status);
            err = ESP_AMP_RPC_FAIL;
        }
    }
    return err;
}

/* non-blocking demo without response */
static void rpc_cmd_printf(esp_amp_rpc_client_t client, const char *fmt, ...)
{
    uint8_t buf[100] = {0};

    va_list ap;
    va_start(ap, fmt);
    int len = vsnprintf((char *)buf, sizeof(buf), fmt, ap);
    va_end(ap);

    printf_params_in_t in_params;
    memcpy(&in_params.len, &len, sizeof(int));
    memcpy(&in_params.buf, buf, len);

    esp_amp_rpc_cmd_t cmd = {
        .cmd_id = RPC_CMD_ID_PRINTF,
        .req_len = sizeof(in_params),
        .resp_len = 0,
        .req_data = (uint8_t *) &in_params,
        .resp_data = NULL,
        .cb = NULL, /* non-blocking */
    };

    /* non-blocking, return immediately */
    esp_amp_rpc_client_execute_cmd(client, &cmd);
}

static void client(void *args)
{
    int client_id = (int)args;

    /* init rpc client */
    esp_amp_rpc_client_cfg_t cfg = {
        .client_id = client_id,
        .server_id = RPC_DEMO_SERVER,
        .rpmsg_dev = &rpmsg_dev,
        .stg = &rpc_client_stg[client_id - 1],
    };
    esp_amp_rpc_client_t client = esp_amp_rpc_client_init(&cfg);
    if (client == NULL) {
        printf("init rpc client failed\n");
        return;
    }

    for (int i = 0; i < 10; i++) {
        int a = i + client_id * 10000;
        int b = client_id * 10000 + i + 1;
        int ret = 0;
        int err = rpc_cmd_add(client, a, b, &ret);
        if (err != ESP_AMP_RPC_OK) {
            printf("client(%d): rpc cmd add exec failed\n", client_id);
        } else {
            printf("client(%d): rpc cmd add(%d, %d): expected=%d, actual=%d, %s\n", client_id, a, b, a + b, ret, (a + b == ret) ? "PASS" : "FAIL");
        }
        rpc_cmd_printf(client, "client(%d) print on server side %d times\n", client_id, (i + 1));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    vTaskDelete(NULL);
}

void app_main(void)
{
    /* init esp amp */
    assert(esp_amp_init() == 0);
    assert(esp_amp_rpmsg_main_init(&rpmsg_dev, 8, 128, false, false) == 0);

    esp_amp_rpmsg_intr_enable(&rpmsg_dev);

    /* Load firmware & start subcore */
    const esp_partition_t *sub_partition = esp_partition_find_first(ESP_PARTITION_TYPE_DATA, 0x40, NULL);
    ESP_ERROR_CHECK(esp_amp_load_sub_from_partition(sub_partition));
    ESP_ERROR_CHECK(esp_amp_start_subcore());

    /* wait for link up */
    assert((esp_amp_event_wait(EVENT_SUBCORE_READY, true, true, 10000) & EVENT_SUBCORE_READY) == EVENT_SUBCORE_READY);

    if (xTaskCreate(client, "c1", 2048, (void *)1, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        printf("Failed to create task c1\n");
    }

    if (xTaskCreate(client, "c2", 2048, (void *)2, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        printf("Failed to create task c2");
    }

    if (xTaskCreate(client, "c3", 2048, (void *)3, tskIDLE_PRIORITY + 1, NULL) != pdPASS) {
        printf("Failed to create task c3");
    }
}
