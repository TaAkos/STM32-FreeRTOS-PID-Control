#include "stm32f4xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"

extern void xPortSysTickHandler(void);


void SysTick_Handler(void){


    HAL_IncTick();

    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
    {
        xPortSysTickHandler();
    }

}

void vApplicationMallocFailedHook(void){


    taskDISABLE_INTERRUPTS();

    for(;;){

    }

}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName){


    (void)xTask;
    (void)pcTaskName;

    taskDISABLE_INTERRUPTS();

    for (;;)
    {
        
    }
    

}