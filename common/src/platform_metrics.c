#include "base.h"
#include "base_types.h"
#include "base_memory.h"
#include "platform_metrics.h"

#if _WIN32

#include <intrin.h>
#include <windows.h>
#include <stdio.h>

global_variable timings_profile GlobalProfiler;
global_variable U32 GlobalProfilerParent = 0;


global_function void StartTimingsProfile()
{
    MemoryZeroArray(GlobalProfiler.Timings);

    GlobalProfiler.TSCElapsed = 0;
    GlobalProfiler.CPUFrequency = GetCPUFrequency(CPU_FREQUENCY_MS);

#if DETECT_ORPHAN_TIMINGS
    GlobalProfiler.Started = TRUE;
    GlobalProfiler.Ended = FALSE;
#endif // DETECT_ORPHAN_TIMINGS

    GlobalProfiler.Start = ReadCPUTimer();
}


global_function void EndTimingsProfile()
{
    GlobalProfiler.TSCElapsed = ReadCPUTimer() - GlobalProfiler.Start;

#if DETECT_ORPHAN_TIMINGS
    Assert(GlobalProfiler.Started && "Profile has not been started");
    GlobalProfiler.Ended = TRUE;
#endif // DETECT_ORPHAN_TIMINGS
}


inline
global_function void _StartTiming(zone_block *block, U32 timingIndex, char const *label)
{
    zone_timing *timing = &GlobalProfiler.Timings[timingIndex];
    block->TSCElapsedOriginal = timing->TSCElapsedOriginal;

    block->ParentIndex = GlobalProfilerParent;
    block->Index = timingIndex;
    block->Label = label;

    GlobalProfilerParent = timingIndex;

#if DETECT_ORPHAN_TIMINGS
    GlobalProfiler.Timings[block->Index].HitCount++;
#endif // DETECT_ORPHAN_TIMINGS

    block->Start = ReadCPUTimer();
}


inline
global_function void _EndTiming(zone_block *block)
{
    U64 elapsed = ReadCPUTimer() - block->Start;
    GlobalProfilerParent = block->ParentIndex;

    zone_timing *parent = &GlobalProfiler.Timings[block->ParentIndex];
    zone_timing *timing = &GlobalProfiler.Timings[block->Index];

    parent->TSCElapsedChildren += elapsed;
    timing->TSCElapsedOriginal = block->TSCElapsedOriginal + elapsed;
    timing->TSCElapsed += elapsed;
#if DETECT_ORPHAN_TIMINGS
    timing->EndCount++;
#else
    timing->HitCount++;
#endif // DETECT_ORPHAN_TIMINGS

    timing->Label = block->Label;
}


inline
global_function void _PreWarmTiming(zone_block *block, U32 timingIndex, char const *label)
{
    block->Index = timingIndex;
    block->Label = label;
}


inline
global_function void _RestartTiming(zone_block *block)
{
    zone_timing *timing = &GlobalProfiler.Timings[block->Index];
    block->TSCElapsedOriginal = timing->TSCElapsedOriginal;

    block->ParentIndex = GlobalProfilerParent;
    GlobalProfilerParent = block->Index;

#if DETECT_ORPHAN_TIMINGS
    GlobalProfiler.Timings[block->Index].HitCount++;
#endif // DETECT_ORPHAN_TIMINGS

    block->Start = ReadCPUTimer();
}


