#include "control_task.h"


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "pid_controller.h"
#include "rtos_messages.h"
#include "event_groups.h"
#include "rtos_events.h"
#include "rtos_notifications.h"
#include "control_config.h"
#include "setpoint_generator.h"
#include "control_timing.h"
#include "fault_manager.h"


typedef enum
{
    CONTROL_FAULT_DETAIL_SETPOINT_INIT = 1U,
    CONTROL_FAULT_DETAIL_INITIAL_SETPOINT_CONFIG,
    CONTROL_FAULT_DETAIL_CONFIG_SNAPSHOT,
    CONTROL_FAULT_DETAIL_PID_INIT,
    CONTROL_FAULT_DETAIL_TIMING_INIT,
    CONTROL_FAULT_DETAIL_TIMING_SNAPSHOT,
    CONTROL_FAULT_DETAIL_TIMING_QUEUE,

    CONTROL_FAULT_DETAIL_RESET_SETPOINT_INIT,
    CONTROL_FAULT_DETAIL_RESET_SETPOINT_CONFIG,

    CONTROL_FAULT_DETAIL_SETPOINT_CONFIG,
    CONTROL_FAULT_DETAIL_SETPOINT_UPDATE,
    CONTROL_FAULT_DETAIL_PID_UPDATE,
    CONTROL_FAULT_DETAIL_CONTROL_QUEUE

} control_fault_detail_t;


extern QueueHandle_t control_queue;
extern QueueHandle_t measurement_queue;
extern QueueHandle_t setpoint_config_queue;
extern QueueHandle_t control_timing_queue;
extern QueueHandle_t telemetry_queue;

extern EventGroupHandle_t system_events;

static void Control_FatalStartupFault(fault_code_t code, uint32_t detail);


