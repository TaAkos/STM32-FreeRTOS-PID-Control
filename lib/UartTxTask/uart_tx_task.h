#ifndef UART_TX_TASK_H
#define UART_TX_TASK_H

#include <stdbool.h>
#include <stdint.h>

typedef struct 
{
    uint32_t response_enqueue_fail_count;
    uint32_t telemetry_drop_count;

    uint32_t dma_start_error_count;
    uint32_t dma_timeout_count;
} uart_tx_stats_t;

bool UartTx_Init(void);

bool UartTx_SubmitResponse(const uint8_t *data, uint16_t length);

bool UartTx_SubmitTelemetry(const uint8_t *data, uint16_t length);

bool UartTx_GetStats(uart_tx_stats_t *stats);

void UartTxTask(void *argument);


#endif