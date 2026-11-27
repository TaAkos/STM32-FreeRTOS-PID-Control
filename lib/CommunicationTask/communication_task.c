#include "communication_task.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

#include "semphr.h"

#include "uart_driver.h"
#include "protocol.h"
#include "protocol_parser.h"
#include "control_config.h"
#include "rtos_messages.h"
#include "noise_generator.h"
#include "system_state.h"
#include "control_timing.h"
#include "uart_tx_task.h"
#include "rtos_events.h"


#include <string.h>


extern QueueHandle_t setpoint_config_queue;
extern QueueHandle_t disturbance_config_queue;
extern QueueHandle_t noise_config_queue;
extern QueueHandle_t system_command_queue;
extern QueueHandle_t system_state_queue;
extern QueueHandle_t control_timing_queue;

extern EventGroupHandle_t system_events;

static SemaphoreHandle_t uart_rx_semaphore = NULL;

static protocol_status_t Communication_HandleFrame(const protocol_frame_t *frame, uint8_t *response_data, uint8_t *response_data_length);
static protocol_status_t HandleControlCommand(const protocol_frame_t *frame, control_command_t command);
static protocol_status_t HandleSetPidParameter(const protocol_frame_t *frame);
static protocol_status_t HandleSetOutputLimits(const protocol_frame_t *frame);
static bool Communication_SendResponse(const protocol_frame_t *request, protocol_status_t status, const uint8_t *data, uint8_t data_length);
static protocol_status_t HandleSetpointCommand(const protocol_frame_t *frame, signal_command_t command);
static protocol_status_t HandleDisturbanceCommand(const protocol_frame_t *frame, signal_command_t command);
static protocol_status_t HandleSignalCommand(const protocol_frame_t *frame, signal_command_t command, QueueHandle_t target_queue);
static protocol_status_t HandleNoiseCommand(const protocol_frame_t *frame, noise_command_t command);
static protocol_status_t SendSystemRequest(system_request_t request_type);
static protocol_status_t HandleSystemCommand(const protocol_frame_t *frame, system_command_t command, uint8_t *response_data, uint8_t *response_data_length);
static void Communication_RxDataFromISR(void);

void CommunicationTask(void *argument){

    (void)argument;

    uart_rx_semaphore = xSemaphoreCreateBinary();

    if (uart_rx_semaphore == NULL)
    {     
        vTaskSuspend(NULL);
    }

    UART2_RegisterRxDataCallback(Communication_RxDataFromISR);

    protocol_parser_t parser;
    protocol_frame_t frame;

    if (!ProtocolParser_Init(&parser))
    {
        vTaskSuspend(NULL);
    }

    for (;;)
    {

        xEventGroupSetBits(system_events, SYSTEM_EVENT_HEARTBEAT_COMM);


        uint8_t byte;

        while (UART2_ReadByte(&byte))
        {
            if (ProtocolParser_ProcessByte(&parser, byte, HAL_GetTick(), &frame))
            {

                uint8_t response_data[PROTOCOL_MAX_PAYLOAD - 1U] = {0};
                uint8_t response_data_length = 0U;

                protocol_status_t status = Communication_HandleFrame(&frame, response_data, &response_data_length);
               
                (void)Communication_SendResponse(&frame, status, response_data, response_data_length);
            }
            
        }
        
        (void)xSemaphoreTake(uart_rx_semaphore, pdMS_TO_TICKS(50U));
    }
    
}

static protocol_status_t Communication_HandleFrame(const protocol_frame_t *frame, uint8_t *response_data, uint8_t *response_data_length){

    if (frame == NULL || response_data == NULL || response_data_length == NULL)
    {
        return PROTOCOL_STATUS_INVALID_VALUE;
    }

    *response_data_length = 0U;

    if (frame->type != REQUEST)
    {
        return PROTOCOL_STATUS_INVALID_VALUE;
    }

    command_group_t group = (command_group_t)(frame->id & CMD_GROUP_MASK);

    uint8_t subcommand = frame->id & CMD_SUBCOMMAND_MASK;
    
    switch (group)
    {
    case CMD_GROUP_SYSTEM:
        return HandleSystemCommand(frame, (system_command_t)subcommand, response_data, response_data_length);

    case CMD_GROUP_CONTROL:
        return HandleControlCommand(frame, (control_command_t)subcommand);

    case CMD_GROUP_SETPOINT:
        return HandleSetpointCommand(frame, (signal_command_t)subcommand);

    case CMD_GROUP_DISTURBANCE:
        return HandleDisturbanceCommand(frame, (signal_command_t)subcommand);

    case CMD_GROUP_NOISE:
        return HandleNoiseCommand(frame, (noise_command_t)subcommand);
    
    default:
        return PROTOCOL_STATUS_UNKNOWN_COMMAND;
    }
    

}

