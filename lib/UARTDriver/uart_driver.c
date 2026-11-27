#include "uart_driver.h"

#include "stm.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


#define UART_RX_DMA_BUFFER_SIZE 256U
#define UART_RX_RING_BUFFER_SIZE 512U



static UART_HandleTypeDef huart2;

static DMA_HandleTypeDef hdma_usart2_rx;
static DMA_HandleTypeDef hdma_usart2_tx;

static uint8_t rx_dma_buffer[UART_RX_DMA_BUFFER_SIZE];

static volatile uint8_t rx_ring_buffer[UART_RX_RING_BUFFER_SIZE];

static volatile uint16_t rx_head = 0U;
static volatile uint16_t rx_tail = 0U;

static uint16_t rx_dma_old_position = 0U;

static volatile uint32_t rx_overflow_count = 0U;
static volatile uint32_t uart_error_count = 0U;

static uart_tx_complete_callback_t tx_complete_callback = NULL;
static uart_rx_data_callback_t rx_data_callback = NULL;




bool UART2_Init(void){

    GPIO_InitTypeDef gpio_init = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();


    gpio_init.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    gpio_init.Mode = GPIO_MODE_AF_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.Alternate = GPIO_AF7_USART2;

    HAL_GPIO_Init(GPIOA, &gpio_init);


    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = USART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&huart2) != HAL_OK ){

        return false;

    }

    hdma_usart2_rx.Instance = DMA1_Stream5;
    hdma_usart2_rx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx.Init.Mode = DMA_CIRCULAR;
    hdma_usart2_rx.Init.Priority = DMA_PRIORITY_HIGH;
    hdma_usart2_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
    {
        return false;
    }

    __HAL_LINKDMA(&huart2, hdmarx, hdma_usart2_rx);

    hdma_usart2_tx.Instance = DMA1_Stream6;
    hdma_usart2_tx.Init.Channel = DMA_CHANNEL_4;
    hdma_usart2_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart2_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_tx.Init.Mode = DMA_NORMAL;
    hdma_usart2_tx.Init.Priority = DMA_PRIORITY_MEDIUM;
    hdma_usart2_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&hdma_usart2_tx) != HAL_OK)
    {
        return false;
    }

    __HAL_LINKDMA(&huart2, hdmatx, hdma_usart2_tx);
    

    HAL_NVIC_SetPriority(USART2_IRQn, 6U, 0U);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    
    HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 6U, 0U);
    HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

    HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 6U, 0U);
    HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);

    rx_head = 0U;
    rx_tail = 0U;
    rx_dma_old_position = 0U;

    rx_overflow_count = 0U;
    uart_error_count = 0U;

    return true;
}

static void UART2_PushRxByte(uint8_t byte){

    uint16_t next_head = (uint16_t)((rx_head + 1U) % UART_RX_RING_BUFFER_SIZE);

    if (next_head == rx_tail)
    {
        rx_overflow_count++;
        return;
    }

    rx_ring_buffer[rx_head] = byte;
    rx_head = next_head;
    
}

static void UART2_ProcessRxRange(uint16_t start, uint16_t end){

    for (uint16_t i = start; i < end; i++)
    {
        UART2_PushRxByte(rx_dma_buffer[i]);
    }
    
}

bool UART2_StartReceive(void){

    if (huart2.RxState != HAL_UART_STATE_READY)
    {
        return false;
    }
    

    rx_dma_old_position = 0U;

    HAL_StatusTypeDef status = HAL_UARTEx_ReceiveToIdle_DMA(&huart2, rx_dma_buffer, UART_RX_DMA_BUFFER_SIZE);

    if (status != HAL_OK)
    {
        return false;
    }
    
    

    return true;

}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size){

    if (huart == NULL || huart->Instance != USART2)
    {
        return;
    }

    if (size > UART_RX_DMA_BUFFER_SIZE)
    {
        uart_error_count++;
        return;
    }

    bool new_data = false;

    if (size > rx_dma_old_position)
    {
        UART2_ProcessRxRange(rx_dma_old_position, size);
    } else if (size < rx_dma_old_position)
    {
        UART2_ProcessRxRange(rx_dma_old_position, UART_RX_DMA_BUFFER_SIZE);

        UART2_ProcessRxRange(0U, size);
    }

    rx_dma_old_position = size;

    if (new_data && rx_data_callback != NULL)
    {   
        rx_data_callback();
    }

}

bool UART2_ReadByte(uint8_t *byte){

    if (byte == NULL)
    {
        return false;
    }

    if (rx_tail == rx_head)
    {
        return false;
    }
    
    *byte = rx_ring_buffer[rx_tail];

    rx_tail = (uint16_t)((rx_tail + 1U) % UART_RX_RING_BUFFER_SIZE);

    return true;

}

bool UART2_SendDMA(const uint8_t *data, uint16_t length){

    if (data == NULL ||length == 0U)
    {
        return false;
    }

    return HAL_UART_Transmit_DMA(&huart2, (uint8_t *)data, length) == HAL_OK;
    
}

bool UART2_AbortTransmit(void){

    return HAL_UART_AbortTransmit(&huart2) == HAL_OK;
}

void UART2_RegisterTxCompleteCallback(uart_tx_complete_callback_t callback){

    tx_complete_callback = callback;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){

    if (huart != NULL && huart->Instance == USART2 && tx_complete_callback != NULL)
    {
        tx_complete_callback();
    }
    
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart){

    if (huart == NULL || huart->Instance != USART2)
    {
        return;
    }

    uart_error_count++;

    if (huart->RxState == HAL_UART_STATE_READY)
    {
        (void)UART2_StartReceive();
    }
    
}

uint32_t UART2_GetRxOverflowCount(void){

    return rx_overflow_count;
}

uint32_t UART2_GetErrorCount(void){

    return uart_error_count;
}

void USART2_IRQHandler(void){
    
    HAL_UART_IRQHandler(&huart2);
}


void DMA1_Stream5_IRQHandler(void){
    
    HAL_DMA_IRQHandler(&hdma_usart2_rx);
}


void DMA1_Stream6_IRQHandler(void){
    
    HAL_DMA_IRQHandler(&hdma_usart2_tx);
}

void UART2_RegisterRxDataCallback(uart_rx_data_callback_t callback){
    
    rx_data_callback = callback;
}