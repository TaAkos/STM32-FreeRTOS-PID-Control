#ifndef PLANT_MODEL_H
#define PLANT_MODEL_H

#include <stdbool.h>

typedef struct {
    float a;
    float b;
    float output;
    float disturbance;
} plant_model_t;


void Plant_Init(plant_model_t *plant, float a, float b);

bool Plant_Update(plant_model_t *plant, float control_input);

void Plant_Reset(plant_model_t *plant);








#endif