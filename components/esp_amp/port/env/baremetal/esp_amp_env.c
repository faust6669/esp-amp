/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "esp_amp_platform.h"
#include "esp_amp_env.h"

static uint32_t s_critical_nesting = 0;
static uint32_t s_old_mstatus = 0;

void esp_amp_env_enter_critical(void)
{
    uint32_t old_mstatus = RV_CLEAR_CSR(mstatus, MSTATUS_MIE);
    /* only save mstatus when critical nesting is 0 */
    if (s_critical_nesting++ == 0) {
        s_old_mstatus = old_mstatus;
    }
}

void esp_amp_env_exit_critical(void)
{
    /* only restore mstatus when critical nesting is 0 */
    if (s_critical_nesting > 0) {
        s_critical_nesting--;
        if (s_critical_nesting == 0) {
            RV_SET_CSR(mstatus, s_old_mstatus & MSTATUS_MIE);
        }
    }
}

int esp_amp_env_in_isr(void)
{
    extern bool _bm_intr_nesting_cnt;
    return (_bm_intr_nesting_cnt > 0);
}

/* Queue API */
int esp_amp_env_queue_create(void **queue, uint32_t queue_len, uint32_t item_size)
{
    /* not implemented */
    return 0;
}

int esp_amp_env_queue_send(void *queue, void *data, uint32_t timeout_ms)
{
    /* not implemented */
    return -1;
}

int esp_amp_env_queue_recv(void *queue, void *data, uint32_t timeout_ms)
{
    /* not implemented */
    return -1;
}

void esp_amp_env_queue_delete(void *queue)
{
    return;
}