static protocol_status_t HandleControlCommand(const protocol_frame_t *frame, control_command_t command){

    switch (command)
    {
    case CONTROL_CMD_SET_PID_PARAM:
        return HandleSetPidParameter(frame);
    
    case CONTROL_CMD_SET_OUTPUT_LIMIT:
        return HandleSetOutputLimits(frame);
    
    default:
        return PROTOCOL_STATUS_UNKNOWN_COMMAND;
    }
}

static protocol_status_t HandleSetPidParameter(const protocol_frame_t *frame){

    if (frame->length != 5U)
    {
        return PROTOCOL_STATUS_INVALID_LENGTH;
    }

    pid_parameter_id_t pid_parameter = (pid_parameter_id_t)frame->payload[0];

    float value;

    if (!Protocol_ReadFloatLE(&frame->payload[1], &value))
    {
        return PROTOCOL_STATUS_INVALID_VALUE;
    }

    control_config_paramater_t config_parameter;

    switch (pid_parameter)
    {
        case PID_PARAM_KP:
            config_parameter = CONTROL_CONFIG_KP;
            break;

        case PID_PARAM_TI:
            config_parameter = CONTROL_CONFIG_TI;
            break;

        case PID_PARAM_TD:
            config_parameter = CONTROL_CONFIG_TD;
            break;

        case PID_PARAM_TF:
            config_parameter = CONTROL_CONFIG_TF;
            break;

        case PID_PARAM_TT:
            config_parameter = CONTROL_CONFIG_TT;
            break;

        default:
            return PROTOCOL_STATUS_INVALID_VALUE;
    }
    
    if (!ControlConfig_SetPidParameter(config_parameter, value))
    {
        return PROTOCOL_STATUS_INVALID_VALUE;
    }
    
    return PROTOCOL_STATUS_OK;

}

static protocol_status_t HandleSetOutputLimits(const protocol_frame_t *frame){

    if (frame->length != 8U)
    {
        return PROTOCOL_STATUS_INVALID_LENGTH;
    }

    float output_min;
    float output_max;

    if (!Protocol_ReadFloatLE(&frame->payload[0], &output_min) || !Protocol_ReadFloatLE(&frame->payload[4], &output_max))
    {
        return PROTOCOL_STATUS_INVALID_VALUE;
    }

    if (!ControlConfig_SetOutputLimits(output_min, output_max))
    {
        return PROTOCOL_STATUS_INVALID_VALUE;
    }

    return PROTOCOL_STATUS_OK;
}

static bool Communication_SendResponse(const protocol_frame_t *request, protocol_status_t status, const uint8_t *data, uint8_t data_length){

    if (request == NULL)
    {
        return false;
    }

    if (data_length > (PROTOCOL_MAX_PAYLOAD - 1U))
    {
        return false;
    }

    if (data_length > 0U && data == NULL)
    {
        return false;
    }


    protocol_frame_t response = {0};

    response.version = PROTOCOL_VERSION;
    response.type = RESPONSE;
    response.id = request->id;
    response.sequence = request->sequence;
    response.payload[0] = (uint8_t)status;

    if (data_length > 0U)
    {
        memcpy(&response.payload[1], data, data_length);
    }
    

    response.length = (uint8_t)(1U + data_length);

    uint8_t tx_buffer[PROTOCOL_MAX_FRAME_SIZE];
    uint16_t encoded_length;

    if (!Protocol_EncodeFrame(&response, tx_buffer, sizeof(tx_buffer), &encoded_length))
    {
        return false;
    }
    
    return UartTx_SubmitResponse(tx_buffer, encoded_length);
}

static protocol_status_t HandleSetpointCommand(const protocol_frame_t *frame, signal_command_t command){

    return HandleSignalCommand(frame, command, setpoint_config_queue);

}
static protocol_status_t HandleDisturbanceCommand(const protocol_frame_t *frame, signal_command_t command){

    return HandleSignalCommand(frame, command, disturbance_config_queue);

}

