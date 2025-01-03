/*
 * SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdint.h>
#include <stdio.h>

#include "esp_amp.h"
#include "esp_attr.h"
#include "event.h"
#include "esp_amp_platform.h"

static int sw_intr_id0_handler_1(void *arg)
{
    printf("SUB: sw_intr_id0_handler_1() called\r\n");
    return 0;
}

static int sw_intr_id0_handler_2(void *arg)
{
    printf("SUB: sw_intr_id0_handler_2() called\r\n");
    return 0;
}

static int sw_intr_id1_handler_1(void *arg)
{
    printf("SUB: sw_intr_id1_handler_1() called\r\n");
    return 0;
}

static int sw_intr_id1_handler_2(void *arg)
{
    printf("SUB: sw_intr_id1_handler_2() called\r\n");
    return 0;
}

static int sw_intr_id2_handler_1(void *arg)
{
    printf("SUB: sw_intr_id2_handler_1() called\r\n");
    return 0;
}

static int sw_intr_id2_handler_2(void *arg)
{
    printf("SUB: sw_intr_id2_handler_2() called\r\n");
    return 0;
}


int main(void)
{
    printf("SUB: Hello!!\r\n");

    assert(esp_amp_init() == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_0, sw_intr_id0_handler_1, NULL) == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_0, sw_intr_id0_handler_2, NULL) == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_1, sw_intr_id1_handler_1, NULL) == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_1, sw_intr_id1_handler_2, NULL) == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_2, sw_intr_id2_handler_1, NULL) == 0);
    assert(esp_amp_sw_intr_add_handler(SW_INTR_ID_2, sw_intr_id2_handler_2, NULL) == 0);

    esp_amp_event_notify(EVENT_SUBCORE_READY);

    while (1) {
        printf("SUB: trigger intr 0 on maincore...\r\n");
        esp_amp_sw_intr_trigger(SW_INTR_ID_0);
        esp_amp_platform_delay_us(1000000);

        printf("SUB: trigger intr 1 on maincore...\r\n");
        esp_amp_sw_intr_trigger(SW_INTR_ID_1);
        esp_amp_platform_delay_us(1000000);

        printf("SUB: trigger intr 2 on maincore...\r\n");
        esp_amp_sw_intr_trigger(SW_INTR_ID_2);
        esp_amp_platform_delay_us(1000000);

        printf("SUB: trigger intr 3 on maincore...\r\n");
        esp_amp_sw_intr_trigger(SW_INTR_ID_3);
        esp_amp_platform_delay_us(1000000);
    }

    printf("SUB: Bye!!\r\n");
    return 0;
}
