#include "plant_model.h"

#include <stddef.h>
#include <math.h>

void Plant_Init(plant_model_t *plant, float a, float b){

    if (plant == NULL){

        return;
    }

    plant->a = a;
    plant->b = b;
    plant->output = 0.0f;
    plant->disturbance = 0.0f;


}

bool Plant_Update(plant_model_t *plant, float control_input){

    if (plant == NULL){

        return false;
    }

    if (!isfinite(control_input) || !isfinite(plant->output) || !isfinite(plant->disturbance))
    {
        return false;
    }

    float new_output = (plant->a * plant->output) + (plant->b * control_input) + plant->disturbance;

    if (!isfinite(new_output))
    {
        return false;
    }

    plant->output = new_output;

    return true;

}

void Plant_Reset(plant_model_t *plant){

    if (plant == NULL){

        return;
    }

    plant->output = 0.0f;
    plant->disturbance = 0.0f;
}