static protocol_status_t HandleSignalCommand(const protocol_frame_t *frame, signal_command_t command, QueueHandle_t target_queue){

    if (frame == NULL || target_queue == NULL)
    {
        return PROTOCOL_STATUS_INVALID_VALUE;
    }

    signal_config_msg_t msg = {0};
    
    switch (command)
    {
    case SIGNAL_CMD_MANUAL:

        if (frame->length != 0U)
        {
            return PROTOCOL_STATUS_INVALID_LENGTH;
        }
        
        msg.mode = SETPOINT_MODE_MANUAL;
        break;
        
    case SIGNAL_CMD_CONSTANT:

        if (frame->length != 4U)
        {
            return PROTOCOL_STATUS_INVALID_LENGTH;
        }
        
        msg.mode = SETPOINT_MODE_CONSTANT;

        if (!Protocol_ReadFloatLE(&frame->payload[0], &msg.config.constant.value))
        {
            return PROTOCOL_STATUS_INVALID_VALUE;
        }
        
        break;

    case SIGNAL_CMD_STEP:

        if (frame->length != 12U)
        {
            return PROTOCOL_STATUS_INVALID_LENGTH;
        }
        
        msg.mode = SETPOINT_MODE_STEP;

        if (!Protocol_ReadFloatLE(&frame->payload[0], &msg.config.step.low) || !Protocol_ReadFloatLE(&frame->payload[4], &msg.config.step.high) || !Protocol_ReadU32LE(&frame->payload[8], &msg.config.step.step_time_ms))
        {
            return PROTOCOL_STATUS_INVALID_VALUE;
        }
        
        break;

    case SIGNAL_CMD_RAMP:

        if (frame->length != 8U)
        {
            return PROTOCOL_STATUS_INVALID_LENGTH;
        }
        
        msg.mode = SETPOINT_MODE_RAMP;

        if (!Protocol_ReadFloatLE(&frame->payload[0], &msg.config.ramp.start) || !Protocol_ReadFloatLE(&frame->payload[4], &msg.config.ramp.slope))
        {
            return PROTOCOL_STATUS_INVALID_VALUE;
        }
        
        break;  

    case SIGNAL_CMD_SQUARE:

        if (frame->length != 12U)
        {
            return PROTOCOL_STATUS_INVALID_LENGTH;
        }
        
        msg.mode = SETPOINT_MODE_SQUARE;

        if (!Protocol_ReadFloatLE(&frame->payload[0], &msg.config.square.low) || !Protocol_ReadFloatLE(&frame->payload[4], &msg.config.square.high) || !Protocol_ReadU32LE(&frame->payload[8], &msg.config.square.period_ms))
        {
            return PROTOCOL_STATUS_INVALID_VALUE;
        }
        
        break;  

    case SIGNAL_CMD_SINE:

        if (frame->length != 12U)
        {
            return PROTOCOL_STATUS_INVALID_LENGTH;
        }
        
        msg.mode = SETPOINT_MODE_SINE;

        if (!Protocol_ReadFloatLE(&frame->payload[0], &msg.config.sine.offset) || !Protocol_ReadFloatLE(&frame->payload[4], &msg.config.sine.amplitude) || !Protocol_ReadFloatLE(&frame->payload[8], &msg.config.sine.frequency))
        {
            return PROTOCOL_STATUS_INVALID_VALUE;
        }
        
        break;  
    
    default:
        return PROTOCOL_STATUS_UNKNOWN_COMMAND;
    }


    if (!SetpointGenerator_ValidateConfig(msg.mode, &msg.config))
    {
        return PROTOCOL_STATUS_INVALID_VALUE;
    }
    

    if (xQueueOverwrite(target_queue, &msg) != pdPASS)
    {
        return PROTOCOL_STATUS_INVALID_VALUE;
    }

    return PROTOCOL_STATUS_OK;

}

static protocol_status_t HandleNoiseCommand(const protocol_frame_t *frame, noise_command_t command){

    if (frame == NULL)
    {
        return PROTOCOL_STATUS_INVALID_VALUE;
    }

    noise_config_msg_t msg = {0};

    switch (command)
    {
    case NOISE_CMD_CONFIG:
        
        if (frame->length != 8U)
        {
            return PROTOCOL_STATUS_INVALID_LENGTH;
        }

        if (!Protocol_ReadFloatLE(&frame->payload[0], &msg.amplitude) || !Protocol_ReadU32LE(&frame->payload[4], &msg.seed))
        {
            return PROTOCOL_STATUS_INVALID_VALUE;
        }

        if (!NoiseGenerator_ValidateConfig(msg.amplitude, msg.seed))
        {
            return PROTOCOL_STATUS_INVALID_VALUE;
        }

        if (xQueueOverwrite(noise_config_queue, &msg) != pdPASS)
        {
            return PROTOCOL_STATUS_INVALID_VALUE;
        }
        
        return PROTOCOL_STATUS_OK;
        
        
    
    default:
        return PROTOCOL_STATUS_UNKNOWN_COMMAND;
    }
    
}

