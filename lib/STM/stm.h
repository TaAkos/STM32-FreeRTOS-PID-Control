#ifndef STM_H
#define STM_H

#include "stm32f4xx_hal.h"


void SystemClock_Config(void);

void GPIO_Init_All(void);

void Error_Handler(void);

void SysTick_Handler(void);


#endif