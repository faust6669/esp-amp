/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#include "esp_log.h"
#include "esp_amp.h"
#include "esp_err.h"

#include "unity.h"
#include "unity_test_runner.h"

#define RPC_MAIN_CORE_CLIENT 0x0000
#define RPC_MAIN_CORE_SERVER 0x0001
#define RPC_CMD_ID_DEMO_1    0x0000

TEST_CASE("RPC client init/deinit", "[esp_amp]")
{
    esp_amp_rpmsg_dev_t* rpmsg_dev = (esp_amp_rpmsg_dev_t*)(malloc(sizeof(esp_amp_rpmsg_dev_t)));
    TEST_ASSERT_NOT_NULL(rpmsg_dev);

    TEST_ASSERT(esp_amp_init() == 0);
    TEST_ASSERT(esp_amp_rpmsg_main_init(rpmsg_dev, 32, 64, false, false) == 0);

    static esp_amp_rpc_client_stg_t rpc_client_stg;
    esp_amp_rpc_client_cfg_t cfg = {
        .client_id = RPC_MAIN_CORE_CLIENT,
        .server_id = RPC_MAIN_CORE_SERVER,
        .rpmsg_dev = rpmsg_dev,
        .stg = NULL,
        .poll_cb = NULL,
        .poll_arg = NULL,
    };

    /* fail if stg is NULL */
    TEST_ASSERT_EQUAL(NULL, esp_amp_rpc_client_init(&cfg));

    /* success if params are valid */
    cfg.stg = &rpc_client_stg;
    esp_amp_rpc_client_t client = esp_amp_rpc_client_init(&cfg);
    TEST_ASSERT_NOT_EQUAL(NULL, client);
    esp_amp_rpc_client_deinit(client);

    /* fail if rpmsg_dev is NULL */
    cfg.rpmsg_dev = NULL;
    TEST_ASSERT_EQUAL(NULL, esp_amp_rpc_client_init(&cfg));

    free(rpmsg_dev);
    vTaskDelay(pdMS_TO_TICKS(500));
}

TEST_CASE("RPC server init/deinit", "[esp_amp]")
{
    esp_amp_rpmsg_dev_t* rpmsg_dev = (esp_amp_rpmsg_dev_t*)(malloc(sizeof(esp_amp_rpmsg_dev_t)));
    TEST_ASSERT_NOT_NULL(rpmsg_dev);

    TEST_ASSERT(esp_amp_init() == 0);
    TEST_ASSERT(esp_amp_rpmsg_main_init(rpmsg_dev, 32, 64, false, false) == 0);

    static esp_amp_rpc_server_stg_t rpc_server_stg;
    static uint8_t req_buf[64];
    static uint8_t resp_buf[64];
    static uint8_t srv_tbl_stg[sizeof(esp_amp_rpc_service_t) * 2];
    esp_amp_rpc_server_cfg_t cfg = {
        .rpmsg_dev = rpmsg_dev,
        .server_id = RPC_MAIN_CORE_SERVER,
        .stg = NULL,
        .req_buf_len = sizeof(req_buf),
        .resp_buf_len = sizeof(resp_buf),
        .req_buf = req_buf,
        .resp_buf = resp_buf,
        .srv_tbl_len = 2,
        .srv_tbl_stg = srv_tbl_stg,
    };

    /* fail if stg is NULL */
    TEST_ASSERT_EQUAL(NULL, esp_amp_rpc_server_init(&cfg));

    /* success if params are valid */
    cfg.stg = &rpc_server_stg;
    esp_amp_rpc_server_t server = esp_amp_rpc_server_init(&cfg);
    TEST_ASSERT_NOT_EQUAL(NULL, server);
    esp_amp_rpc_server_deinit(server);

    /* fail if rpmsg_dev is NULL */
    cfg.rpmsg_dev = NULL;
    TEST_ASSERT_EQUAL(NULL, esp_amp_rpc_server_init(&cfg));

    /* fail if req_buf is NULL */
    cfg.rpmsg_dev = rpmsg_dev;
    cfg.req_buf = NULL;
    TEST_ASSERT_EQUAL(NULL, esp_amp_rpc_server_init(&cfg));

    /* fail if resp_buf is NULL */
    cfg.req_buf = req_buf;
    cfg.resp_buf = NULL;
    TEST_ASSERT_EQUAL(NULL, esp_amp_rpc_server_init(&cfg));

    /* fail if srv_tbl_stg is NULL */
    cfg.resp_buf = resp_buf;
    cfg.srv_tbl_stg = NULL;
    TEST_ASSERT_EQUAL(NULL, esp_amp_rpc_server_init(&cfg));

    /* fail if srv_tbl_len is 0 */
    cfg.srv_tbl_stg = srv_tbl_stg;
    cfg.srv_tbl_len = 0;
    TEST_ASSERT_EQUAL(NULL, esp_amp_rpc_server_init(&cfg));

    free(rpmsg_dev);
    vTaskDelay(pdMS_TO_TICKS(500));
}

