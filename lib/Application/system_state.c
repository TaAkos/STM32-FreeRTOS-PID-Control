#include "system_state.h"

#include <stddef.h>


bool SystemState_Transition(system_state_t *state, system_state_t new_state){

    if (state == NULL){

        return false;
    }

    switch (*state)
    {
        case SYSTEM_STATE_INIT:
        
            if (new_state == SYSTEM_STATE_IDLE || new_state == SYSTEM_STATE_FAULT)
            {
                *state = new_state;
                return true;
            }
            return false;


        case SYSTEM_STATE_IDLE:
        
            if (new_state == SYSTEM_STATE_RUNNING || new_state == SYSTEM_STATE_IDLE || new_state == SYSTEM_STATE_FAULT)
            {
                *state = new_state;
                return true;
            }
            return false;
            

        case SYSTEM_STATE_RUNNING:
        
            if (new_state == SYSTEM_STATE_PAUSED || new_state == SYSTEM_STATE_IDLE || new_state == SYSTEM_STATE_RUNNING || new_state == SYSTEM_STATE_FAULT)
            {
                *state = new_state;
                return true;
            }
            return false;
            

        case SYSTEM_STATE_PAUSED:
            
            if (new_state == SYSTEM_STATE_RUNNING || new_state == SYSTEM_STATE_IDLE || new_state == SYSTEM_STATE_PAUSED || new_state == SYSTEM_STATE_FAULT)
            {
                *state = new_state;
                return true;
            }
            return false;

        case SYSTEM_STATE_FAULT:
            
            if (new_state == SYSTEM_STATE_IDLE || new_state == SYSTEM_STATE_FAULT)
            {
                *state = new_state;
                return true;
            }
            return false;
            
    
        default:
            return false;
    }

}