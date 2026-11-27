#include "safety_task.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

#include "fault_manager.h"
#include "rtos_events.h"
#include "rtos_messages.h"
#include "system_state.h"
#include "control_timing.h"
#include "watchdog_manager.h"



#define SAFETY_PERIOD_MS        100U
#define SAFETY_STARTUP_GRACE_MS 300U

#define HEARTBEAT_MISS_LIMIT   3U

#define CONTROL_DEADLINE_US     10000U

#define STACK_LOW_THRESHOLD_WORDS 32U


extern EventGroupHandle_t system_events;

extern QueueHandle_t system_command_queue;
extern QueueHandle_t system_state_queue;
extern QueueHandle_t control_timing_queue;

extern TaskHandle_t control_task_handle;
extern TaskHandle_t plant_task_handle;
extern TaskHandle_t communication_task_handle;
extern TaskHandle_t system_manager_task_handle;
extern TaskHandle_t uart_tx_task_handle;
extern TaskHandle_t logger_task_handle;
extern TaskHandle_t safety_task_handle;

static safety_stats_t safety_stats = {0};


static void Safety_RequestFaultState(void);
static uint32_t Safety_GetStackFreeWords(TaskHandle_t task_handle);
static bool Safety_CheckStack(uint32_t free_words, fault_module_t module);


static void Safety_RequestFaultState(void){

    system_state_t state;

    if (xQueuePeek(system_state_queue, &state, 0U) == pdPASS)
    {
        if (state == SYSTEM_STATE_FAULT)
        {
            return;
        }
        
    }

    xEventGroupClearBits(system_events, SYSTEM_EVENT_RUN_ALLOWED);

    system_request_msg_t request = {0};

    request.request = SYSTEM_REQUEST_FAULT;
    request.requester = NULL;

    (void)xQueueSend(system_command_queue, &request, 0U);
}

