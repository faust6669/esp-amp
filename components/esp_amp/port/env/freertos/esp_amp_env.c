/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "stdint.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_amp_env.h"

/* spinlock is not used in FreeRTOS unicore mode */
static int lock;

void esp_amp_env_enter_critical()
{
    if (xPortInIsrContext()) {
        portENTER_CRITICAL_ISR(&lock);
    } else {
        portENTER_CRITICAL(&lock);
    }
}

void esp_amp_env_exit_critical()
{
    if (xPortInIsrContext()) {
        portEXIT_CRITICAL_ISR(&lock);
    } else {
        portEXIT_CRITICAL(&lock);
    }
}

int esp_amp_env_in_isr(void)
{
    return xPortInIsrContext();
}

/* Queue API */
int esp_amp_env_queue_create(void **queue, uint32_t queue_len, uint32_t item_size)
{
    *queue = xQueueCreate(queue_len, item_size);
    if (*queue == NULL) {
        return -1;
    }
    return 0;
}

int esp_amp_env_queue_send(void *queue, void *data, uint32_t timeout_ms)
{
    BaseType_t need_yield = pdFALSE;
    if (esp_amp_env_in_isr()) {
        if (xQueueSendFromISR(queue, data, &need_yield) == pdPASS) {
            portYIELD_FROM_ISR(need_yield);
            return 0;
        }
    } else {
        uint32_t timeout = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
        if (xQueueSend(queue, data, timeout) == pdPASS) {
            return 0;
        }
    }
    return -1;
}

int esp_amp_env_queue_recv(void *queue, void *data, uint32_t timeout_ms)
{
    BaseType_t need_yield = pdFALSE;
    if (esp_amp_env_in_isr()) {
        if (xQueueReceiveFromISR(queue, data, &need_yield) == pdPASS) {
            portYIELD_FROM_ISR(need_yield);
            return 0;
        }
    } else {
        uint32_t timeout = (timeout_ms == portMAX_DELAY) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
        if (xQueueReceive(queue, data, timeout) == pdPASS) {
            return 0;
        }
    }
    return -1;
}

void esp_amp_env_queue_delete(void *queue)
{
    vQueueDelete(queue);
}
