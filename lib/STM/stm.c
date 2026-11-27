#include "stm.h"

#include "stm32f4xx_hal.h"








void SystemClock_Config(void){

    RCC_OscInitTypeDef RCC_OscinitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkinitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

    RCC_OscinitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscinitStruct.HSIState = RCC_HSI_ON;
    RCC_OscinitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscinitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscinitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscinitStruct.PLL.PLLM = 16;
    RCC_OscinitStruct.PLL.PLLN = 336;
    RCC_OscinitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscinitStruct.PLL.PLLQ = 7;
    RCC_OscinitStruct.PLL.PLLR = 2;

    if (HAL_RCC_OscConfig(&RCC_OscinitStruct) != HAL_OK){

        Error_Handler();
    }

    RCC_ClkinitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkinitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkinitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkinitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkinitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkinitStruct, FLASH_LATENCY_2) != HAL_OK){

        Error_Handler();
    }


}

void GPIO_Init_All(void){

    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();


    GPIO_InitStruct.Pin = GPIO_PIN_8;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


    GPIO_InitStruct.Pin = GPIO_PIN_5;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;

    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);



}


void Error_Handler(void){

    __disable_irq();

    while (1){
    
    }
    

}

//void SysTick_Handler(void){
//    HAL_IncTick();
//}