#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <stdbool.h>

typedef enum
{
    SYSTEM_STATE_INIT,
    SYSTEM_STATE_IDLE,
    SYSTEM_STATE_RUNNING,
    SYSTEM_STATE_PAUSED,
    SYSTEM_STATE_FAULT
} system_state_t;

bool SystemState_Transition(system_state_t *state, system_state_t new_state);





#endif