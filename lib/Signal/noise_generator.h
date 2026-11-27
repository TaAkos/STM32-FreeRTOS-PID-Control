#ifndef NOISE_GENERATOR_H
#define NOISE_GENERATOR_H

#include <stdint.h>
#include <stdbool.h>


typedef struct
{
    float amplitude;
    uint32_t state;
} noise_generator_t;

bool NoiseGenerator_Init(noise_generator_t *generator, uint32_t seed);

bool NoiseGenerator_SetAmplitude( noise_generator_t *generator, float amplitude);

bool NoiseGenerator_SetSeed(noise_generator_t *generator, uint32_t seed);

bool NoiseGenerator_Update(noise_generator_t *generator, float *noise);

bool NoiseGenerator_Set(noise_generator_t *generator, float amplitude, uint32_t seed);

bool NoiseGenerator_ValidateConfig(float amplitude, uint32_t seed);


#endif 