global_function void PrintTimingsProfile()
{
#if DETECT_ORPHAN_TIMINGS
    Assert(GlobalProfiler.Started && "Profile has not been started");
    Assert(GlobalProfiler.Ended && "Profile has not been ended");
#endif // DETECT_ORPHAN_TIMINGS

    F64 totalTimeMs = ((F64)GlobalProfiler.TSCElapsed / (F64)GlobalProfiler.CPUFrequency) * 1000.0f;
    S64 unaccounted = GlobalProfiler.TSCElapsed;

    printf("Timings (cycles):\n");
    // Note (Aaron): Timer at index 0 represents "no timer" and should be skipped
    for (int i = 1; i < ArrayCount(GlobalProfiler.Timings); ++i)
    {
        zone_timing *timingPtr = &GlobalProfiler.Timings[i];
        if (!timingPtr->HitCount)
        {
            Assert(!timingPtr->Label && "Timing has a label; most likely RESTART_TIMING has not been called");
            continue;
        }

        Assert(timingPtr->Label && "Timing missing label; most likely END_TIMING has not been called");


#if DETECT_ORPHAN_TIMINGS
        Assert((timingPtr->HitCount == timingPtr->EndCount)
               && "Timing started but not finished or finished without starting");
#endif // DETECT_ORPHAN_TIMINGS

        U64 elapsed = timingPtr->TSCElapsed - timingPtr->TSCElapsedChildren;
        F64 percent = ((F64)elapsed / (F64)GlobalProfiler.TSCElapsed) * 100.0f;
        printf("  %s[%llu]: %llu (%.2f%%)", timingPtr->Label, timingPtr->HitCount, elapsed, percent);

        if (timingPtr->TSCElapsedOriginal != elapsed)
        {
            F64 percentWithChildren = (F64)timingPtr->TSCElapsedOriginal / (F64)GlobalProfiler.TSCElapsed * 100.0;
            printf(", %.2f%% w/children", percentWithChildren);
        }

        printf("\n");
        unaccounted -= elapsed;
    }

    Assert(unaccounted > 0 && "Unaccounted cycles can't be less than zero!");

    F64 percent = ((F64)unaccounted / (F64)GlobalProfiler.TSCElapsed) * 100.0f;
    printf("  Unaccounted: %llu (%.2f%s)\n\n", unaccounted, percent, "%");
    printf("Total cycles: %.4llu\n", GlobalProfiler.TSCElapsed);
    printf("Total time:   %.4fms (CPU freq %llu)\n", totalTimeMs, GlobalProfiler.CPUFrequency);
}


global_function U64 GetOSTimerFrequency()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
}


global_function U64 ReadOSTimer()
{
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    return value.QuadPart;
}


global_function U64 ReadCPUTimer()
{
    return __rdtsc();
}


global_function U64 GetCPUFrequency(U64 millisecondsToWait)
{
    U64 osFrequency = GetOSTimerFrequency();

    U64 cpuStart = ReadCPUTimer();
    U64 osStart = ReadOSTimer();
    U64 osEnd = 0;
    U64 osElapsed = 0;

    U64 osWaitTime = osFrequency * millisecondsToWait / 1000;

    while (osElapsed < osWaitTime)
    {
        osEnd = ReadOSTimer();
        osElapsed = osEnd - osStart;
    }

    U64 cpuEnd = ReadCPUTimer();
    U64 cpuElapsed = cpuEnd - cpuStart;
    U64 cpuFrequency = 0;
    if (osElapsed)
    {
        cpuFrequency = osFrequency * cpuElapsed / osElapsed;
    }

    return cpuFrequency;
}

#else // _WIN32

global_function void StartNamedTimingsProfile() { Assert(FALSE && "Not implemented"); }
global_function void EndNamedTimingsProfile() { Assert(FALSE && "Not implemented"); }
global_function void PrintNamedTimingsProfile() { Assert(FALSE && "Not implemented"); }
global_function void _StartTiming(zone_block *block, U32 timingIndex, char const *label) { Assert(FALSE && "Not implemented"); }
global_function void _EndTiming(zone_block *block) { Assert(FALSE && "Not implemented"); }

global_function U64 GetOSTimerFreq(void) { Assert(FALSE && "Not implemented"); }
global_function U64 ReadOSTimer(void) { Assert(FALSE && "Not implemented"); }
global_function U64 ReadCPUTimer() { Assert(FALSE && "Not implemented"); }
global_function U64 GetCPUFrequency(U64 millisecondsToWait) { Assert(FALSE && "Not implemented"); }

#endif // #else
