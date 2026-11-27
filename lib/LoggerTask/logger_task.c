#include "logger_task.h"

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "rtos_messages.h"
#include "protocol.h"
#include "uart_tx_task.h"


#include <stdbool.h>
#include <string.h>


#define TELEMETRY_PERIOD_MS 100U


extern QueueHandle_t telemetry_queue;


static logger_stats_t logger_stats = {0};


void LoggerTask(void *argument){

    (void)argument;

    telemetry_msg_t msg = {0};

    uint32_t previous_sequence = 0U;
    bool previous_sequence_valid = false;

    uint32_t last_telemetry_time_ms = 0U;
    bool telemetry_time_valid = false;

    uint8_t protocol_sequence = 0U;

    memset(&logger_stats, 0, sizeof(logger_stats));

    for (;;)
    {
        
        if (xQueueReceive(telemetry_queue, &msg, portMAX_DELAY) != pdPASS)
        {
            continue;
        }

        if (previous_sequence_valid)
        {
            uint32_t expected_sequence = previous_sequence + 1U;
            uint32_t lost = msg.sample_sequence - expected_sequence;

            if (lost > 0U)
            {
                logger_stats.lost_sample_count += lost;
            }
            
        } else {

            previous_sequence_valid = true;
        }

        previous_sequence = msg.sample_sequence;

        if (telemetry_time_valid && msg.time_ms < last_telemetry_time_ms)
        {
            last_telemetry_time_ms = msg.time_ms;

            continue;
        }

        if (!telemetry_time_valid)
        {
            last_telemetry_time_ms = msg.time_ms;
            telemetry_time_valid = true;

            continue;
        }

        if ((msg.time_ms - last_telemetry_time_ms) < TELEMETRY_PERIOD_MS)
        {
            continue;
        }

        last_telemetry_time_ms = msg.time_ms;

        
        protocol_frame_t frame = {0};

        frame.version = PROTOCOL_VERSION;
        frame.type = TELEMETRY;
        frame.id = TELEMETRY_CONTROL;
        frame.sequence = protocol_sequence++;
        frame.length = 21U;

        if (!Protocol_WriteU32LE(&frame.payload[0], msg.time_ms) ||
                !Protocol_WriteFloatLE(&frame.payload[4], msg.setpoint) ||
                !Protocol_WriteFloatLE(&frame.payload[8], msg.measurement) ||
                !Protocol_WriteFloatLE(&frame.payload[12], msg.control) ||
                !Protocol_WriteFloatLE(&frame.payload[16], msg.error))
        {
            logger_stats.encode_error_count++;
            continue;
        }
        
        frame.payload[20] = (uint8_t)PROTOCOLSYSTEM_STATE_RUNNING;

        uint8_t tx_buffer[PROTOCOL_MAX_FRAME_SIZE];
        uint16_t encoded_length = 0U;

        if (!Protocol_EncodeFrame(&frame, tx_buffer, sizeof(tx_buffer), &encoded_length))
        {
            logger_stats.encode_error_count++;
            continue;
        }

        if (!UartTx_SubmitTelemetry(tx_buffer, encoded_length))
        {
            logger_stats.tx_submit_fail_count++;
        }
        
    }

}

bool Logger_GetStats(logger_stats_t *stats){

    if (stats == NULL)
    {
        return false;
    }

    taskENTER_CRITICAL();

    *stats = logger_stats;

    taskEXIT_CRITICAL();

    return true;
    
}