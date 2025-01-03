/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "sdkconfig.h"
#include "esp_err.h"
#include "stdint.h"

#include "esp_amp_rpmsg.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Definitions for error types. */
#define ESP_AMP_RPC_OK                   0  /* rpc executed successfully */
#define ESP_AMP_RPC_FAIL                -1  /* rpc executed failed other than the following */
#define ESP_AMP_RPC_ERR_INVALID_ARG     -2  /* rpc execute failed due to invalid argument */
#define ESP_AMP_RPC_ERR_INVALID_SIZE    -3  /* rpc execute failed due to invalid size */
#define ESP_AMP_RPC_ERR_NO_MEM          -4  /* rpc execute failed due to no memory */
#define ESP_AMP_RPC_ERR_EXIST           -5  /* rpc execute failed due to object already exists */
#define ESP_AMP_RPC_ERR_NOT_FOUND       -6  /* rpc execute failed due to object not found */
#define ESP_AMP_RPC_ERR_TIMEOUT         -7  /* rpc execute failed due to timeout */
#define ESP_AMP_RPC_ERR_INVALID_STATE   -8  /* rpc execute failed due to invalid state */

/* Definitions for command status */
#define ESP_AMP_RPC_STATUS_OK           0x0000  /* command executed successfully */
#define ESP_AMP_RPC_STATUS_SERVER_BUSY  0xffff  /* server is busy */
#define ESP_AMP_RPC_STATUS_INVALID_CMD  0xfffe  /* invalid cmd id */
#define ESP_AMP_RPC_STATUS_EXEC_FAILED  0xfffd  /* server failed to execute command */
#define ESP_AMP_RPC_STATUS_PENDING      0xfffc  /* command is pending, timeout */

typedef void *esp_amp_rpc_server_t;
typedef void *esp_amp_rpc_client_t;

typedef struct esp_amp_rpc_cmd_t esp_amp_rpc_cmd_t;

/**
 * @brief rpc client callback
 *
 * @param client client handle
 * @param cmd rpc command
 */
typedef void (*esp_amp_rpc_app_cb_t)(esp_amp_rpc_client_t, esp_amp_rpc_cmd_t *, void *);

/**
 * @brief rpc command
 *
 * @note set cb to NULL if you don't care about whether the command is executed successfully
 */
struct esp_amp_rpc_cmd_t {
    uint16_t cmd_id;
    uint16_t status;
    uint16_t req_len;
    uint16_t resp_len;
    uint8_t *req_data; /* request data */
    uint8_t *resp_data; /* response data */
    esp_amp_rpc_app_cb_t cb; /* callback function */
    void *cb_arg; /* callback argument */
};

/**
 * @brief rpc client polling function
 *
 * @note this function will be called by esp_amp_rpc_client_poll()
 */
typedef void (*esp_amp_rpc_app_poll_cb_t)(void *);

/**
 * @brief rpc packet
 */
typedef struct {
    uint16_t msg_id;
    uint16_t cmd_id;
    uint16_t status;
    uint16_t msg_len; /* queue len is uint16_t */
    uint8_t msg_data[0];
} esp_amp_rpc_pkt_t;


/**
 * @brief rpc command handler (server side)
 */
typedef void (*esp_amp_rpc_cmd_handler_t)(esp_amp_rpc_cmd_t *cmd);

/**
 * @brief rpc service (server side)
 *
 * @param cmd_id command id
 * @param handler command handler
 */
typedef struct {
    uint16_t cmd_id;
    esp_amp_rpc_cmd_handler_t handler;
} esp_amp_rpc_service_t;

/**
 * @brief rpc server
 *
 * @note only for internal use
 */
typedef struct {
    uint16_t server_id;
    uint8_t running;
    uint8_t srv_tbl_len;
    uint16_t req_buf_len;
    uint16_t resp_buf_len;
    uint8_t *req_buf;
    uint8_t *resp_buf;
    esp_amp_rpmsg_dev_t *rpmsg_dev;
    esp_amp_rpmsg_ept_t rpmsg_ept;
    void *queue;
    esp_amp_rpc_service_t *srv;
} esp_amp_rpc_server_inst_t;

/**
 * @brief rpc client
 *
 * @note only for internal use
 */
typedef struct {
    uint16_t server_id;
    uint16_t client_id;
    uint16_t pending_id;
    uint8_t running;
    esp_amp_rpmsg_dev_t *rpmsg_dev;
    esp_amp_rpmsg_ept_t rpmsg_ept;
    esp_amp_rpc_cmd_t *pending_cmd;
    esp_amp_rpc_app_poll_cb_t poll_cb;
    void *poll_arg;
} esp_amp_rpc_client_inst_t;

