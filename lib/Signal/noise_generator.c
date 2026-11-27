#include "noise_generator.h"

#include <stddef.h>
#include <math.h>


bool NoiseGenerator_Init(noise_generator_t *generator, uint32_t seed){

    if (generator == NULL || seed == 0U)
    {
        return false;
    }
    
    generator->amplitude = 0.0f;
    generator->state = seed;

    return true;

}

bool NoiseGenerator_SetAmplitude( noise_generator_t *generator, float amplitude){

    if (generator == NULL)
    {
        return false;
    }

    if (!isfinite(amplitude) || amplitude < 0.0f)
    {
        return false;
    }

    generator->amplitude = amplitude;

    return true;

}

bool NoiseGenerator_SetSeed(noise_generator_t *generator, uint32_t seed){

    if (generator == NULL)
    {
        return false;
    }

    if (seed == 0U)
    {
        return false;
    }
    
    generator->state = seed;
    return true;

}

bool NoiseGenerator_Set(noise_generator_t *generator, float amplitude, uint32_t seed){

    if (generator == NULL)
    {
        return false;
    }

    if(!NoiseGenerator_ValidateConfig(amplitude, seed)){
        return false;
    }
    
    generator->amplitude = amplitude;
    generator->state = seed;
    return true;

}

bool NoiseGenerator_ValidateConfig(float amplitude, uint32_t seed){

    
    return amplitude >= 0 && isfinite(amplitude) && seed !=0;
}

bool NoiseGenerator_Update(noise_generator_t *generator, float *noise){

    if (generator == NULL || noise == NULL)
    {
        return false;
    }
    
    uint32_t x = generator->state;

    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;

    generator->state = x;

    float normalized = (float)x / (float)UINT32_MAX;
    float centered = 2.0f * normalized - 1.0f;

    float result = centered * generator->amplitude;

    if (!isfinite(result))
    {
        return false;
    }

    *noise = result;

    return true;
    

}