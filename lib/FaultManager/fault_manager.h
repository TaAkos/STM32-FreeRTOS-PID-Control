#ifndef FAULT_MANAGER_H
#define FAULT_MANAGER_H

#include "stdbool.h"
#include "stdint.h"



typedef enum
{
    FAULT_NONE = 0,

    FAULT_CONTROL_DEADLINE,
    FAULT_PLANT_DEADLINE,
    FAULT_QUEUE_OVERFLOW,
    FAULT_COMMUNICATION_TIMEOUT,
    FAULT_INVALID_PARAMETER,
    FAULT_WATCHDOG,
    FAULT_NUMERICAL_ERROR,
    FAULT_STACK_LOW
} fault_code_t;


typedef enum
{
    FAULT_MODULE_NONE = 0,

    FAULT_MODULE_CONTROL,
    FAULT_MODULE_PLANT,
    FAULT_MODULE_COMMUNICATION,
    FAULT_MODULE_LOGGER,
    FAULT_MODULE_UART,
    FAULT_MODULE_SYSTEM_MANAGER,
    FAULT_MODULE_SAFETY
} fault_module_t;


typedef struct 
{
    fault_code_t code;
    fault_module_t module;

    uint32_t timestamp_ms;
    uint32_t data;
} fault_record_t;


bool FaultManager_Init(void);

bool FaultManager_Report(fault_code_t code, fault_module_t module, uint32_t data);

bool FaultManager_GetActive(fault_record_t *record);

bool FaultManager_GetLast(fault_record_t *record);

bool FaultManager_ClearActive(void);

bool FaultManager_GetTotalCount(uint32_t *count);

bool FaultManager_IsCritical(fault_code_t code);


#endif