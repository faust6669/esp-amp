/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "sdkconfig.h"
#include "stdio.h"
#include "string.h"
#include "rpc_service.h"
#include "esp_amp_platform.h"

/* actual function definition */
int add(int a, int b)
{
    printf("server: executing add(%d, %d)\r\n", a, b);
    return a + b;
}

void prints(char *buf, int len)
{
    buf[len] = '\0';
    puts(buf);
}

void rpc_cmd_handler_add(esp_amp_rpc_cmd_t *cmd)
{
    int a, b; /* params in */
    int c; /* params out */

    /* get params in */
    add_params_in_t *params_in = (add_params_in_t *)cmd->req_data;
    memcpy(&a, &params_in->a, sizeof(int));
    memcpy(&b, &params_in->b, sizeof(int));

    /* execute the function */
    c = add(params_in->a, params_in->b);

    /* set params out */
    add_params_out_t *add_params_out = (add_params_out_t *)cmd->resp_data;
    memcpy(&add_params_out->ret, &c, sizeof(int));
    cmd->resp_len = sizeof(add_params_out_t);

    /* set status */
    cmd->status = ESP_AMP_RPC_STATUS_OK;
}

void rpc_cmd_handler_printf(esp_amp_rpc_cmd_t *cmd)
{
    int len; /* params in */
    char buf[100]; /* params in */

    /* get params in */
    printf_params_in_t *params_in = (printf_params_in_t *)cmd->req_data;
    memcpy(&len, &params_in->len, sizeof(int));
    if (len > sizeof(buf)) {
        len = sizeof(buf);
    }
    memcpy(&buf, &params_in->buf, len);

    /* execute the function */
    prints(buf, len);

    /* set params out */
    cmd->resp_len = sizeof(printf_params_out_t);

    /* set status */
    cmd->status = ESP_AMP_RPC_STATUS_OK;
}
