#ifndef UART_DRIVER_H
#define UART_DRIVER_H

#include "stm32f4xx_hal.h"

#include <stdbool.h>
#include <stdint.h>

typedef void (*uart_rx_data_callback_t)(void);
typedef void (*uart_tx_complete_callback_t)(void);

bool UART2_Init(void);

bool UART2_StartReceive(void);

bool UART2_ReadByte(uint8_t *byte);

bool UART2_SendDMA(const uint8_t *data, uint16_t length);

bool UART2_AbortTransmit(void);

void UART2_RegisterTxCompleteCallback(uart_tx_complete_callback_t callback);

uint32_t UART2_GetRxOverflowCount(void);

uint32_t UART2_GetErrorCount(void);

void UART2_RegisterRxDataCallback(uart_rx_data_callback_t callback);


#endif