#ifndef CONTROL_CONFIG_H
#define CONTROL_CONFIG_H

#include <stdbool.h>


typedef enum
{
CONTROL_CONFIG_KP,
CONTROL_CONFIG_TI,
CONTROL_CONFIG_TD,
CONTROL_CONFIG_TF,
CONTROL_CONFIG_TT
}control_config_paramater_t;

typedef struct 
{
    float Kp;
    float Ti;
    float Td;
    float Tf;
    float Tt;

    float output_min;
    float output_max;

} control_config_t;

bool ControlConfig_Init(void);
bool ControlConfig_GetSnapshot(control_config_t *snapshot);
bool ControlConfig_SetPidParameter(control_config_paramater_t parameter, float value);
bool ControlConfig_SetOutputLimits(float output_min, float output_max);










#endif