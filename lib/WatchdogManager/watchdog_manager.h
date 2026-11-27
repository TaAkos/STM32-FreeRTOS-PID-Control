#ifndef WATCHDOG_MANAGER_H
#define WATCHDOG_MANAGER_H

#include <stdbool.h>


bool Watchdog_Init(void);

bool Watchdog_Refresh(void);

bool Watchdog_WasResetCause(void);


#endif