static void rpc_cmd_handler_test(esp_amp_rpc_cmd_t *cmd)
{
    return;
}

TEST_CASE("RPC server add/del service", "[esp_amp]")
{
    esp_amp_rpmsg_dev_t* rpmsg_dev = (esp_amp_rpmsg_dev_t*)(malloc(sizeof(esp_amp_rpmsg_dev_t)));
    TEST_ASSERT_NOT_NULL(rpmsg_dev);

    TEST_ASSERT(esp_amp_init() == 0);
    TEST_ASSERT(esp_amp_rpmsg_main_init(rpmsg_dev, 32, 64, false, false) == 0);

    /* init server */
    static esp_amp_rpc_server_stg_t rpc_server_stg;
    static uint8_t req_buf[64];
    static uint8_t resp_buf[64];
    static uint8_t srv_tbl_stg[sizeof(esp_amp_rpc_service_t) * 2];
    esp_amp_rpc_server_cfg_t cfg = {
        .rpmsg_dev = rpmsg_dev,
        .server_id = RPC_MAIN_CORE_SERVER,
        .stg = &rpc_server_stg,
        .req_buf_len = sizeof(req_buf),
        .resp_buf_len = sizeof(resp_buf),
        .req_buf = req_buf,
        .resp_buf = resp_buf,
        .srv_tbl_len = 2,
        .srv_tbl_stg = srv_tbl_stg,
    };

    esp_amp_rpc_server_t server = esp_amp_rpc_server_init(&cfg);
    TEST_ASSERT_NOT_EQUAL(NULL, server);

    /* handler added to same cmd id will report error */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_OK, esp_amp_rpc_server_add_service(server, RPC_CMD_ID_DEMO_1, rpc_cmd_handler_test));
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_ERR_EXIST, esp_amp_rpc_server_add_service(server, RPC_CMD_ID_DEMO_1, rpc_cmd_handler_test));

    /* handler delete a valid cmd id will succeed */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_OK, esp_amp_rpc_server_del_service(server, RPC_CMD_ID_DEMO_1));

    /* handler delete an invalid cmd id will report error */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_ERR_NOT_FOUND, esp_amp_rpc_server_del_service(server, RPC_CMD_ID_DEMO_1));

    esp_amp_rpc_server_deinit(server);

    free(rpmsg_dev);
    vTaskDelay(pdMS_TO_TICKS(500));
}

