#include "system_manager_task.h"


#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "event_groups.h"

#include "rtos_messages.h"
#include "rtos_events.h"
#include "rtos_notifications.h"
#include "system_state.h"
#include "fault_manager.h"

extern QueueHandle_t system_command_queue;
extern QueueHandle_t system_state_queue;

extern EventGroupHandle_t system_events;

extern TaskHandle_t control_task_handle;

void SystemManagerTask(void *argument){

    (void)argument;

    system_state_t state = SYSTEM_STATE_INIT;
    system_request_msg_t request = {0};

    if (!SystemState_Transition(&state, SYSTEM_STATE_IDLE)){

        vTaskSuspend(NULL);
    }

    if (xQueueOverwrite(system_state_queue, &state) != pdPASS)
    {
        vTaskSuspend(NULL);
    }
    

    xEventGroupClearBits(system_events, SYSTEM_EVENT_RUN_ALLOWED | SYSTEM_EVENT_CONTROL_RESET_DONE | SYSTEM_EVENT_PLANT_RESET_DONE);

    for (;;)
    {

        xEventGroupSetBits(system_events, SYSTEM_EVENT_HEARTBEAT_SYSMGR);
        
        if (xQueueReceive(system_command_queue, &request, pdMS_TO_TICKS(50U)) != pdPASS)
        {

            continue;
        }

        system_result_t result;

            
            switch (request.request)
            {
            case SYSTEM_REQUEST_START:
                if (SystemState_Transition(&state, SYSTEM_STATE_RUNNING))
                {

                    if (xQueueOverwrite(system_state_queue, &state) != pdPASS)
                    {
                        result = SYSTEM_RESULT_INTERNAL_ERROR;
                        break;
                    }

                    xEventGroupSetBits(system_events, SYSTEM_EVENT_RUN_ALLOWED);
                    result = SYSTEM_RESULT_OK;
                } else{
                    result = SYSTEM_RESULT_INVALID_STATE;
                }
                break;

            case SYSTEM_REQUEST_PAUSE:
                
                xEventGroupClearBits(system_events, SYSTEM_EVENT_RUN_ALLOWED);
            
                if (SystemState_Transition(&state, SYSTEM_STATE_PAUSED))
                {
                    
                    if (xQueueOverwrite(system_state_queue, &state) != pdPASS)
                    {
                        result = SYSTEM_RESULT_INTERNAL_ERROR;
                        break;
                    }
                    
                    result = SYSTEM_RESULT_OK;
                } else{
                    result = SYSTEM_RESULT_INVALID_STATE;
                }
                break;

            case SYSTEM_REQUEST_RESET:

                xEventGroupClearBits(system_events, SYSTEM_EVENT_RUN_ALLOWED | SYSTEM_EVENT_CONTROL_RESET_DONE | SYSTEM_EVENT_PLANT_RESET_DONE);
                
                if (xTaskNotify(control_task_handle, CONTROL_NOTIFY_RESET, eSetBits) != pdPASS)
                {
                    (void)SystemState_Transition(&state, SYSTEM_STATE_FAULT);
                    (void)xQueueOverwrite(system_state_queue, &state);

                    result = SYSTEM_RESULT_INTERNAL_ERROR;
                    break;
                }

                const EventBits_t required_bits = SYSTEM_EVENT_CONTROL_RESET_DONE | SYSTEM_EVENT_PLANT_RESET_DONE;
                EventBits_t reset_bits = xEventGroupWaitBits(system_events, required_bits, pdTRUE, pdTRUE, pdMS_TO_TICKS(100U));
                
                if ((reset_bits &required_bits) != required_bits)
                {
                    (void)SystemState_Transition(&state, SYSTEM_STATE_FAULT);

                    (void)xQueueOverwrite(system_state_queue, &state);

                    result = SYSTEM_RESULT_INTERNAL_ERROR;
                    break;
                }

                if (!FaultManager_ClearActive())
                {
                    result = SYSTEM_RESULT_INTERNAL_ERROR;
                    break;
                }
                
                
                if (!SystemState_Transition(&state, SYSTEM_STATE_IDLE))
                {
                    result = SYSTEM_RESULT_INTERNAL_ERROR;
                    break;
                }
                
                if (xQueueOverwrite(system_state_queue, &state) != pdPASS)
                {
                    result = SYSTEM_RESULT_INTERNAL_ERROR;
                    break;
                }
                

                result = SYSTEM_RESULT_OK;
                break;
            
            case SYSTEM_REQUEST_FAULT:
            {
                xEventGroupClearBits(system_events, SYSTEM_EVENT_RUN_ALLOWED);

                if (control_task_handle != NULL)
                {   
                    (void)xTaskNotify(control_task_handle, CONTROL_NOTIFY_SAFE_STOP, eSetBits);
                }
                
                if (!SystemState_Transition(&state, SYSTEM_STATE_FAULT))
                {
                    result = SYSTEM_RESULT_INTERNAL_ERROR;
                    break;
                }

                if (xQueueOverwrite(system_state_queue, &state) != pdPASS)
                {
                    result = SYSTEM_RESULT_INTERNAL_ERROR;
                    break;
                }

                result = SYSTEM_RESULT_OK;

                break;
                
            }

            default:
                result = SYSTEM_RESULT_UNSUPPORTED;
                break;
            }

            if (request.requester != NULL)
            {
                (void)xTaskNotify(request.requester, (uint32_t)result, eSetValueWithoutOverwrite);
            }
    }

}