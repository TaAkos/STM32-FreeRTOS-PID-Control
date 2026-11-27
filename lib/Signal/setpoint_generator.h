#ifndef SETPOINT_GENERATOR_H
#define SETPOINT_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>

typedef enum
{
SETPOINT_MODE_MANUAL,
SETPOINT_MODE_CONSTANT,
SETPOINT_MODE_STEP,
SETPOINT_MODE_RAMP,
SETPOINT_MODE_SQUARE,
SETPOINT_MODE_SINE
} setpoint_mode_t;


typedef struct 
{
    float value;
} constant_config_t;

typedef struct 
{
    float low;
    float high;
    uint32_t step_time_ms;
} step_config_t;

typedef struct 
{
    float start;
    float slope;
} ramp_config_t;

typedef struct 
{
    float low;
    float high;
    uint32_t period_ms;
} square_config_t;

typedef struct 
{
    float offset;
    float amplitude;
    float frequency;
} sine_config_t;

typedef union
{
    constant_config_t constant;
    step_config_t step;
    ramp_config_t ramp;
    square_config_t square;
    sine_config_t sine;
} setpoint_config_t;

typedef struct 
{
    setpoint_mode_t mode;
    setpoint_config_t config;
    uint32_t mode_start_time_ms;
} setpoint_generator_t;

bool SetpointGenerator_Init(setpoint_generator_t *generator);

bool SetpointGenerator_Update(const setpoint_generator_t *generator, uint32_t elapsed_time_ms, float manual_input, float *setpoint);

bool SetpointGenerator_ValidateConfig(setpoint_mode_t mode, const setpoint_config_t *config);

bool SetpointGenerator_Configure(setpoint_generator_t *generator, const setpoint_generator_t *new_config, uint32_t current_time_ms);

#endif