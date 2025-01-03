/*
* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
*
* SPDX-License-Identifier: Apache-2.0
*/

#pragma once

#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Enter critical section
 *
 * @note This function can be used to protect critical sections of code.
 * Nested critical section is allowed. The critical section will be
 * exited when the nesting count reaches 0.
 */
void esp_amp_env_enter_critical(void);

/**
 * @brief Exit critical section
 *
 * @note This function can be used to exit critical sections of code.
 * Ensure each call to esp_amp_env_enter_critical() is matched by a
 * call to esp_amp_env_exit_critical().
 */
void esp_amp_env_exit_critical(void);

/**
 * @brief Check if the current context is in interrupt service routine
 *
 * @retval 0 if the current context is not in interrupt service routine
 * @retval 1 if the current context is in interrupt service routine
 */
int esp_amp_env_in_isr(void);

/**
 * @brief Create a queue
 *
 * @param queue queue handle
 * @param queue_len queue length
 * @param item_size item size
 */
int esp_amp_env_queue_create(void **queue, uint32_t queue_len, uint32_t item_size);

/**
 * @brief Send data to a queue
 *
 * @param queue queue handle
 * @param data data to send
 * @param timeout timeout in ms
 * @return int 0 if success
 * @return int -1 if failed
 */
int esp_amp_env_queue_send(void *queue, void *data, uint32_t timeout_ms);

/**
 * @brief Receive data from a queue
 *
 * @param queue queue handle
 * @param data data to receive
 * @param timeout timeout in ms
 * @return int 0 if success
 * @return int -1 if failed
 */
int esp_amp_env_queue_recv(void *queue, void *data, uint32_t timeout_ms);

/**
 * @brief Delete a queue
 */
void esp_amp_env_queue_delete(void *queue);

#ifdef __cplusplus
}
#endif
