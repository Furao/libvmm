/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#pragma once

#include <microkit.h>
#include <sddf/util/string.h>
#include <sddf/serial/queue.h>
#include <stdint.h>

/* Number of clients */
#define SERIAL_NUM_CLIENTS 3

/* Only support transmission and not receive. */
#define SERIAL_TX_ONLY 0

/* Associate a colour with each client's output. */
#define SERIAL_WITH_COLOUR 1

/* Control character to switch input stream - ctrl \. To input character input twice. */
#define SERIAL_SWITCH_CHAR 28

/* Control character to terminate client number input. */
#define SERIAL_TERMINATE_NUM '\r'

/* Default baud rate of the uart device */
#define UART_DEFAULT_BAUD 115200

/* String to be printed to start console input */
#define SERIAL_CONSOLE_BEGIN_STRING "Begin input\n"
#define SERIAL_CONSOLE_BEGIN_STRING_LEN 13

#define SERIAL_CLI0_NAME "CLIENT_VMM-1"
#define SERIAL_CLI1_NAME "CLIENT_VMM-2"
#define SERIAL_CLI2_NAME "BLK_DRIVER_VMM"
#define SERIAL_VIRT_RX_NAME "serial_virt_rx"
#define SERIAL_VIRT_TX_NAME "serial_virt_tx"
#define SERIAL_DRIVER_NAME "uart_driver"

#define SERIAL_QUEUE_CAPACITY                   0x1000
#define SERIAL_DATA_REGION_CAPACITY             0x2000

#define SERIAL_TX_DATA_REGION_CAPACITY_DRIV     SERIAL_DATA_REGION_CAPACITY
#define SERIAL_TX_DATA_REGION_CAPACITY_CLI0     SERIAL_DATA_REGION_CAPACITY
#define SERIAL_TX_DATA_REGION_CAPACITY_CLI1     SERIAL_DATA_REGION_CAPACITY
#define SERIAL_TX_DATA_REGION_CAPACITY_CLI2     SERIAL_DATA_REGION_CAPACITY

#define SERIAL_RX_DATA_REGION_CAPACITY_DRIV     SERIAL_DATA_REGION_CAPACITY
#define SERIAL_RX_DATA_REGION_CAPACITY_CLI0     SERIAL_DATA_REGION_CAPACITY
#define SERIAL_RX_DATA_REGION_CAPACITY_CLI1     SERIAL_DATA_REGION_CAPACITY
#define SERIAL_RX_DATA_REGION_CAPACITY_CLI2     SERIAL_DATA_REGION_CAPACITY

#define SERIAL_MAX_TX_DATA_CAPACITY MAX(SERIAL_TX_DATA_REGION_CAPACITY_DRIV, MAX(SERIAL_TX_DATA_REGION_CAPACITY_CLI2, MAX(SERIAL_TX_DATA_REGION_CAPACITY_CLI0, SERIAL_TX_DATA_REGION_CAPACITY_CLI1)))
#define SERIAL_MAX_RX_DATA_CAPACITY MAX(SERIAL_RX_DATA_REGION_CAPACITY_DRIV, MAX(SERIAL_RX_DATA_REGION_CAPACITY_CLI2, MAX(SERIAL_RX_DATA_REGION_CAPACITY_CLI0, SERIAL_RX_DATA_REGION_CAPACITY_CLI1)))
#define SERIAL_MAX_DATA_CAPACITY MAX(SERIAL_MAX_TX_DATA_CAPACITY, SERIAL_MAX_RX_DATA_CAPACITY)
_Static_assert(SERIAL_MAX_DATA_CAPACITY < UINT32_MAX,
               "Data regions must be smaller than UINT32 max to correctly use queue data structure.");

