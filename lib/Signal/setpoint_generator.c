#include "setpoint_generator.h"

#include <stddef.h>
#include <math.h>
#include <string.h>


bool SetpointGenerator_Init(setpoint_generator_t *generator){

    if (generator == NULL)
    {
        return false;
    }
    
    generator->mode = SETPOINT_MODE_MANUAL;

    memset(&generator->config, 0, sizeof(generator->config));

    generator->mode_start_time_ms = 0U;

    return true;

}

bool SetpointGenerator_Update(const setpoint_generator_t *generator, uint32_t elapsed_time_ms, float manual_input, float *setpoint){

    if (generator == NULL || setpoint == NULL)
    {
        return false;
    }

    uint32_t mode_elapsed_time_ms = elapsed_time_ms - generator->mode_start_time_ms;

    float result;

    switch (generator->mode)
    {
    case SETPOINT_MODE_MANUAL:
        result = manual_input;
        break;
    
    case SETPOINT_MODE_CONSTANT:
        result = generator->config.constant.value;
        break;

    case SETPOINT_MODE_STEP:
        if (mode_elapsed_time_ms < generator->config.step.step_time_ms)
        {
            result = generator->config.step.low;
        } else {
            result = generator->config.step.high;
        }
        break;
    
    case SETPOINT_MODE_RAMP:
        result = generator->config.ramp.start + generator->config.ramp.slope * ((float)mode_elapsed_time_ms / 1000.0f);
        break;

    case SETPOINT_MODE_SQUARE:
    {
        if (generator->config.square.period_ms == 0U)
        {
            return false;
        }
        
        uint32_t phase = mode_elapsed_time_ms % generator->config.square.period_ms;

        if (phase < (generator->config.square.period_ms / 2U))
        {
            result = generator->config.square.low;
        }else {
            result = generator->config.square.high;
        }
        break;
    }

    case SETPOINT_MODE_SINE:
        result = generator->config.sine.offset + generator->config.sine.amplitude * sinf(2.0f * 3.14159265f * generator->config.sine.frequency * ((float)mode_elapsed_time_ms / 1000.0f));
        break;

    default:
        return false;
    }

    if (!isfinite(result))
    {
        return false;
    }
    
    *setpoint = result;
    
    return true;

}

bool SetpointGenerator_ValidateConfig(setpoint_mode_t mode, const setpoint_config_t *config){

    if (config == NULL)
    {
        return false;
    }
    


     switch (mode)
    {
    case SETPOINT_MODE_MANUAL:
        return true;

    case SETPOINT_MODE_CONSTANT:
        return isfinite(config->constant.value);

    case SETPOINT_MODE_STEP:
        return isfinite(config->step.low) && isfinite(config->step.high);

    case SETPOINT_MODE_RAMP:
        return isfinite(config->ramp.start) && isfinite(config->ramp.slope);

    case SETPOINT_MODE_SQUARE:
        return isfinite(config->square.low) && isfinite(config->square.high) && (config->square.period_ms > 0U);

    case SETPOINT_MODE_SINE:
        return isfinite(config->sine.offset) && 
        isfinite(config->sine.amplitude) && 
        isfinite(config->sine.frequency) && 
        (config->sine.amplitude >= 0.0f) &&
         (config->sine.frequency >= 0.0f);

    default:
        return false;
    }


}

bool SetpointGenerator_Configure(setpoint_generator_t *generator, const setpoint_generator_t *new_config, uint32_t current_time_ms){

    if (generator == NULL || new_config == NULL)
    {
        return false;
    }
    
    if (!SetpointGenerator_ValidateConfig(new_config->mode, &new_config->config))
    {
        return false;
    }
    

    generator->mode = new_config->mode;
    generator->config = new_config->config;
    generator->mode_start_time_ms = current_time_ms;

    return true;
}


