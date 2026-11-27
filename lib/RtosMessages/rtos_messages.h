#ifndef RTOS_MESSAGES_H
#define RTOS_MESSAGES_H


#include "FreeRTOS.h"
#include "task.h"

#include "setpoint_generator.h"


#include <stdint.h>


#define INCLUDE_uxTaskGetStackHighWaterMark 1


typedef enum
{
    CONTROL_MSG_UPDATE,
    CONTROL_MSG_RESET
} control_msg_type_t;

typedef struct 
{
    control_msg_type_t type;
    float control;
    uint32_t simulation_time_ms;
} control_msg_t;


typedef struct 
{
    float measurement;
} measurement_msg_t;

typedef struct 
{
    uint32_t sample_sequence;
    uint32_t time_ms;

    float setpoint;
    float measurement;
    float control;
    float error;
} telemetry_msg_t;


typedef struct 
{
    setpoint_mode_t mode;
    setpoint_config_t config;
} signal_config_msg_t;

typedef struct 
{
    float amplitude;
    uint32_t seed;
} noise_config_msg_t;

typedef enum
{
    SYSTEM_REQUEST_START,
    SYSTEM_REQUEST_PAUSE,
    SYSTEM_REQUEST_RESET,
    SYSTEM_REQUEST_FAULT
} system_request_t;

typedef struct 
{
    system_request_t request;
    TaskHandle_t requester;
} system_request_msg_t;

typedef enum
{
    SYSTEM_RESULT_OK = 1U,
    SYSTEM_RESULT_INVALID_STATE,
    SYSTEM_RESULT_UNSUPPORTED,
    SYSTEM_RESULT_INTERNAL_ERROR
} system_result_t;






#endif