static inline void serial_cli_queue_init_sys(char *pd_name, serial_queue_handle_t *rx_queue_handle,
                                             serial_queue_t *rx_queue, char *rx_data, serial_queue_handle_t *tx_queue_handle,
                                             serial_queue_t *tx_queue, char *tx_data)
{
    if (!sddf_strcmp(pd_name, SERIAL_CLI0_NAME)) {
        serial_queue_init(rx_queue_handle, rx_queue, SERIAL_RX_DATA_REGION_CAPACITY_CLI0, rx_data);
        serial_queue_init(tx_queue_handle, tx_queue, SERIAL_TX_DATA_REGION_CAPACITY_CLI0, tx_data);
    } else if (!sddf_strcmp(pd_name, SERIAL_CLI1_NAME)) {
        serial_queue_init(rx_queue_handle, rx_queue, SERIAL_RX_DATA_REGION_CAPACITY_CLI1, rx_data);
        serial_queue_init(tx_queue_handle, tx_queue, SERIAL_TX_DATA_REGION_CAPACITY_CLI1, tx_data);
    } else if (!sddf_strcmp(pd_name, SERIAL_CLI2_NAME)) {
        serial_queue_init(rx_queue_handle, rx_queue, SERIAL_RX_DATA_REGION_CAPACITY_CLI2, rx_data);
        serial_queue_init(tx_queue_handle, tx_queue, SERIAL_TX_DATA_REGION_CAPACITY_CLI2, tx_data);
    }
}

static inline void serial_virt_queue_init_sys(char *pd_name, serial_queue_handle_t *cli_queue_handle,
                                              serial_queue_t *cli_queue, char *cli_data)
{
    uintptr_t cli_queue_ptr = (uintptr_t)cli_queue;
    if (!sddf_strcmp(pd_name, SERIAL_VIRT_RX_NAME)) {
        serial_queue_init(cli_queue_handle, cli_queue, SERIAL_RX_DATA_REGION_CAPACITY_CLI0, cli_data);
        serial_queue_init(&cli_queue_handle[1], (serial_queue_t *)(cli_queue_ptr + SERIAL_QUEUE_CAPACITY),
                          SERIAL_RX_DATA_REGION_CAPACITY_CLI1, (char *)(cli_data + SERIAL_RX_DATA_REGION_CAPACITY_CLI0));
        serial_queue_init(&cli_queue_handle[2], (serial_queue_t *)(cli_queue_ptr + 2 * SERIAL_QUEUE_CAPACITY),
                          SERIAL_RX_DATA_REGION_CAPACITY_CLI2,
                          (char *)(cli_data + SERIAL_RX_DATA_REGION_CAPACITY_CLI0 + SERIAL_RX_DATA_REGION_CAPACITY_CLI1));
    } else if (!sddf_strcmp(pd_name, SERIAL_VIRT_TX_NAME)) {
        serial_queue_init(cli_queue_handle, cli_queue, SERIAL_TX_DATA_REGION_CAPACITY_CLI0, cli_data);
        serial_queue_init(&cli_queue_handle[1], (serial_queue_t *)(cli_queue_ptr + SERIAL_QUEUE_CAPACITY),
                          SERIAL_TX_DATA_REGION_CAPACITY_CLI1, cli_data + SERIAL_TX_DATA_REGION_CAPACITY_CLI0);
        serial_queue_init(&cli_queue_handle[2], (serial_queue_t *)(cli_queue_ptr + 2 * SERIAL_QUEUE_CAPACITY),
                          SERIAL_TX_DATA_REGION_CAPACITY_CLI2,
                          (char *)(cli_data + SERIAL_TX_DATA_REGION_CAPACITY_CLI0 + SERIAL_TX_DATA_REGION_CAPACITY_CLI1));
    }
}

#if SERIAL_WITH_COLOUR
static inline void serial_channel_names_init(char **client_names)
{
    client_names[0] = SERIAL_CLI0_NAME;
    client_names[1] = SERIAL_CLI1_NAME;
    client_names[2] = SERIAL_CLI2_NAME;
}
#endif