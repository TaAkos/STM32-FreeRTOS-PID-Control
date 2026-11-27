#include "stm32f4xx_hal.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

#include "control_task.h"
#include "plant_task.h"
#include "communication_task.h"
#include "system_manager_task.h"
#include "control_config.h"
#include "control_timing.h"
#include "rtos_messages.h"
#include "rtos_events.h"
#include "uart_driver.h"
#include "stm.h"
#include "system_state.h"
#include "uart_tx_task.h"
#include "logger_task.h"
#include "fault_manager.h"
#include "safety_task.h"
#include "watchdog_manager.h"


QueueHandle_t control_queue;
QueueHandle_t measurement_queue;
QueueHandle_t setpoint_config_queue;
QueueHandle_t disturbance_config_queue;
QueueHandle_t noise_config_queue;
QueueHandle_t system_command_queue;
QueueHandle_t system_state_queue;
QueueHandle_t control_timing_queue;
QueueHandle_t telemetry_queue;

EventGroupHandle_t system_events;

TaskHandle_t control_task_handle = NULL;
TaskHandle_t plant_task_handle = NULL;
TaskHandle_t communication_task_handle = NULL;
TaskHandle_t system_manager_task_handle = NULL;
TaskHandle_t uart_tx_task_handle = NULL;
TaskHandle_t logger_task_handle = NULL;
TaskHandle_t safety_task_handle = NULL;


int main(void){

    HAL_Init();
    SystemClock_Config();

    if (!UART2_Init())
    {
        while (1)
        {
            
        }
        
    }
    
    if (!UART2_StartReceive())
    {
        while (1)
        {
            
        }
        
    }

    if (!ControlConfig_Init())
    {
        while (1)
        {
        }
    }

    if (!FaultManager_Init())
    {
        while (1)
        {
            
        }
        
    }
    

    
    
    

    control_queue = xQueueCreate(1U, sizeof(control_msg_t));
    measurement_queue = xQueueCreate(1U, sizeof(measurement_msg_t));
    setpoint_config_queue = xQueueCreate(1U, sizeof(signal_config_msg_t));
    disturbance_config_queue = xQueueCreate(1U, sizeof(signal_config_msg_t));
    noise_config_queue = xQueueCreate(1U, sizeof(noise_config_msg_t));
    system_command_queue = xQueueCreate(2U, sizeof(system_request_msg_t));
    system_state_queue = xQueueCreate(1U, sizeof(system_state_t));
    control_timing_queue = xQueueCreate(1U, sizeof(control_timing_snapshot_t));
    telemetry_queue = xQueueCreate(16U, sizeof(telemetry_msg_t));

    system_events = xEventGroupCreate();

    if (control_queue == NULL || measurement_queue == NULL || setpoint_config_queue == NULL || disturbance_config_queue == NULL || noise_config_queue == NULL || system_command_queue == NULL || system_events == NULL || system_state_queue == NULL || control_timing_queue == NULL || telemetry_queue == NULL)
    {
        while (1)
        {
            
        }
        
    }

    if (!UartTx_Init())
    {
        while (1)
        {
            
        }
        
    }

    if (xTaskCreate(SafetyMonitorTask, "Safety", 256U, NULL, 6U, &safety_task_handle) != pdPASS)
    {
        while (1)
        {
            
        }
        
    }
    
    

    if (xTaskCreate(CommunicationTask, "Comm", 384U, NULL, 3U, &communication_task_handle) != pdPASS)
    {
        while (1)
        {
            
        }
        
    }
    
    if (xTaskCreate(ControlTask, "Control", 256U, NULL, 5U, &control_task_handle) !=pdPASS)
    {
        while (1)
        {
            
        }
        
    }

    if (xTaskCreate(PlantTask, "Plant", 256U, NULL, 4U, &plant_task_handle) != pdPASS)
    {
        while (1)
        {
            
        }
        
    }

    if (xTaskCreate(SystemManagerTask, "SysMgr", 256U, NULL, 3U, &system_manager_task_handle) != pdPASS)
    {
        while (1)
        {
            
        }
        
    }

    if (xTaskCreate(UartTxTask, "UartTx", 256U, NULL, 2U, &uart_tx_task_handle) != pdPASS)
    {
        while (1)
        {
            
        }
        
    }

    if (xTaskCreate(LoggerTask, "Logger", 256U, NULL, 1U, &logger_task_handle) != pdPASS)
    {
        while (1)
        {
            
        }
        
    }
    
    if (!Watchdog_Init())
    {
        while (1)
        {
            
        }
        
    }
    
    

    vTaskStartScheduler();

    while (1)
    {
        
    }
    
    
    





}