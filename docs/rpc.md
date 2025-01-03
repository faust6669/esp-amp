# RPC

## Overview

RPC stands for Remote Procedure Call. It is a communication protocol that allows a program to execute a procedure (or function) in another address space. This is done as if the procedure were a local function call, hiding the complexity of the underlying transport. ESP-AMP offers an easy-to-use RPC framework on top of RPMsg to facilitate inter-process communication between maincore and subcore.

## Design

ESP-AMP RPC is technically a wrapper of RPMsg. It provides a simple set of APIs to define RPC services on one core and call RPC commands on another. It is designed to be flexible and extensible, easily portable to different environments. 

RPC commands can be blocking or non-blocking, with or without response.  Only one pending request is allowed at a time. ESP-AMP RPC client API returns immediately after the command is sent. You can send one command immediately after another, if the previous command does not require response. If you need to wait for the response, you can use the blocking APIs provided by the environment where you implement your RPC client.

ESP RPC does not implement or integrate serialization library. You are free to choose your favorite serialization library to construct RPC commands. RPC commands are sent over RPMsg in format of `esp_amp_rpc_pkt_t`. The following table lists the fields of `esp_amp_rpc_pkt_t`.

| Field | Size | Description |
|-------|------|-------------|
| cmd_id | 2 bytes | Command ID |
| msg_id | 2 bytes | Unique ID of the message, used to match request and response |
| status | 2 bytes | Status of the command |
| msg_len | 2 bytes | Length of the message |
| msg_data | variable | Message data |

## Usage

### Client APIs

A complete RPC client workflow consists of the following steps:
1. Create RPC client
2. Create & execute RPC command: create RPC command and send it to server.
3. Process result & error handling: process the result of RPC command. Handle errors if any.

#### 1. Create RPC Client

The following code creates a RPC client. This client is registered to the server with ID `RPC_DEMO_SERVER`.

``` c
#define RPC_DEMO_CLIENT 0x0001
#define RPC_DEMO_SERVER 0x1001

static esp_amp_rpmsg_dev_t rpmsg_dev;
static esp_amp_rpc_client_stg_t rpc_client_stg;

esp_amp_rpc_client_cfg_t cfg = {
    .client_id = RPC_DEMO_CLIENT,
    .server_id = RPC_DEMO_SERVER,
    .rpmsg_dev = &rpmsg_dev,
    .stg = &rpc_client_stg,
};
esp_amp_rpc_client_t client = esp_amp_rpc_client_init(&cfg);
```

**NOTE**: One client can only connect to one server.

#### 2. Create & Execute RPC Command

To create a new RPC command, you need to:
1. Identify the command ID: the command ID is a unique identifier for the command. Make sure it matches the command ID defined on the server side.
2. Construct RPC packet: serialize the parameters of RPC command into a buffer.
3. Identify the command type: whether the command is blocking or non-blocking, with or without result.
4. Define the callback: this callback will be called when the command result is received.

The following code creates a RPC command to print on server console. The command ID is `RPC_CMD_ID_PRINTF`. The command is non-blocking and does not expect a response, so the callback is set to NULL.

``` c
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
```

In this example, the serialization is simply done by memcpy. The structure of the serialized command data is defined as follows:

``` c
typedef struct {
    int len;
    char buf[100];
} printf_params_in_t;
```

To execute the RPC command, call `esp_amp_rpc_client_execute_cmd`, which is defined as follows:

``` c
int esp_amp_rpc_client_execute_cmd(esp_amp_rpc_client_t client, esp_amp_rpc_cmd_t *cmd);
```
* `client`: the RPC client.
* `cmd`: the RPC command to be executed.
* Return values:
  * ESP_AMP_RPC_OK on success
  * ESP_AMP_RPC_ERR_INVALID_ARG if invalid argument
  * ESP_AMP_RPC_ERR_INVALID_SIZE if invalid size
  * ESP_AMP_RPC_ERR_NO_MEM if out of memory.