static protocol_status_t SendSystemRequest(system_request_t request_type){

    system_request_msg_t request = {0};

    request.request = request_type;
    request.requester = xTaskGetCurrentTaskHandle();

    xTaskNotifyStateClear(NULL);

    if (xQueueSend(system_command_queue, &request, 0U) != pdPASS)
    {
        return PROTOCOL_STATUS_INVALID_VALUE;
    }

    uint32_t notification_value = 0U;

    if (xTaskNotifyWait(0U, UINT32_MAX, &notification_value, pdMS_TO_TICKS(200U)) != pdPASS)
    {
        return PROTOCOL_STATUS_INVALID_VALUE;
    }
    
    system_result_t result = (system_result_t)notification_value;

    switch (result)
    {
    case SYSTEM_RESULT_OK:
        return PROTOCOL_STATUS_OK;

    case SYSTEM_RESULT_INVALID_STATE:
        return PROTOCOL_STATUS_INVALID_STATE;

    case SYSTEM_RESULT_INTERNAL_ERROR:
        return PROTOCOL_STATUS_INTERNAL_ERROR;
    
    default:
        return PROTOCOL_STATUS_INVALID_VALUE;
    }


}

static protocol_status_t HandleSystemCommand(const protocol_frame_t *frame, system_command_t command, uint8_t *response_data, uint8_t *response_data_length){

    if (frame == NULL || response_data == NULL || response_data_length == NULL)
    {
        return PROTOCOL_STATUS_INVALID_VALUE;
    }

    switch (command)
    {
    case SYSTEM_CMD_START:
        
        if (frame->length != 0U)
        {
            return PROTOCOL_STATUS_INVALID_LENGTH;
        }

        return SendSystemRequest(SYSTEM_REQUEST_START);

    case SYSTEM_CMD_PAUSE:
        
        if (frame->length != 0U)
        {
            return PROTOCOL_STATUS_INVALID_LENGTH;
        }

        return SendSystemRequest(SYSTEM_REQUEST_PAUSE);

    case SYSTEM_CMD_RESET:
        
        if (frame->length != 0U)
        {
            return PROTOCOL_STATUS_INVALID_LENGTH;
        }

        return SendSystemRequest(SYSTEM_REQUEST_RESET);

    case SYSTEM_CMD_STATUS:

        if (frame->length != 0U)
        {
            return PROTOCOL_STATUS_INVALID_LENGTH;
        }

        system_state_t state;

        if (xQueuePeek(system_state_queue, &state, 0U) != pdPASS)
        {
            return PROTOCOL_STATUS_INTERNAL_ERROR;
        }

        response_data[0] = (uint8_t)state;
        *response_data_length = 1U;

        return PROTOCOL_STATUS_OK;

    case SYSTEM_CMD_GET_TIMING:
    {

        if (frame->length != 0U)
        {
            return PROTOCOL_STATUS_INVALID_LENGTH;
        }

        control_timing_snapshot_t timing;

        if (xQueuePeek(control_timing_queue, &timing, 0U) != pdPASS)
        {
            return PROTOCOL_STATUS_INTERNAL_ERROR;
        }

        if (!Protocol_WriteU32LE(&response_data[0], timing.cycle_count) || 
                !Protocol_WriteU32LE(&response_data[4], timing.last_execution_us) || 
                !Protocol_WriteU32LE(&response_data[8], timing.average_execution_us) ||
                !Protocol_WriteU32LE(&response_data[12], timing.max_execution_us) || 
                !Protocol_WriteU32LE(&response_data[16], timing.last_jitter_us) || 
                !Protocol_WriteU32LE(&response_data[20], timing.average_jitter_us) || 
                !Protocol_WriteU32LE(&response_data[24], timing.max_jitter_us))
        {
            return PROTOCOL_STATUS_INTERNAL_ERROR;
        }
        
        *response_data_length = 28U;

        return PROTOCOL_STATUS_OK;
        
    }
          
    
    default:
        return PROTOCOL_STATUS_UNKNOWN_COMMAND;
    }
    
}

static void Communication_RxDataFromISR(void){

    if (uart_rx_semaphore == NULL)
    {
        return;
    }

    BaseType_t higher_priority_task_woken = pdFALSE;

    xSemaphoreGiveFromISR(uart_rx_semaphore, &higher_priority_task_woken);

    portYIELD_FROM_ISR(higher_priority_task_woken);

}