typedef uint8_t esp_amp_rpc_client_stg_t[sizeof(esp_amp_rpc_client_inst_t)];
typedef uint8_t esp_amp_rpc_server_stg_t[sizeof(esp_amp_rpc_server_inst_t)];


/**
 * @brief rpc client config
 */
typedef struct {
    uint16_t client_id;
    uint16_t server_id;
    esp_amp_rpmsg_dev_t *rpmsg_dev;
    esp_amp_rpc_client_stg_t *stg;
    esp_amp_rpc_app_poll_cb_t poll_cb;
    void *poll_arg;
} esp_amp_rpc_client_cfg_t;

/**
 * @brief rpc server config
 */
typedef struct {
    uint16_t server_id;
    uint8_t queue_len;
    uint8_t srv_tbl_len;
    esp_amp_rpmsg_dev_t *rpmsg_dev;
    esp_amp_rpc_server_stg_t *stg;
    uint16_t req_buf_len;
    uint16_t resp_buf_len;
    uint8_t *req_buf;
    uint8_t *resp_buf;
    uint8_t *srv_tbl_stg;
} esp_amp_rpc_server_cfg_t;

/**
 * @brief init rpc client
 *
 * @retval NULL if failed
 * @retval client handle if success
 */
esp_amp_rpc_client_t esp_amp_rpc_client_init(esp_amp_rpc_client_cfg_t *cfg);

/**
 * @brief deinit rpc client
 *
 * @param client client handle
 */
void esp_amp_rpc_client_deinit(esp_amp_rpc_client_t client);

/**
 * @brief execute rpc command
 *
 * @param client client handle
 * @param cmd rpc command
 *
 * @retval ESP_AMP_RPC_OK if success
 * @retval ESP_AMP_RPC_ERR_INVALID_ARG if client or cmd is NULL, or client is deinited
 * @retval ESP_AMP_RPC_ERR_INVALID_SIZE if invalid size
 * @retval ESP_AMP_RPC_ERR_NO_MEM if no memory
 */
int esp_amp_rpc_client_execute_cmd(esp_amp_rpc_client_t client, esp_amp_rpc_cmd_t *cmd);

/**
 * @brief rpc client poll
 *
 * @param client client handle
 */
void esp_amp_rpc_client_poll(esp_amp_rpc_client_t client);

/**
 * @brief rpc server
 *
 * @param server_addr server address
 * @retval NULL if failed
 * @retval server handle if success
 */
esp_amp_rpc_server_t esp_amp_rpc_server_init(esp_amp_rpc_server_cfg_t *cfg);

/**
 * @brief deinit rpc server
 */
void esp_amp_rpc_server_deinit(esp_amp_rpc_server_t server);

/**
 * @brief add an rpc command handler to server
 *
 * @param server server handle
 * @param cmd_id command id
 * @param handler command handler
 * @retval ESP_AMP_RPC_OK if success
 * @retval ESP_AMP_RPC_NO_MEM if service table is full
 * @retval ESP_AMP_RPC_ERR_INVALID_ARG if server or handler is NULL
 * @retval ESP_AMP_RPC_ERR_EXIST if command id already exists
 */
int esp_amp_rpc_server_add_service(esp_amp_rpc_server_t server, uint16_t cmd_id, esp_amp_rpc_cmd_handler_t handler);

/**
 * @brief delete an rpc command handler from server
 *
 * @param server server handle
 * @param cmd_id command id
 * @retval ESP_AMP_RPC_OK if success
 * @retval ESP_AMP_RPC_ERR_INVALID_ARG if server is NULL
 * @retval ESP_AMP_RPC_ERR_NOT_FOUND if not found
 */
int esp_amp_rpc_server_del_service(esp_amp_rpc_server_t server, uint16_t cmd_id);

#if !IS_ENV_BM
/**
 * @brief rpc server run
 *
 * @param server server handle
 * @param timeout_ms timeout in ms. this defines the wait time of polling
 *
 * @retval ESP_AMP_RPC_OK if success
 * @retval ESP_AMP_RPC_ERR_INVALID_ARG if server is NULL
 * @retval ESP_AMP_RPC_ERR_INVALID_STATE if server is not running
 */
int esp_amp_rpc_server_run(esp_amp_rpc_server_t server, uint32_t timeout_ms);
#endif

#ifdef __cplusplus
}
#endif
