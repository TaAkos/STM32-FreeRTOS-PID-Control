#ifndef LOGGER_TASK_H
#define LOGGER_TASK_H


#include <stdbool.h>
#include <stdint.h>


typedef struct 
{
    uint32_t lost_sample_count;
    uint32_t encode_error_count;
    uint32_t tx_submit_fail_count;
} logger_stats_t;


void LoggerTask(void *argument);

bool Logger_GetStats(logger_stats_t *stats);



#endif