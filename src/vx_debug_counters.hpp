#ifndef VX_DEBUG_COUNTERS_H
#define VX_DEBUG_COUNTERS_H

#include <stdio.h>
#include <x86intrin.h>
#include "um.hpp"

enum
{
    DebugCycleCount_MainUpdate,
    DebugCycleCount_MainRender,
    DebugCycleCount_RenderChunks,
    DebugCycleCount_RenderText,
    DebugCycleCount_Count
};

extern u64 g_debugCounters[DebugCycleCount_Count];

#define BEGIN_TIMED_BLOCK(id) u64 debugCounter##id = __rdtsc()
#define END_TIMED_BLOCK(id) g_debugCounters[id] = __rdtsc() - debugCounter##id

inline void
debug_counters_dump()
{
    printf("DEBUG COUNTERS\n");
    for (u32 i = 0; i < DebugCycleCount_Count; i++)
    {
        printf("%u: %lu\n", i, g_debugCounters[i]);
    }
}

#endif // VX_DEBUG_COUNTERS_H
