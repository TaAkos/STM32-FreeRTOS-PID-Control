#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <stdbool.h>



typedef struct 
{
    float Kp;       
    float Ti;   
    float integrator;
    float Ts;
    float Tt;
    float Td;
    float Tf;
    float derivative;
    float previous_measurement;         
    float output_min;
    float output_max; 
    bool first_update;
} pid_controller_t;

bool PID_Init(pid_controller_t *controller, float Kp, float Ti, float Ts, float Tt, float Td, float Tf, float output_min, float output_max);

bool PID_Update(pid_controller_t *controller, float error, float measurement, float *output);

void PID_Reset(pid_controller_t *controller);

void PID_PrepareResume(pid_controller_t *controller);


#endif