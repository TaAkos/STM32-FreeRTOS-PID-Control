#include "control_timing.h"

#include "stm32f4xx.h"

#include <stddef.h>
#include <string.h>


static uint32_t CyclesToUs(const control_timing_t *timing, uint32_t cycles)
{
    if (timing == NULL || timing->cycles_per_us == 0U)
    {
        return 0U;
    }

    return cycles / timing->cycles_per_us;
}


bool ControlTiming_Init(control_timing_t *timing, uint32_t nominal_period_us)
{
    if (timing == NULL || nominal_period_us == 0U)
    {
        return false;
    }

    memset(timing, 0, sizeof(*timing));

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    if ((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0U)
    {
        return false;
    }

    timing->cycles_per_us = SystemCoreClock / 1000000U;

    if (timing->cycles_per_us == 0U)
    {
        return false;
    }

    timing->nominal_period_cycles = timing->cycles_per_us * nominal_period_us;

    return true;
}


void ControlTiming_Reset(
    control_timing_t *timing)
{
    if (timing == NULL)
    {
        return;
    }

    uint32_t cycles_per_us = timing->cycles_per_us;

    uint32_t nominal_period_cycles = timing->nominal_period_cycles;

    memset(timing, 0, sizeof(*timing));

    timing->cycles_per_us = cycles_per_us;

    timing->nominal_period_cycles = nominal_period_cycles;
}


void ControlTiming_PrepareResume(control_timing_t *timing)
{
    if (timing == NULL)
    {
        return;
    }

    timing->previous_start_valid = false;
    timing->snapshot.last_jitter_us = 0U;
}


uint32_t ControlTiming_NowCycles(void)
{
    return DWT->CYCCNT;
}


void ControlTiming_BeginCycle(control_timing_t *timing, uint32_t start_cycles)
{
    if (timing == NULL)
    {
        return;
    }

    if (timing->previous_start_valid)
    {
        uint32_t actual_period_cycles = start_cycles - timing->previous_start_cycles;

        uint32_t jitter_cycles;

        if (actual_period_cycles >= timing->nominal_period_cycles)
        {
            jitter_cycles = actual_period_cycles - timing->nominal_period_cycles;
        }
        else
        {
            jitter_cycles = timing->nominal_period_cycles - actual_period_cycles;
        }

        timing->total_jitter_cycles += jitter_cycles;

        timing->jitter_sample_count++;

        timing->snapshot.last_jitter_us = CyclesToUs(timing, jitter_cycles);

        timing->snapshot.average_jitter_us = CyclesToUs(timing, (uint32_t)(timing->total_jitter_cycles / timing->jitter_sample_count));

        if (timing->snapshot.last_jitter_us > timing->snapshot.max_jitter_us)
        {
            timing->snapshot.max_jitter_us = timing->snapshot.last_jitter_us;
        }
    }
    else
    {
        timing->previous_start_valid = true;
        timing->snapshot.last_jitter_us = 0U;
    }

    timing->previous_start_cycles = start_cycles;
}


void ControlTiming_EndCycle(
    control_timing_t *timing,
    uint32_t start_cycles,
    uint32_t end_cycles)
{
    if (timing == NULL)
    {
        return;
    }

    uint32_t execution_cycles = end_cycles - start_cycles;

    timing->snapshot.cycle_count++;

    timing->total_execution_cycles += execution_cycles;

    timing->snapshot.last_execution_us = CyclesToUs(timing, execution_cycles);

    timing->snapshot.average_execution_us = CyclesToUs(timing, (uint32_t)(timing->total_execution_cycles / timing->snapshot.cycle_count));

    if (timing->snapshot.last_execution_us > timing->snapshot.max_execution_us)
    {
        timing->snapshot.max_execution_us = timing->snapshot.last_execution_us;
    }
}


bool ControlTiming_GetSnapshot(const control_timing_t *timing, control_timing_snapshot_t *snapshot)
{
    if (timing == NULL || snapshot == NULL)
    {
        return false;
    }

    *snapshot = timing->snapshot;

    return true;
}