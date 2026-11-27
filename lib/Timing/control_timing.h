#ifndef CONTROL_TIMING_H
#define CONTROL_TIMING_H

#include <stdint.h>
#include <stdbool.h>


typedef struct 
{
    uint32_t cycle_count;

    uint32_t last_execution_us;
    uint32_t average_execution_us;
    uint32_t max_execution_us;

    uint32_t last_jitter_us;
    uint32_t average_jitter_us;
    uint32_t max_jitter_us;

} control_timing_snapshot_t;

typedef struct 
{
    control_timing_snapshot_t snapshot;

    uint64_t total_execution_cycles;
    uint64_t total_jitter_cycles;

    uint32_t previous_start_cycles;
    uint32_t jitter_sample_count;

    uint32_t cycles_per_us;
    uint32_t nominal_period_cycles;

    bool previous_start_valid;
} control_timing_t;

bool ControlTiming_Init(control_timing_t *timing, uint32_t nominal_period_us);

void ControlTiming_Reset(control_timing_t *timing);

void ControlTiming_PrepareResume(control_timing_t *timing);

uint32_t ControlTiming_NowCycles(void);

void ControlTiming_BeginCycle(control_timing_t *timing, uint32_t start_cycles);

void ControlTiming_EndCycle(control_timing_t *timing, uint32_t start_cycles, uint32_t end_cycles);

bool ControlTiming_GetSnapshot(const control_timing_t *timing, control_timing_snapshot_t *snapshot);






#endif