TEST_CASE("RPC client exec cmd", "[esp_amp]")
{
    esp_amp_rpmsg_dev_t* rpmsg_dev = (esp_amp_rpmsg_dev_t*)(malloc(sizeof(esp_amp_rpmsg_dev_t)));
    TEST_ASSERT_NOT_NULL(rpmsg_dev);

    TEST_ASSERT(esp_amp_init() == 0);
    TEST_ASSERT(esp_amp_rpmsg_main_init(rpmsg_dev, 32, 64, false, false) == 0);

    static esp_amp_rpc_client_stg_t rpc_client_stg;
    esp_amp_rpc_client_cfg_t cfg = {
        .client_id = RPC_MAIN_CORE_CLIENT,
        .server_id = RPC_MAIN_CORE_SERVER,
        .rpmsg_dev = rpmsg_dev,
        .stg = &rpc_client_stg,
        .poll_cb = NULL,
        .poll_arg = NULL,
    };

    esp_amp_rpc_client_t client = esp_amp_rpc_client_init(&cfg);
    TEST_ASSERT_NOT_EQUAL(NULL, client);

    /* exec a valid cmd id will succeed */
    esp_amp_rpc_cmd_t cmd = { 0 };
    cmd.cmd_id = RPC_CMD_ID_DEMO_1;
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_OK, esp_amp_rpc_client_execute_cmd(client, &cmd));

    /* exec an invalid cmd id will report error */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_ERR_INVALID_ARG, esp_amp_rpc_client_execute_cmd(client, NULL));
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_ERR_INVALID_ARG, esp_amp_rpc_client_execute_cmd(NULL, &cmd));

    esp_amp_rpc_client_deinit(client);

    /* exec after deinit will report error */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_ERR_INVALID_STATE, esp_amp_rpc_client_execute_cmd(client, &cmd));

    free(rpmsg_dev);
    vTaskDelay(pdMS_TO_TICKS(500));
}

#define EVENT_SUBCORE_READY (1 << 0)

static void cmd_demo_cb(esp_amp_rpc_client_t client, esp_amp_rpc_cmd_t *cmd, void *arg)
{
    TaskHandle_t task = (TaskHandle_t)arg;
    BaseType_t need_yield = false;

    vTaskNotifyGiveFromISR(task, &need_yield);
    portYIELD_FROM_ISR(need_yield);
}

extern const uint8_t subcore_rpc_test_bin_start[] asm("_binary_subcore_test_rpc_bin_start");
extern const uint8_t subcore_rpc_test_bin_end[]   asm("_binary_subcore_test_rpc_bin_end");

TEST_CASE("RPC invalid cmd test", "[esp_amp]")
{
    static esp_amp_rpc_client_stg_t rpc_client_stg;
    static esp_amp_rpmsg_dev_t rpmsg_dev;

    /* init esp amp */
    assert(esp_amp_init() == 0);
    assert(esp_amp_rpmsg_main_init(&rpmsg_dev, 8, 128, false, false) == 0);
    esp_amp_rpmsg_intr_enable(&rpmsg_dev);

    /* Load firmware & start subcore */
    TEST_ASSERT_EQUAL(ESP_OK, esp_amp_load_sub(subcore_rpc_test_bin_start));
    ESP_ERROR_CHECK(esp_amp_start_subcore());

    /* wait for link up */
    assert((esp_amp_event_wait(EVENT_SUBCORE_READY, true, true, 10000) & EVENT_SUBCORE_READY) == EVENT_SUBCORE_READY);

    /* init client */
    esp_amp_rpc_client_cfg_t cfg = {
        .client_id = RPC_MAIN_CORE_CLIENT,
        .server_id = RPC_MAIN_CORE_SERVER,
        .rpmsg_dev = &rpmsg_dev,
        .stg = &rpc_client_stg,
        .poll_cb = NULL,
        .poll_arg = NULL,
    };
    esp_amp_rpc_client_t client = esp_amp_rpc_client_init(&cfg);
    TEST_ASSERT_NOT_EQUAL(NULL, client);

    /* exec an invalid cmd id expect ESP_AMP_RPC_STATUS_INVALID_CMD */
    esp_amp_rpc_cmd_t cmd = { 0 };
    cmd.cmd_id = RPC_CMD_ID_DEMO_1;
    cmd.cb = cmd_demo_cb;
    cmd.cb_arg = (void*)xTaskGetCurrentTaskHandle();
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_OK, esp_amp_rpc_client_execute_cmd(client, &cmd));
    ulTaskNotifyTake(true, pdMS_TO_TICKS(1000)); /* wait up to 1000ms */
    TEST_ASSERT_EQUAL(ESP_AMP_RPC_STATUS_INVALID_CMD, cmd.status);
}
