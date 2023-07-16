/* TODO (Aaron):
*/

#ifndef PLATFORM_METRICS_H
#define PLATFORM_METRICS_H

#include "base.h"
#include "base_types.h"


// Note (Aaron): Increase this count as needed
#define MAX_NAMED_TIMINGS 64

// Note (Aaron): Defines number of milliseconds used to determine the CPU frequency.
// A higher number is more accurate.
#define CPU_FREQUENCY_MS 200

// Note (Aaron): Enable this to check for timings that have been started but not ended or vice versa
#define DETECT_ORPHAN_TIMINGS 0


typedef struct timings_profile timings_profile;
typedef struct zone_timing zone_timing;
typedef struct zone_block zone_block;

global_function void StartTimingsProfile();
global_function void EndTimingsProfile();
global_function void PrintTimingsProfile();
global_function void _StartTiming(zone_block *block, U32 timingIndex, char const *label);
global_function void _EndTiming(zone_block *block);

global_function U64 GetOSTimerFrequency();
global_function U64 ReadOSTimer();
global_function U64 ReadCPUTimer();
global_function U64 GetCPUFrequency(U64 millisecondsToWait);


struct zone_timing
{
    U64 TSCElapsed;
    U64 TSCElapsedChildren;
    U64 TSCElapsedOriginal;
    U64 HitCount;
    char const *Label;

#if DETECT_ORPHAN_TIMINGS
    U64 EndCount;
#endif // DETECT_ORPHAN_TIMINGS
};


struct zone_block
{
    U32 ParentIndex;
    U32 Index;
    U64 TSCElapsedOriginal;
    U64 Start;
    char const *Label;

#if __cplusplus
    B8 AutoExecute;

    zone_block(U32 index, char const *label, B8 autoExecute)
    {
        AutoExecute = autoExecute;
        if (AutoExecute) _StartTiming(this, index, label);
    }

    ~zone_block(void)
    {
        if (AutoExecute) _EndTiming(this);
    }
#endif // __cplusplus
};


struct timings_profile
{
    U64 Start;
    U64 TSCElapsed;
    U64 CPUFrequency;
    zone_timing Timings[MAX_NAMED_TIMINGS];

#if DETECT_ORPHAN_TIMINGS
    B8 Started;
    B8 Ended;
#endif // DETECT_ORPHAN_TIMINGS
};


#ifndef __cplusplus //////////////////////////////////////////////////////////////////////////
// Note (Aaron): Use the following macros to start and end named timings within the same scope.
#define START_TIMING(label)     zone_block label##Block = {0};                       \
                                _StartTiming(&label##Block, __COUNTER__ + 1, #label);

#define END_TIMING(label)       _EndTiming(&label##Block);


// Note (Aaron): The following macros can be used to control the scope a timing is created in
// so it can be re-used later in the same scope.
#define PREWARM_TIMING(label)   zone_block label##Block = {0}; \
                                _PreWarmTiming(&label##Block, __COUNTER__ + 1, #label)

#define RESTART_TIMING(label)   _RestartTiming(&label##Block);

#else ////////////////////////////////////////////////////////////////////////////////////////
#define FUNCTION_TIMING         zone_block __func__##Block (__COUNTER__ + 1, __func__, TRUE);
#define ZONE_TIMING(label)      zone_block label##Block (__COUNTER__ + 1, #label, TRUE);

#define START_TIMING(label)     zone_block label##Block (__COUNTER__ + 1, #label, FALSE); \
                                _StartTiming(&label##Block, __COUNTER__ + 1, #label);

#define END_TIMING(label)       _EndTiming(&label##Block);

#define PREWARM_TIMING(label)   zone_block label##Block (__COUNTER__ + 1, #label, FALSE); \
                                _PreWarmTiming(&label##Block, __COUNTER__ + 1, #label);
#define RESTART_TIMING(label)   _RestartTiming(&label##Block);
#endif // __cplusplus ////////////////////////////////////////////////////////////////////////

#endif // PLATFORM_METRICS_H


#ifdef PLATFORM_METRICS_IMPLEMENTATION

#if _WIN32

#include <intrin.h>
#include <windows.h>
#include <stdio.h>

#include "base.h"
#include "base_memory.h"


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

global_function U64 GetOSTimerFreq(void) { Assert(FALSE && "Not implemented"); }
global_function U64 ReadOSTimer(void) { Assert(FALSE && "Not implemented"); }
global_function U64 ReadCPUTimer() { Assert(FALSE && "Not implemented"); }
global_function U64 GetCPUFrequency(U64 millisecondsToWait) { Assert(FALSE && "Not implemented"); }

#endif // #else

#endif // PLATFORM_METRICS_IMPLEMENTATION
