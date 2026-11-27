#include "control_config.h"

#include "FreeRTOS.h"
#include "semphr.h"


#include <stddef.h>
#include <math.h>


static control_config_t control_config;

static SemaphoreHandle_t config_mutex = NULL;


bool ControlConfig_Init(void){

    config_mutex = xSemaphoreCreateMutex();

    if (config_mutex == NULL)
    {
        return false;
    }
    
    control_config.Kp = 2.0f;
    control_config.Ti = 2.0f;
    control_config.Td = 0.02f;
    control_config.Tf = 0.01f;
    control_config.Tt = 0.1f;

    control_config.output_min = 0.0f;
    control_config.output_max = 100.0f;


    return true;
}

bool ControlConfig_GetSnapshot(control_config_t *snapshot){

    if (snapshot == NULL || config_mutex == NULL)
    {
        return false;
    }
    
    if (xSemaphoreTake(config_mutex, 0U) != pdTRUE)
    {
        return false;
    }
    
    *snapshot = control_config;

    xSemaphoreGive(config_mutex);

    return true;
}

bool ControlConfig_SetPidParameter(control_config_paramater_t parameter, float value){

    if ((config_mutex == NULL) || !isfinite(value))
    {
        return false;
    }
    
    switch (parameter)
    {
        case CONTROL_CONFIG_KP:
            if ((value < 0.0f) || (value > 100.0f))
            {
                return false;
            }
            break;

        case CONTROL_CONFIG_TI:
            if (value <= 0.0f)
            {
                return false;
            }
            break;

        case CONTROL_CONFIG_TD:
            if (value < 0.0f)
            {
                return false;
            }
            break;

        case CONTROL_CONFIG_TF:
            if (value <= 0.0f)
            {
                return false;
            }
            break;

        case CONTROL_CONFIG_TT:
            if (value <= 0.0f)
            {
                return false;
            }
            break;

        default:
            return false;
    }

    if (xSemaphoreTake(config_mutex, portMAX_DELAY) != pdTRUE)
    {
        return false;
    }
    
     switch (parameter)
    {
        case CONTROL_CONFIG_KP:
            control_config.Kp = value;
            break;

        case CONTROL_CONFIG_TI:
            control_config.Ti = value;
            break;

        case CONTROL_CONFIG_TD:
            control_config.Td = value;
            break;

        case CONTROL_CONFIG_TF:
            control_config.Tf = value;
            break;

        case CONTROL_CONFIG_TT:
            control_config.Tt = value;
            break;

        default:
            xSemaphoreGive(config_mutex);
            return false;
    }

    xSemaphoreGive(config_mutex);

    return true;
}

bool ControlConfig_SetOutputLimits(float output_min, float output_max){

    if ((config_mutex == NULL) ||
        !isfinite(output_min) ||
        !isfinite(output_max))
    {
        return false;
    }

    if ((output_min < 0.0f) ||
        (output_min >= output_max) ||
        (output_max > 100.0f))
    {
        return false;
    }

    if (xSemaphoreTake(config_mutex, portMAX_DELAY) != pdTRUE)
    {
        return false;
    }

    
    control_config.output_min = output_min;
    control_config.output_max = output_max;

    xSemaphoreGive(config_mutex);

    return true;
}