void ControlTask(void *argument){


    (void)argument;

    uint32_t simulation_time_ms = 0U;
    uint32_t telemetry_sequence = 0U;
    float setpoint = 0.0f;

    pid_controller_t pid;
    setpoint_generator_t setpoint_generator;

    if (!SetpointGenerator_Init(&setpoint_generator))
    {
        Control_FatalStartupFault(FAULT_NUMERICAL_ERROR, CONTROL_FAULT_DETAIL_SETPOINT_INIT);
    }

    setpoint_generator_t initial_setpoint = {0};

    initial_setpoint.mode = SETPOINT_MODE_CONSTANT;
    initial_setpoint.config.constant.value = 5.0f;

    if (!SetpointGenerator_Configure(&setpoint_generator, &initial_setpoint, 0U))
    {
        Control_FatalStartupFault(FAULT_NUMERICAL_ERROR, CONTROL_FAULT_DETAIL_INITIAL_SETPOINT_CONFIG);
    }


    

    measurement_msg_t measurement_msg = {0};
    control_msg_t control_msg = {0};

    control_config_t config;

    if (!ControlConfig_GetSnapshot(&config))
    {
        Control_FatalStartupFault(FAULT_NUMERICAL_ERROR, CONTROL_FAULT_DETAIL_CONFIG_SNAPSHOT);
    }
    

    float measurement = 0.0f;

    if (!PID_Init(
        &pid,
        config.Kp,
        config.Ti,
        0.01f,
        config.Tt,
        config.Td,
        config.Tf,
        config.output_min,
        config.output_max))
    {
        Control_FatalStartupFault(FAULT_NUMERICAL_ERROR, CONTROL_FAULT_DETAIL_PID_INIT);
    }


    TickType_t last_wake_time = xTaskGetTickCount();

    control_timing_t timing;

    if (!ControlTiming_Init(&timing, 10000U))
    {
        Control_FatalStartupFault(FAULT_CONTROL_DEADLINE, CONTROL_FAULT_DETAIL_TIMING_INIT);
    }

    control_timing_snapshot_t timing_snapshot = {0};

    if (!ControlTiming_GetSnapshot(&timing, &timing_snapshot))
    {
        Control_FatalStartupFault(FAULT_CONTROL_DEADLINE, CONTROL_FAULT_DETAIL_TIMING_SNAPSHOT);
    }
    
    if (xQueueOverwrite(control_timing_queue, &timing_snapshot) != pdPASS)
    {
        Control_FatalStartupFault(FAULT_QUEUE_OVERFLOW, CONTROL_FAULT_DETAIL_TIMING_QUEUE);
    }

    bool control_faulted = false;

    for (;;)
    {

        xEventGroupSetBits(system_events, SYSTEM_EVENT_HEARTBEAT_CONTROL);
        
        uint32_t notification_value = 0U;

        bool cycle_ok = true;

        if (xTaskNotifyWait(0U, UINT32_MAX, &notification_value, 0U) == pdPASS)
        {
            
            if ((notification_value & CONTROL_NOTIFY_RESET) != 0U)
            {
                
                bool reset_ok = true;

                PID_Reset(&pid);

                simulation_time_ms = 0U;
                measurement = 0.0f;
                setpoint = 0.0f;

                ControlTiming_Reset(&timing);

                if (ControlTiming_GetSnapshot(&timing, &timing_snapshot))
                {
                   if (xQueueOverwrite(control_timing_queue, &timing_snapshot) != pdPASS)
                   {
                        (void)FaultManager_Report(FAULT_QUEUE_OVERFLOW, FAULT_MODULE_CONTROL, CONTROL_FAULT_DETAIL_TIMING_QUEUE);

                        reset_ok = false;
                   }
                } else {

                    (void)FaultManager_Report(FAULT_CONTROL_DEADLINE, FAULT_MODULE_CONTROL, CONTROL_FAULT_DETAIL_TIMING_SNAPSHOT);

                    reset_ok = false;
                }
                

                if (reset_ok && !SetpointGenerator_Init(&setpoint_generator))
                {
                    (void)FaultManager_Report(FAULT_NUMERICAL_ERROR, FAULT_MODULE_CONTROL, CONTROL_FAULT_DETAIL_RESET_SETPOINT_INIT);
                
                    reset_ok = false;
                }
                
                setpoint_generator_t initial_setpoint = {0};

                initial_setpoint.mode = SETPOINT_MODE_CONSTANT;
                initial_setpoint.config.constant.value = 5.0f;

                if (reset_ok && !SetpointGenerator_Configure(&setpoint_generator, &initial_setpoint, 0U))
                {
                    (void)FaultManager_Report(FAULT_NUMERICAL_ERROR, FAULT_MODULE_CONTROL, CONTROL_FAULT_DETAIL_RESET_SETPOINT_CONFIG);

                    reset_ok = false;
                }

                signal_config_msg_t discarded_setpoint;

                (void)xQueueReceive(setpoint_config_queue, &discarded_setpoint, 0U); 

                if (reset_ok)
                {
                    control_msg.type = CONTROL_MSG_RESET;
                    control_msg.control = 0.0f;
                    control_msg.simulation_time_ms = 0U;
                
                    if (xQueueOverwrite(control_queue, &control_msg) != pdPASS)
                    {
                        (void)FaultManager_Report(FAULT_QUEUE_OVERFLOW, FAULT_MODULE_CONTROL, CONTROL_FAULT_DETAIL_CONTROL_QUEUE);

                        reset_ok = false;
                    }
                }
                
                if (reset_ok)
                {
                    control_faulted = false;

                    xEventGroupSetBits(system_events, SYSTEM_EVENT_CONTROL_RESET_DONE);
                } else {

                    control_faulted = true;
                }
                

            } else if ((notification_value & CONTROL_NOTIFY_SAFE_STOP) != 0U)
            {

                control_faulted = true;

                PID_Reset(&pid);

                control_msg.type = CONTROL_MSG_UPDATE;
                control_msg.control = 0.0f;
                control_msg.simulation_time_ms = simulation_time_ms;

                if(xQueueOverwrite(control_queue, &control_msg) != pdPASS){

                    (void)FaultManager_Report(FAULT_QUEUE_OVERFLOW, FAULT_MODULE_CONTROL, CONTROL_FAULT_DETAIL_CONTROL_QUEUE);
                }

                ControlTiming_PrepareResume(&timing);
            }
            
            
        }
        


        EventBits_t events = xEventGroupGetBits(system_events);

        if (((events & SYSTEM_EVENT_RUN_ALLOWED) != 0U) && !control_faulted)
        {
            
            uint32_t cycle_start = ControlTiming_NowCycles();

            ControlTiming_BeginCycle(&timing, cycle_start);
            
        
            signal_config_msg_t signal_msg;

            if (xQueueReceive(setpoint_config_queue, &signal_msg, 0U) == pdPASS)
            {
                setpoint_generator_t new_setpoint = {0};


                new_setpoint.mode = signal_msg.mode;
                new_setpoint.config = signal_msg.config;


                if (!SetpointGenerator_Configure(&setpoint_generator, &new_setpoint, simulation_time_ms))
                {
                    (void)FaultManager_Report(FAULT_INVALID_PARAMETER, FAULT_MODULE_CONTROL, CONTROL_FAULT_DETAIL_SETPOINT_CONFIG);
                }
            
            }
        
            if(!SetpointGenerator_Update(&setpoint_generator, simulation_time_ms, 0.0f, &setpoint))
            {

                (void)FaultManager_Report(FAULT_NUMERICAL_ERROR, FAULT_MODULE_CONTROL, CONTROL_FAULT_DETAIL_SETPOINT_UPDATE);

                control_faulted = true;
                cycle_ok = false;
            }

            control_config_t new_config;

            if (ControlConfig_GetSnapshot(&new_config))
            {
                config = new_config;

                pid.Kp = config.Kp;
                pid.Ti = config.Ti;
                pid.Td = config.Td;
                pid.Tf = config.Tf;
                pid.Tt = config.Tt;
                pid.output_min = config.output_min;
                pid.output_max = config.output_max;

            
            }

        
        
        
            if (xQueueReceive(measurement_queue, &measurement_msg, 0U) == pdPASS)
            {
                measurement = measurement_msg.measurement;
            }

            float error = setpoint - measurement;

            if (cycle_ok)
            {
                

                if (!PID_Update(&pid, error, measurement, &control_msg.control))
                {
                    (void)FaultManager_Report(FAULT_NUMERICAL_ERROR, FAULT_MODULE_CONTROL, CONTROL_FAULT_DETAIL_PID_UPDATE);

                    control_faulted = true;
                    cycle_ok = false;
                }
            }
            

            

            
            control_msg.type = CONTROL_MSG_UPDATE;
            control_msg.simulation_time_ms = simulation_time_ms;

            if (!cycle_ok)
            {
                PID_Reset(&pid);

                control_msg.type = CONTROL_MSG_UPDATE;
                control_msg.control = 0.0f;
                control_msg.simulation_time_ms = simulation_time_ms;

                if (xQueueOverwrite(control_queue, &control_msg) != pdPASS)
                {
                    (void)FaultManager_Report(FAULT_QUEUE_OVERFLOW, FAULT_MODULE_CONTROL, CONTROL_FAULT_DETAIL_CONTROL_QUEUE);
                }

                ControlTiming_PrepareResume(&timing);
                
            } else {
            

                if (xQueueOverwrite(control_queue, &control_msg) != pdPASS)
                {
                    (void)FaultManager_Report(FAULT_QUEUE_OVERFLOW, FAULT_MODULE_CONTROL, CONTROL_FAULT_DETAIL_CONTROL_QUEUE);

                    control_faulted = true;
                } else {

                    simulation_time_ms += 10U;

                    uint32_t cycle_end = ControlTiming_NowCycles();

                    ControlTiming_EndCycle(&timing, cycle_start, cycle_end);

                    if (ControlTiming_GetSnapshot(&timing, &timing_snapshot))
                    {
                        xQueueOverwrite(control_timing_queue, &timing_snapshot);
                    }
            
                    telemetry_msg_t telemetry_msg = {0};

                    telemetry_msg.sample_sequence = telemetry_sequence++;
                    telemetry_msg.time_ms = control_msg.simulation_time_ms;
                    telemetry_msg.setpoint = setpoint;
                    telemetry_msg.measurement = measurement;
                    telemetry_msg.control = control_msg.control;
                    telemetry_msg.error = error;

                    (void)xQueueSend(telemetry_queue, &telemetry_msg, 0U);
                    
                }
            }

        
        } else {

            ControlTiming_PrepareResume(&timing);
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(10U));
    
    }


}

static void Control_FatalStartupFault(fault_code_t code, uint32_t detail){
    
    (void)FaultManager_Report(code, FAULT_MODULE_CONTROL, detail);

    for (;;)
    {
        xEventGroupSetBits(
            system_events,
            SYSTEM_EVENT_HEARTBEAT_CONTROL);

        vTaskDelay(
            pdMS_TO_TICKS(50U));
    }
}