This function is non-blocking. It returns immediately after the command is sent. Return value reflects any error before the command is sent. Error related to transport or server execution will be indicated by `cmd.status`. The command result can be obtained from `cmd.resp_data` after the command is executed, if any.

The structure of RPC command is defined as follows:

``` c
struct esp_amp_rpc_cmd_t {
    uint16_t cmd_id; /* command ID */
    uint16_t status; /* status */
    uint16_t req_len; /* request data length */
    uint16_t resp_len; /* response data length */
    uint8_t *req_data; /* request data */
    uint8_t *resp_data; /* response data */
    esp_amp_rpc_app_cb_t cb; /* callback function */
    void *cb_arg; /* callback argument */
};
```

To make the command blocking, you can rely on any blocking APIs provided by the environment where you implement your RPC client. Here is an example of utilizing TaskNotify in FreeRTOS to make the command blocking:

``` c
int err = esp_amp_rpc_client_execute_cmd(client, &cmd);
if (err == ESP_AMP_RPC_OK) {
    ulTaskNotifyTake(true, pdMS_TO_TICKS(1000)); /* wait up to 1000 ms */
    if (cmd.status == ESP_AMP_RPC_STATUS_OK) {
        *ret = out_params.ret;
    } else {
        printf("client: rpc cmd failed, status: %x\n", cmd.status);
        err = ESP_AMP_RPC_FAIL;
    }
}
```

To wake up the blocked task once the result is ready, simply notify it from the callback:

``` c
void rpc_call_cb(esp_amp_rpc_client_t client, esp_amp_rpc_cmd_t *cmd, void *arg)
{
    TaskHandle_t task = (TaskHandle_t)arg;
    BaseType_t need_yield = false;

    vTaskNotifyGiveFromISR(task, &need_yield);
    portYIELD_FROM_ISR(need_yield);
}
```

You can even make the command asynchronous by handling it in the callback. This way, you don't need to block the calling task. However, blocking APIs should be avoided in callback executed in ISR context.

#### 3. Process Result & Error Handling

Once the command is executed and sent back by the server, the result can be obtained `cmd.resp_data`. `cmd.status` indicates the status of the command execution. The following table lists the possible values of `cmd.status`:

| Value | Hex | Description |
|-------|-----|-------------|
| ESP_AMP_RPC_STATUS_OK | 0x0000 | Command executed successfully. |
| ESP_AMP_RPC_STATUS_SERVER_BUSY | 0xffff | Server is busy. Command is dropped. |
| ESP_AMP_RPC_STATUS_INVALID_CMD | 0xfffe | Command ID cannot be found. |
| ESP_AMP_RPC_STATUS_EXEC_FAILED | 0xfffd | Error happened when executing command. |
| ESP_AMP_RPC_STATUS_PENDING | 0xfffc | Command is still pending. Interpreted as timeout if the command is blocking. |

You can define more status code to indicate the command execution status on server side.

### Server APIs

A complete RPC server workflow consists of the following steps:
1. Create RPC server
2. Register command handlers: register command handlers to process RPC commands from clients.
3. Poll RPC commands: poll RPC commands from clients. This step is handled by the RPC server internally.
4. Process RPC commands: process RPC commands by calling the corresponding command handler.
5. Send result to RPC client: send execution result to client. This step is handled by the RPC server internally.

#### 1. Create RPC Server

The following code creates a RPC server.

``` c
#define RPC_DEMO_SERVER 0x1001
static esp_amp_rpmsg_dev_t rpmsg_dev;
static esp_amp_rpc_server_stg_t rpc_server_stg;
static uint8_t req_buf[128];
static uint8_t resp_buf[128];
static uint8_t srv_tbl_stg[sizeof(esp_amp_rpc_service_t) * RPC_SRV_NUM];

esp_amp_rpc_server_cfg_t cfg = {
    .rpmsg_dev = &rpmsg_dev,
    .server_id = RPC_DEMO_SERVER,
    .queue_len = 8,
    .stg = &rpc_server_stg,
    .req_buf_len = sizeof(req_buf),
    .resp_buf_len = sizeof(resp_buf),
    .req_buf = req_buf,
    .resp_buf = resp_buf,
    .srv_tbl_len = 2,
    .srv_tbl_stg = srv_tbl_stg,
};
esp_amp_rpc_server_t server = esp_amp_rpc_server_init(&cfg);
```

