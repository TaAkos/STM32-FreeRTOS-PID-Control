#include "plant_task.h"


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

#include "plant_model.h"
#include "noise_generator.h"
#include "rtos_messages.h"
#include "rtos_events.h"
#include "fault_manager.h"


typedef enum
{
    PLANT_FAULT_DETAIL_DISTURBANCE_INIT = 1U,
    PLANT_FAULT_DETAIL_NOISE_INIT,

    PLANT_FAULT_DETAIL_RESET_DISTURBANCE_INIT,
    PLANT_FAULT_DETAIL_RESET_NOISE_INIT,

    PLANT_FAULT_DETAIL_DISTURBANCE_CONFIG,
    PLANT_FAULT_DETAIL_NOISE_CONFIG,

    PLANT_FAULT_DETAIL_DISTURBANCE_UPDATE,
    PLANT_FAULT_DETAIL_PLANT_UPDATE,
    PLANT_FAULT_DETAIL_NOISE_UPDATE,

    PLANT_FAULT_DETAIL_MEASUREMENT_QUEUE

} plant_fault_detail_t;


extern QueueHandle_t control_queue;
extern QueueHandle_t measurement_queue;
extern QueueHandle_t disturbance_config_queue;
extern QueueHandle_t noise_config_queue;

extern EventGroupHandle_t system_events;

static void Plant_FatalStartupFault(uint32_t detail);


void PlantTask(void *argument){


    (void)argument;

    plant_model_t plant;
    setpoint_generator_t disturbance_generator;
    noise_generator_t noise_generator;

    if (!SetpointGenerator_Init(&disturbance_generator))
    {
        Plant_FatalStartupFault(PLANT_FAULT_DETAIL_DISTURBANCE_INIT);
    }

    if (!NoiseGenerator_Init(&noise_generator, 12345U))
    {
        Plant_FatalStartupFault(PLANT_FAULT_DETAIL_NOISE_INIT);
    }
    
    bool plant_faulted = false;


    control_msg_t control_msg = {0};
    measurement_msg_t measurement_msg = {0};
    signal_config_msg_t disturbance_msg = {0};
    noise_config_msg_t noise_msg = {0};
    float disturbance = 0.0f;
    float measurement_noise = 0.0f;

    Plant_Init(&plant, 0.99005f, 0.00995f);

    for (;;)
    {
        
        xEventGroupSetBits(system_events, SYSTEM_EVENT_HEARTBEAT_PLANT);

        if (xQueueReceive(control_queue, &control_msg, pdMS_TO_TICKS(50U)) != pdPASS){

            continue;
        }


        if (control_msg.type == CONTROL_MSG_RESET)
            {
                
                bool reset_ok = true;

                Plant_Reset(&plant);

                if (!SetpointGenerator_Init(&disturbance_generator))
                {
                    (void)FaultManager_Report(FAULT_NUMERICAL_ERROR, FAULT_MODULE_PLANT, PLANT_FAULT_DETAIL_RESET_DISTURBANCE_INIT);

                    reset_ok = false;
                
                }
            
                if (reset_ok && !NoiseGenerator_Init(&noise_generator, 12345U))
                {
                    (void)FaultManager_Report(FAULT_NUMERICAL_ERROR, FAULT_MODULE_PLANT, PLANT_FAULT_DETAIL_RESET_NOISE_INIT);

                    reset_ok = false;
                }
                
                disturbance = 0.0f;
                measurement_noise = 0.0f;
                plant.disturbance = 0.0f;

                signal_config_msg_t discarded_disturbance;
                noise_config_msg_t discarded_noise;

                (void)xQueueReceive(disturbance_config_queue, &discarded_disturbance, 0U);
                (void)xQueueReceive(noise_config_queue, &discarded_noise, 0U);
                
                measurement_msg.measurement = 0.0f;

                if (reset_ok && xQueueOverwrite(measurement_queue, &measurement_msg) != pdPASS){

                    (void)FaultManager_Report(FAULT_QUEUE_OVERFLOW, FAULT_MODULE_PLANT, PLANT_FAULT_DETAIL_MEASUREMENT_QUEUE);

                    reset_ok = false;
                }

                if (reset_ok)
                {
                    plant_faulted = false;

                    xEventGroupSetBits(system_events, SYSTEM_EVENT_PLANT_RESET_DONE);
                } else {


                    plant_faulted = true;
                }

                continue;

            }

            if (plant_faulted)
            {
                continue;
            }
            


            if (xQueueReceive(disturbance_config_queue, &disturbance_msg, 0U) == pdPASS)
            {
                setpoint_generator_t new_disturbance = {0};

                new_disturbance.mode = disturbance_msg.mode;
                new_disturbance.config = disturbance_msg.config;

                if (!SetpointGenerator_Configure(&disturbance_generator, &new_disturbance, control_msg.simulation_time_ms))
                {
                    (void)FaultManager_Report(FAULT_INVALID_PARAMETER, FAULT_MODULE_PLANT, PLANT_FAULT_DETAIL_DISTURBANCE_CONFIG);
                }
                

            }

            if (xQueueReceive(noise_config_queue, &noise_msg, 0U) == pdPASS)
            {
                if (!NoiseGenerator_Set(&noise_generator, noise_msg.amplitude, noise_msg.seed))
                {
                    (void)FaultManager_Report(FAULT_INVALID_PARAMETER, FAULT_MODULE_PLANT, PLANT_FAULT_DETAIL_NOISE_CONFIG);
                }
                
            }
            
            bool update_ok = true;
            

            if (!SetpointGenerator_Update(&disturbance_generator, control_msg.simulation_time_ms, 0.0f, &disturbance))
            {
                (void)FaultManager_Report(FAULT_NUMERICAL_ERROR, FAULT_MODULE_PLANT, PLANT_FAULT_DETAIL_DISTURBANCE_UPDATE);
            
                plant_faulted = true;
                update_ok = false;
            }

            if (update_ok)
            {
                plant.disturbance = disturbance;

                if (!Plant_Update(&plant, control_msg.control))
                {
                    (void)FaultManager_Report(FAULT_NUMERICAL_ERROR, FAULT_MODULE_PLANT, PLANT_FAULT_DETAIL_PLANT_UPDATE);

                    plant_faulted = true;
                    update_ok = false;
                }
            }
            
            
            
            
            if (update_ok && !NoiseGenerator_Update(&noise_generator, &measurement_noise))
            {
                (void)FaultManager_Report(FAULT_NUMERICAL_ERROR, FAULT_MODULE_PLANT, PLANT_FAULT_DETAIL_NOISE_UPDATE);

                plant_faulted = true;
                update_ok = false;
            }

            if (update_ok)
            {
                measurement_msg.measurement = plant.output + measurement_noise;

                if (xQueueOverwrite(measurement_queue, &measurement_msg) != pdPASS)
                {
                    (void)FaultManager_Report(FAULT_QUEUE_OVERFLOW, FAULT_MODULE_PLANT, PLANT_FAULT_DETAIL_MEASUREMENT_QUEUE);

                    plant_faulted = true;
                }
            }
            

            

        

            
    }

}

static void Plant_FatalStartupFault(uint32_t detail){
    (void)FaultManager_Report(FAULT_NUMERICAL_ERROR, FAULT_MODULE_PLANT, detail);

    for (;;)
    {
        xEventGroupSetBits(system_events, SYSTEM_EVENT_HEARTBEAT_PLANT);

        vTaskDelay(pdMS_TO_TICKS(50U));
    }
}