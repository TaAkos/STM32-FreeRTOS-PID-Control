#include "watchdog_manager.h"

#include "stm32f4xx_hal.h"


static IWDG_HandleTypeDef hiwdg;

static bool watchdog_initialized = false;
static bool watchdog_was_reset_cause = false;


bool Watchdog_Init(void)
{
    watchdog_was_reset_cause =
        (__HAL_RCC_GET_FLAG(
            RCC_FLAG_IWDGRST) != RESET);

    
    __HAL_RCC_CLEAR_RESET_FLAGS();


    hiwdg.Instance = IWDG;

    hiwdg.Init.Prescaler = IWDG_PRESCALER_64;


    hiwdg.Init.Reload = 999U;


    if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
    {
        return false;
    }


    watchdog_initialized = true;

    return true;
}


bool Watchdog_Refresh(void)
{
    if (!watchdog_initialized)
    {
        return false;
    }

    return
        HAL_IWDG_Refresh(&hiwdg) == HAL_OK;
}


bool Watchdog_WasResetCause(void)
{
    return watchdog_was_reset_cause;
}