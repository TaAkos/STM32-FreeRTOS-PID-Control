#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H


#include <stdint.h>


extern uint32_t SystemCoreClock;

/* ================= Scheduler ================= */

#define configUSE_PREEMPTION                    1
#define configUSE_TIME_SLICING                  1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION 1

#define configCPU_CLOCK_HZ                      ( SystemCoreClock )
#define configTICK_RATE_HZ                      1000U

#define configMAX_PRIORITIES                    7U
#define configMINIMAL_STACK_SIZE                128U
#define configMAX_TASK_NAME_LEN                 16U

#define configTICK_TYPE_WIDTH_IN_BITS           TICK_TYPE_WIDTH_32_BITS

#define configIDLE_SHOULD_YIELD                 1
#define configUSE_TICKLESS_IDLE                 0

/* ================= RTOS primitives ================= */

#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             0
#define configUSE_COUNTING_SEMAPHORES           1

#define configTASK_NOTIFICATION_ARRAY_ENTRIES   1U
#define configQUEUE_REGISTRY_SIZE               8U

/* ================= Memory ================= */

#define configSUPPORT_STATIC_ALLOCATION         0
#define configSUPPORT_DYNAMIC_ALLOCATION        1

#define configTOTAL_HEAP_SIZE                   ( ( size_t ) ( 32U * 1024U ) )
#define configAPPLICATION_ALLOCATED_HEAP        0

/* ================= Software timers ================= */

#define configUSE_TIMERS                        1
#define configTIMER_TASK_PRIORITY               2U
#define configTIMER_QUEUE_LENGTH                8U
#define configTIMER_TASK_STACK_DEPTH            256U

/* ================= Debug / safety ================= */

#define configCHECK_FOR_STACK_OVERFLOW          2
#define configUSE_MALLOC_FAILED_HOOK            1

#define configUSE_IDLE_HOOK                     0
#define configUSE_TICK_HOOK                     0

#define configUSE_TRACE_FACILITY                1
#define configUSE_STATS_FORMATTING_FUNCTIONS    0
#define configGENERATE_RUN_TIME_STATS           0

#define configUSE_NEWLIB_REENTRANT              0

/* ================= Interrupt priorities ================= */

#ifdef __NVIC_PRIO_BITS
    #define configPRIO_BITS                     __NVIC_PRIO_BITS
#else
    #define configPRIO_BITS                     4
#endif

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY         15U
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY    5U

#define configKERNEL_INTERRUPT_PRIORITY \
    ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << ( 8U - configPRIO_BITS ) )

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << ( 8U - configPRIO_BITS ) )

/* Cortex-M exception handlers */

#define vPortSVCHandler                         SVC_Handler
#define xPortPendSVHandler                      PendSV_Handler

/* SysTick-et szándékosan NEM mapeljük itt. */

/* ================= API inclusion ================= */

#define INCLUDE_vTaskDelay                      1
#define INCLUDE_xTaskDelayUntil                 1
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_xTaskGetSchedulerState          1
#define INCLUDE_uxTaskPriorityGet               1
#define INCLUDE_vTaskPrioritySet                1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_uxTaskGetStackHighWaterMark2    1

/* ================= Assertions ================= */

#define configASSERT( x )                       \
    if( ( x ) == 0 )                            \
    {                                           \
        taskDISABLE_INTERRUPTS();               \
        for( ;; );                              \
    }

#endif 