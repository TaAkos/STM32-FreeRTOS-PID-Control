#ifndef SAFETY_TASK_H
#define SAFETY_TASK_H


#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t control_stack_free_words;
    uint32_t plant_stack_free_words;
    uint32_t communication_stack_free_words;
    uint32_t system_manager_stack_free_words;
    uint32_t uart_tx_stack_free_words;
    uint32_t logger_stack_free_words;
    uint32_t safety_stack_free_words;

    uint32_t watchdog_refresh_count;

} safety_stats_t;


void SafetyMonitorTask(void *argument);

bool Safety_GetStats(safety_stats_t *stats);


#endif