Things to **NOTE**:
* One server can handle commands from multiple clients.
* Make sure `req_buf` and `resp_buf` are available for the lifetime of the server and large enough to hold the largest request.

#### 2. Register Command Handlers

The following API can be used to register command handlers to server's service table.

``` c
int esp_amp_rpc_server_add_service(esp_amp_rpc_server_t server, uint16_t cmd_id, esp_amp_rpc_cmd_handler_t handler);
```

* `server`: the RPC server.
* `cmd_id`: the command ID.
* `handler`: the command handler.
* Return values:
  * ESP_AMP_RPC_OK on success
  * ESP_AMP_RPC_NO_MEM if out of memory
  * ESP_AMP_RPC_ERR_INVALID_ARG if server or handler is NULL
  * ESP_AMP_RPC_ERR_EXIST if command id already exists

The prototype of command handler is as follows:

``` c
typedef void (*esp_amp_rpc_cmd_handler_t)(esp_amp_rpc_cmd_t *cmd);
```

Only one command handler can be registered for a command ID. If you want to update the command handler, you need to unregister the old one first.

``` c
int esp_amp_rpc_server_del_service(esp_amp_rpc_server_t server, uint16_t cmd_id);
```

* `server`: the RPC server.
* `cmd_id`: the command ID.
* Return values:
  * ESP_AMP_RPC_OK on success
  * ESP_AMP_RPC_ERR_INVALID_ARG if server is NULL
  * ESP_AMP_RPC_ERR_NOT_FOUND if command id does not exist

#### 3. Poll RPC Commands

RPC server is similar to any RPMsg endpoint. It relies on RPMsg's polling or notification mechanism to handle incoming RPC comamnds. Please refer to the [RPMsg doc](./rpmsg.md) for more details.

In baremetal environment, we recommend polling mechanism. Server will poll commands from clients in non-isr context and call the corresponding command handler immediately without context switch.

In FreeRTOS environment, we recommend notification mechanism. Server will poll commands from clients in isr context and call the corresponding command handler in non-isr context. Incoming commands will be queued. If the queue is full, the command will be dropped and the server will return `ESP_AMP_RPC_STATUS_SERVER_BUSY` to the client. Call the following API in non-isr context to handle commands from clients.

``` c
void esp_amp_rpc_server_run(esp_amp_rpc_server_t server, uint32_t timeout_ms);
```

* `server`: the RPC server.
* `timeout_ms`: the timeout in milliseconds. If the timeout is 0, the function will return immediately. If the timeout is non-zero, the function will block until a command is received or the timeout is reached.

#### 4. Process RPC Commands

Once there is any incoming RPC command, ESP-AMP RPC server will traverse its service table to find the corresponding handler. The following code demostrates an example of command handler. `memcpy` is used to deserialize the incoming data and serialize the outgoing data.

``` c
int add(int a, int b)
{
    printf("server: executing add(%d, %d)\r\n", a, b);
    return a + b;
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
```

#### 5. Send Result to RPC Client

You don't need to do anything to send the result back to client. The RPC server will automatically send the result back to client.

## Application Examples

* [maincore_client_subcore_server](../examples/rpc/maincore_client_subcore_server): demonstrates how to initiate an RPC client in FreeRTOS environment on maincore side and an RPC server in bare-metal environment on subcore side.
* [subcore_client_maincore_server](../examples/rpc/subcore_client_maincore_server): demonstrates how to initiate an RPC client in bare-metal environment on subcore side and an RPC client in FreeRTOS environment on maincore side.
