#include "fault_manager.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <string.h>

static SemaphoreHandle_t fault_mutex = NULL;


static fault_record_t active_fault;
static fault_record_t last_fault;

static uint32_t total_fault_count = 0U;



bool FaultManager_Init(void){

    fault_mutex = xSemaphoreCreateMutex();

    if (fault_mutex == NULL)
    {
        return false;
    }
    
    memset(&active_fault, 0, sizeof(active_fault));
    memset(&last_fault, 0, sizeof(last_fault));

    active_fault.code = FAULT_NONE;
    last_fault.code = FAULT_NONE;

    total_fault_count = 0U;

    return true;

}

bool FaultManager_Report(fault_code_t code, fault_module_t module, uint32_t data){

    if (fault_mutex == NULL || code == FAULT_NONE || module == FAULT_MODULE_NONE)
    {
        return false;
    }

    fault_record_t record = {0};

    record.code = code;
    record.module = module;
    record.timestamp_ms = (uint32_t)xTaskGetTickCount() * (uint32_t)portTICK_PERIOD_MS;
    record.data = data;

    if (xSemaphoreTake(fault_mutex, portMAX_DELAY) != pdTRUE) 
    {
        return false;
    }

    last_fault = record;

    total_fault_count++;


    if (FaultManager_IsCritical(code) && active_fault.code == FAULT_NONE)
    {
        active_fault = record;
    }
    
    xSemaphoreGive(fault_mutex);

    return true;
    
}

bool FaultManager_GetActive(fault_record_t *record){

    if (record == NULL || fault_mutex == NULL)
    {
        return false;
    }

    if (xSemaphoreTake(fault_mutex, portMAX_DELAY) != pdTRUE)
    {           
        return false;
    }
    
    *record = active_fault;

    xSemaphoreGive(fault_mutex);

    return true;

}

bool FaultManager_GetLast(fault_record_t *record){

    if (record == NULL || fault_mutex == NULL)
    {
        return false;
    }

    if (xSemaphoreTake(fault_mutex, portMAX_DELAY) != pdTRUE)
    {           
        return false;
    }

    *record = last_fault;

    xSemaphoreGive(fault_mutex);

    return true;

}

bool FaultManager_ClearActive(void){

    if (fault_mutex == NULL)
    {
        return false;
    }

    if (xSemaphoreTake(fault_mutex, portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    memset(&active_fault, 0, sizeof(active_fault));

    xSemaphoreGive(fault_mutex);

    return true;
    
}

bool FaultManager_GetTotalCount(uint32_t *count){

    if (count == NULL || fault_mutex == NULL)
    {
        return false;
    }

    if (xSemaphoreTake(fault_mutex, portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    *count = total_fault_count;

    xSemaphoreGive(fault_mutex);

    return true;
    
}

bool FaultManager_IsCritical(fault_code_t code){

    switch (code)
    {
    case FAULT_CONTROL_DEADLINE:
    case FAULT_PLANT_DEADLINE:
    case FAULT_QUEUE_OVERFLOW:  
    case FAULT_WATCHDOG:
    case FAULT_NUMERICAL_ERROR:
    case FAULT_STACK_LOW:
        return true;


    case FAULT_NONE:  
    case FAULT_COMMUNICATION_TIMEOUT:
    case FAULT_INVALID_PARAMETER:
    default:
        return false;
    }

}

