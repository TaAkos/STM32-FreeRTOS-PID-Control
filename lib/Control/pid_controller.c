#include "pid_controller.h"



#include <stddef.h>
#include <stdbool.h>
#include <math.h>





bool PID_Init(pid_controller_t *controller, float Kp, float Ti, float Ts, float Tt, float Td, float Tf, float output_min, float output_max){

    if(controller == NULL){

        return false;
    }

    if (!isfinite(Kp) || !isfinite(Ti) || !isfinite(Ts) || !isfinite(Tt) || !isfinite(Td) || !isfinite(Tf) || !isfinite(output_min) || !isfinite(output_max))
    {
        return false;
    }   

    if(Kp < 0 || Ts <= 0.0f || Ti <= 0.0f || Tt <= 0.0f || Td < 0.0f || (Td > 0.0f && Tf <= 0.0f) || output_min >= output_max){

        return false;
    }

    

    controller->Kp = Kp;
    controller->Ti = Ti;
    controller->integrator = 0.0f;
    controller->Ts = Ts;
    controller->Tt = Tt;
    controller->Td = Td;
    controller->Tf = Tf;
    controller->derivative = 0.0f;
    controller->previous_measurement = 0.0f;
    controller->output_min = output_min;
    controller->output_max = output_max;
    controller->first_update = true;

    return true;
}

bool PID_Update(pid_controller_t *controller, float error, float measurement, float *output){


    if(controller == NULL || output == NULL){

        return false;
    }

    if (!isfinite(error) || !isfinite(measurement))
    {
        return false;
    }

    if (controller->first_update){

        controller->previous_measurement = measurement;
        controller->derivative = 0.0f;
        controller->first_update = false;
    } else if (controller->Td > 0.0f){

        float new_derivative = (controller->Tf / (controller->Tf + controller->Ts)) * controller->derivative - (controller->Kp * controller->Td) / (controller->Tf + controller->Ts) * (measurement - controller->previous_measurement);
        
        if (!isfinite(new_derivative))
        {
            return false;
        }

        controller->derivative = new_derivative;
        

        controller->previous_measurement = measurement;
    } else {

        controller->derivative = 0.0f;
        controller->previous_measurement = measurement;
    }

    float output_limited;

    float P = controller->Kp * error;

    float integrator_candidate =  controller->integrator + controller->Kp * (controller->Ts / controller->Ti) * error;

    if (!isfinite(integrator_candidate))
        {
            return false;
        }

    float output_unlimited = P + integrator_candidate + controller->derivative;

    if (!isfinite(output_unlimited))
        {
            return false;
        }

    
    if (output_unlimited > controller->output_max){
        
        output_limited = controller->output_max;

    } else if (output_unlimited < controller->output_min){
        
        output_limited = controller->output_min;
    } else {

        output_limited = output_unlimited;
    }
    
    float tracking_error = output_limited - output_unlimited;

    float new_integrator = integrator_candidate + (controller->Ts / controller->Tt) * tracking_error;

    if (!isfinite(new_integrator))
    {
        return false;
    }

    controller->integrator = new_integrator;


    *output = output_limited;

    return true;


}


void PID_Reset(pid_controller_t *controller){

    if(controller == NULL){

        return;
    }

    controller->integrator = 0.0f;
    controller->derivative = 0.0f;
    controller->previous_measurement = 0.0f;
    controller->first_update = true;

}

void PID_PrepareResume(pid_controller_t *controller){

    if (controller == NULL)
    {
        return;
    }

    controller->derivative = 0.0f;
    controller->first_update = true;

}