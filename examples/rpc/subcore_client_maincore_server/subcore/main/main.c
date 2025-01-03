/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdatomic.h>
#include <string.h>
#include "esp_amp.h"
#include "esp_amp_platform.h"

#include "event.h"
#include "rpc_service.h"

#define TAG "rpc_client"

static esp_amp_rpmsg_dev_t rpmsg_dev;
static esp_amp_rpc_client_stg_t rpc_client_stg;

static void cmd_add_cb(esp_amp_rpc_client_t client, esp_amp_rpc_cmd_t *cmd, void *arg)
{
    atomic_flag *nack = (atomic_flag *)arg;
    atomic_flag_clear(nack);
}

/* blocking demo with response */
static int rpc_cmd_add(esp_amp_rpc_client_t client, int a, int b, int *ret)
{
    /* construct request & response data */
    add_params_in_t in_params = {
        .a = a,
        .b = b,
    };

    add_params_out_t out_params;

    /* blocking call*/
    atomic_flag nack = ATOMIC_FLAG_INIT;
    atomic_flag_test_and_set(&nack); /* set nack flag */

    esp_amp_rpc_cmd_t cmd = {
        .cmd_id = RPC_CMD_ID_ADD,
        .req_len = sizeof(in_params),
        .resp_len = sizeof(out_params),
        .req_data = (uint8_t *) &in_params,
        .resp_data = (uint8_t *) &out_params,
        .cb = cmd_add_cb,
        .cb_arg = &nack,
    };

    int err = esp_amp_rpc_client_execute_cmd(client, &cmd);
    if (err == ESP_AMP_RPC_OK) {
        uint32_t tic = esp_amp_platform_get_time_ms();
        /* poll response */
        while (atomic_flag_test_and_set(&nack)) {
            esp_amp_rpc_client_poll(client);
            if (esp_amp_platform_get_time_ms() - tic > 10) { /* wait up to 10ms */
                break;
            }
        }

        if (cmd.status == ESP_AMP_RPC_STATUS_OK) {
            *ret = out_params.ret;
        } else {
            printf("SUB: client: rpc cmd add failed, status: %x\n", cmd.status);
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

/* rpc client user-defined poll function */
static void rpc_client_poll(void *args)
{
    esp_amp_rpmsg_dev_t *rpmsg_dev = (esp_amp_rpmsg_dev_t *)args;
    static uint32_t cnt = 0;

    if (cnt++ % 1000 == 0) {
        printf("SUB: client: polling\r\n");
    }

    esp_amp_rpmsg_poll(rpmsg_dev);
    esp_amp_platform_delay_ms(1);
}

int main(void)
{
    printf("SUB: Hello!!\r\n");

    assert(esp_amp_init() == 0);
    assert(esp_amp_rpmsg_sub_init(&rpmsg_dev, true, true) == 0);

    esp_amp_rpc_client_cfg_t cfg = {
        .client_id = RPC_DEMO_CLIENT,
        .server_id = RPC_DEMO_SERVER,
        .rpmsg_dev = &rpmsg_dev,
        .stg = &rpc_client_stg,
        .poll_cb = rpc_client_poll, /* user-defined poll function */
        .poll_arg = &rpmsg_dev,
    };

    esp_amp_rpc_client_t client = esp_amp_rpc_client_init(&cfg);
    assert(client != NULL);

    /* notify link up with main core */
    esp_amp_event_notify(EVENT_SUBCORE_READY);

    for (int i = 0; i < 10; i++) {
        int a = i;
        int b = i + 1;
        int ret = 0;
        int err = rpc_cmd_add(client, a, b, &ret);
        if (err == ESP_AMP_RPC_OK) {
            printf("SUB: client: rpc cmd add(%d, %d): expected=%d, actual=%d, %s\r\n", a, b, a + b, ret, (a + b == ret) ? "PASS" : "FAIL");
        } else {
            printf("SUB: client: rpc cmd add exec failed, err=%d\r\n", err);
        }
        rpc_cmd_printf(client, "client print on server %d times\n", i + 1);
        while (esp_amp_rpmsg_poll(&rpmsg_dev) == 0);
        esp_amp_platform_delay_us(100000);
    }

    /* wait for more response */
    while (true) {
        while (esp_amp_rpmsg_poll(&rpmsg_dev) == 0);
        esp_amp_platform_delay_us(100000);
    }

    printf("SUB: Bye!!\r\n");
    abort();
}
