#include "uart_tx_task.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "protocol.h"
#include "uart_driver.h"

#include <string.h>

#define UART_TX_QUEUE_LENGTH 12U
#define UART_TX_RESPONSE_RESERVE 2U
#define UART_TX_DMA_TIMEOUT_MS 20U


typedef struct 
{
    uint16_t length;
    uint8_t data[PROTOCOL_MAX_FRAME_SIZE];
} uart_tx_msg_t;

static QueueHandle_t uart_tx_queue = NULL;

static TaskHandle_t uart_tx_task_handle = NULL;

static uart_tx_stats_t uart_tx_stats = {0};

static void UartTx_DmaCompleteFromISR(void){

    if (uart_tx_task_handle == NULL)
    {
        return;
    }

    BaseType_t higher_priority_task_woken = pdFALSE;

    vTaskNotifyGiveFromISR(uart_tx_task_handle, &higher_priority_task_woken);

    portYIELD_FROM_ISR(higher_priority_task_woken);

    
}

bool UartTx_Init(void){

    uart_tx_queue = xQueueCreate(UART_TX_QUEUE_LENGTH, sizeof(uart_tx_msg_t));

    if (uart_tx_queue == NULL)
    {
        return false;
    }

    memset(&uart_tx_stats, 0, sizeof(uart_tx_stats));

    UART2_RegisterTxCompleteCallback(UartTx_DmaCompleteFromISR);

    return true; 

}

bool UartTx_SubmitResponse(const uint8_t *data, uint16_t length){

    if (uart_tx_queue == NULL || data == NULL || length == 0U || length > PROTOCOL_MAX_FRAME_SIZE)
    {
        return false;
    }
    
    uart_tx_msg_t msg = {0};

    msg.length = length;

    memcpy(msg.data, data, length);

    if (xQueueSendToBack(uart_tx_queue, &msg, 0U) != pdPASS)
    {
        taskENTER_CRITICAL();
        uart_tx_stats.response_enqueue_fail_count++;
        taskEXIT_CRITICAL();

        return false;
    }

    return true;
    
}

bool UartTx_SubmitTelemetry(const uint8_t *data, uint16_t length){

    if (uart_tx_queue == NULL || data == NULL || length == 0U || length > PROTOCOL_MAX_FRAME_SIZE)
    {
        return false;
    }

    if (uxQueueSpacesAvailable(uart_tx_queue) <= UART_TX_RESPONSE_RESERVE)
    {
        taskENTER_CRITICAL();
        uart_tx_stats.telemetry_drop_count++;
        taskEXIT_CRITICAL();

        return false;
    }

    uart_tx_msg_t msg = {0};

    msg.length = length;

    memcpy(msg.data, data, length);

    if (xQueueSendToBack(uart_tx_queue, &msg, 0U) != pdPASS)
    {
        taskENTER_CRITICAL();
        uart_tx_stats.telemetry_drop_count++;
        taskEXIT_CRITICAL();

        return false;
    }

    return true;
    
}

void UartTxTask(void *argument){

    (void)argument;

    uart_tx_task_handle =xTaskGetCurrentTaskHandle();

    uart_tx_msg_t msg;

    for (;;)
    {
        if (xQueueReceive(uart_tx_queue, &msg, portMAX_DELAY) != pdPASS)
        {
            continue;
        }

        (void)ulTaskNotifyTake(pdTRUE, 0U);

        if (!UART2_SendDMA(msg.data, msg.length))
        {
            taskENTER_CRITICAL();
            uart_tx_stats.dma_start_error_count++;
            taskEXIT_CRITICAL();

            continue;
        }

        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(UART_TX_DMA_TIMEOUT_MS)) == 0U)
        {
            taskENTER_CRITICAL();
            uart_tx_stats.dma_timeout_count++;
            taskEXIT_CRITICAL();

            (void)UART2_AbortTransmit();
        }  
        
    }
    
}

bool UartTx_GetStats(uart_tx_stats_t *stats)
{
    if (stats == NULL)
    {
        return false;
    }

    taskENTER_CRITICAL();

    *stats = uart_tx_stats;

    taskEXIT_CRITICAL();

    return true;
}