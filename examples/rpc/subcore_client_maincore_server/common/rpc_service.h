/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#pragma once

#include "esp_amp.h"

#define RPC_DEMO_CLIENT 0x0001
#define RPC_DEMO_SERVER 0x1001
#define RPC_SRV_NUM 2

/* params of rpc service: add */
typedef struct {
    int a;
    int b;
} add_params_in_t;

typedef struct {
    int ret;
} add_params_out_t;

typedef struct {
    int len;
    char buf[100];
} printf_params_in_t;

typedef struct {

} printf_params_out_t;

typedef enum {
    RPC_CMD_ID_ADD,
    RPC_CMD_ID_PRINTF,
} rpc_cmd_id_t;

/* service handlers */
void rpc_cmd_handler_add(esp_amp_rpc_cmd_t *cmd);
void rpc_cmd_handler_printf(esp_amp_rpc_cmd_t *cmd);