void SafetyMonitorTask(void *argument){

    (void)argument;

    vTaskDelay(pdMS_TO_TICKS(SAFETY_STARTUP_GRACE_MS));

    TickType_t last_wake_time = xTaskGetTickCount();

    
    uint32_t control_miss_count = 0U;
    uint32_t plant_miss_count = 0U;
    uint32_t comm_miss_count = 0U;
    uint32_t sysmgr_miss_count = 0U;

    for (;;)
    {
            
        EventBits_t heartbeat_bits = xEventGroupClearBits(system_events, SYSTEM_EVENT_HEARTBEAT_MASK);
        
        EventBits_t missing = SYSTEM_EVENT_HEARTBEAT_MASK & ~heartbeat_bits;

        if ((missing & SYSTEM_EVENT_HEARTBEAT_CONTROL) != 0U)
        {
            control_miss_count++;
        } else {
            control_miss_count = 0U;
        }


        if ((missing & SYSTEM_EVENT_HEARTBEAT_PLANT) != 0U)
        {
            plant_miss_count++;
        } else {
            plant_miss_count = 0U;
        }


        if ((missing & SYSTEM_EVENT_HEARTBEAT_COMM) != 0U)
        {
            comm_miss_count++;
        } else {
            comm_miss_count = 0U;
        }


        if ((missing & SYSTEM_EVENT_HEARTBEAT_SYSMGR) != 0U)
        {
            sysmgr_miss_count++;
        } else {
            sysmgr_miss_count = 0U;
        }


        if (control_miss_count == HEARTBEAT_MISS_LIMIT)
        {
            (void)FaultManager_Report(FAULT_CONTROL_DEADLINE, FAULT_MODULE_CONTROL, control_miss_count);
        }


        if (plant_miss_count == HEARTBEAT_MISS_LIMIT)
        {
            (void)FaultManager_Report(FAULT_PLANT_DEADLINE, FAULT_MODULE_PLANT, plant_miss_count);
        }


        if (comm_miss_count == HEARTBEAT_MISS_LIMIT)
        {
            (void)FaultManager_Report(FAULT_COMMUNICATION_TIMEOUT, FAULT_MODULE_COMMUNICATION, comm_miss_count);
        }


        if (sysmgr_miss_count == HEARTBEAT_MISS_LIMIT)
        {
            (void)FaultManager_Report(FAULT_WATCHDOG, FAULT_MODULE_SYSTEM_MANAGER, sysmgr_miss_count);
        }


        safety_stats_t new_stats = safety_stats;

        new_stats.control_stack_free_words = Safety_GetStackFreeWords(control_task_handle);

        new_stats.plant_stack_free_words = Safety_GetStackFreeWords(plant_task_handle);

        new_stats.communication_stack_free_words = Safety_GetStackFreeWords(communication_task_handle);

        new_stats.system_manager_stack_free_words = Safety_GetStackFreeWords(system_manager_task_handle);

        new_stats.uart_tx_stack_free_words = Safety_GetStackFreeWords(uart_tx_task_handle);

        new_stats.logger_stack_free_words = Safety_GetStackFreeWords(logger_task_handle);

        new_stats.safety_stack_free_words = Safety_GetStackFreeWords(safety_task_handle);


        fault_record_t active_fault = {0};

        bool active_fault_valid = FaultManager_GetActive(&active_fault);
    

        control_timing_snapshot_t timing;

        if (xQueuePeek(control_timing_queue, &timing, 0U) == pdPASS)
        {
            if (timing.max_execution_us > CONTROL_DEADLINE_US && active_fault_valid && active_fault.code == FAULT_NONE)
            {
                (void)FaultManager_Report(FAULT_CONTROL_DEADLINE, FAULT_MODULE_CONTROL, timing.max_execution_us);
            }
            
        }

        active_fault_valid = FaultManager_GetActive(&active_fault);
        

        if (active_fault_valid && active_fault.code == FAULT_NONE)
        {
            if (!Safety_CheckStack(new_stats.control_stack_free_words, FAULT_MODULE_CONTROL))
            {
        
            } else if (!Safety_CheckStack(new_stats.plant_stack_free_words, FAULT_MODULE_PLANT))
            {
            } else if (!Safety_CheckStack(new_stats.communication_stack_free_words, FAULT_MODULE_COMMUNICATION))
            {
            } else if (!Safety_CheckStack(new_stats.system_manager_stack_free_words, FAULT_MODULE_SYSTEM_MANAGER))
            {
            } else if (!Safety_CheckStack(new_stats.uart_tx_stack_free_words, FAULT_MODULE_UART))
            {
            } else if (!Safety_CheckStack(new_stats.logger_stack_free_words, FAULT_MODULE_LOGGER))
            {
            } else {
                (void)Safety_CheckStack(new_stats.safety_stack_free_words, FAULT_MODULE_SAFETY);
            }
        }

        taskENTER_CRITICAL();

        safety_stats.control_stack_free_words = new_stats.control_stack_free_words;

        safety_stats.plant_stack_free_words = new_stats.plant_stack_free_words;

        safety_stats.communication_stack_free_words = new_stats.communication_stack_free_words;

        safety_stats.system_manager_stack_free_words = new_stats.system_manager_stack_free_words;

        safety_stats.uart_tx_stack_free_words = new_stats.uart_tx_stack_free_words;

        safety_stats.logger_stack_free_words = new_stats.logger_stack_free_words;

        safety_stats.safety_stack_free_words = new_stats.safety_stack_free_words;

        taskEXIT_CRITICAL();

        
        bool critical_heartbeats_ok = (control_miss_count == 0U) && (plant_miss_count == 0U) && (sysmgr_miss_count == 0U);


        if (FaultManager_GetActive(&active_fault))
        {
            if (active_fault.code != FAULT_NONE)
            {
                Safety_RequestFaultState();

            } else if (critical_heartbeats_ok)
            {
                if (Watchdog_Refresh())
                {
                
                    taskENTER_CRITICAL();

                    safety_stats.watchdog_refresh_count++;

                    taskEXIT_CRITICAL();
                } else {
                    (void)FaultManager_Report(FAULT_WATCHDOG, FAULT_MODULE_SAFETY, 0U);

                    Safety_RequestFaultState();
                }
            }
        }


        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(SAFETY_PERIOD_MS));
        
    }
        
}
    

static uint32_t Safety_GetStackFreeWords(TaskHandle_t task_handle)
{
    if (task_handle == NULL)
    {
        return 0U;
    }

    return (uint32_t)uxTaskGetStackHighWaterMark(task_handle);
}

static bool Safety_CheckStack(uint32_t free_words, fault_module_t module){
    
    
    if (free_words >= STACK_LOW_THRESHOLD_WORDS)
    {
        return true;
    }

    (void)FaultManager_Report(FAULT_STACK_LOW, module, free_words);

    return false;
}

bool Safety_GetStats(safety_stats_t *stats){


    if (stats == NULL)
    {
        return false;
    }

    taskENTER_CRITICAL();

    *stats = safety_stats;

    taskEXIT_CRITICAL();

    return true;
}

