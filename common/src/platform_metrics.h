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


typedef struct
{
    U64 TSCElapsed;
    U64 TSCElapsedChildren;
    U64 HitCount;
#if DETECT_ORPHAN_TIMINGS
    U64 EndCount;
#endif
    char *Label;
} timing_entry;


typedef struct
{
    U32 TimingIndex;
    U32 ParentTimingIndex;
    U64 Start;
    U64 TSCElapsed;
    U64 TSCElapsedChildren;
    char *Label;
} timing_block;


typedef struct
{
    U64 Start;
    U64 TSCElapsed;
    U64 CPUFrequency;
    timing_entry Timings[MAX_NAMED_TIMINGS];
#if DETECT_ORPHAN_TIMINGS
    B8 Started;
    B8 Ended;
#endif
} timings_profile;


global_function void StartTimingsProfile();
global_function void EndTimingsProfile();
global_function void PrintTimingsProfile();

#if DETECT_ORPHAN_TIMINGS
// Note (Aaron): To detect orphaned timers:
// - HitCount must be incremented when START_TIMING or RESTART_TIMING is invoked,
//   and then decremented when END_TIMING is invoked to offset the existing increment.
// - EndCount must be incremented when END_TIMING is invoked
#define DETECT_ORPHAN_INCREMENT_HIT_COUNT(label)    GlobalProfiler.Timings[label##Block.TimingIndex].HitCount++;
#define DETECT_ORPHAN_INCREMENT_END_COUNT(label)    label##TimingPtr->HitCount--; label##TimingPtr->EndCount++;
#else
#define DETECT_ORPHAN_INCREMENT_HIT_COUNT(label)
#define DETECT_ORPHAN_INCREMENT_END_COUNT(label)
#endif // DETECT_ORPHAN_TIMINGS

// Note (Aaron): Use the following macros to start and end named timings within the same scope.
#define START_TIMING(label)     timing_block label##Block = {0}; \
                                label##Block.TimingIndex = __COUNTER__ + 1; \
                                label##Block.Label = #label; \
                                label##Block.ParentTimingIndex = GlobalProfilerParent; \
                                GlobalProfilerParent = label##Block.TimingIndex; \
                                DETECT_ORPHAN_INCREMENT_HIT_COUNT(label) \
                                label##Block.Start = ReadCPUTimer();

#define END_TIMING(label)       label##Block.TSCElapsed = ReadCPUTimer() - label##Block.Start; \
                                GlobalProfilerParent = label##Block.ParentTimingIndex; \
                                timing_entry *label##TimingPtr = &GlobalProfiler.Timings[label##Block.TimingIndex]; \
                                label##TimingPtr->TSCElapsed += label##Block.TSCElapsed; \
                                label##TimingPtr->HitCount++; \
                                DETECT_ORPHAN_INCREMENT_END_COUNT(label) \
                                label##TimingPtr->Label = label##Block.Label; \
                                timing_entry *label##ParentTimingPtr = &GlobalProfiler.Timings[label##Block.ParentTimingIndex]; \
                                label##ParentTimingPtr->TSCElapsedChildren += label##Block.TSCElapsed;

// Note (Aaron): The following macros can be used to control the scope a timing is created in
// so it can be re-used later in the same scope.
#define PREWARM_TIMING(label)   timing_block label##Block = {0}; \
                                label##Block.TimingIndex = __COUNTER__ + 1; \
                                label##Block.Label = #label;

#define RESTART_TIMING(label)   label##Block.ParentTimingIndex = GlobalProfilerParent; \
                                GlobalProfilerParent = label##Block.TimingIndex; \
                                DETECT_ORPHAN_INCREMENT_HIT_COUNT(label) \
                                label##Block.Start = ReadCPUTimer();


global_function U64 GetOSTimerFrequency();
global_function U64 ReadOSTimer();
global_function U64 ReadCPUTimer();
global_function U64 GetCPUFrequency(U64 millisecondsToWait);

#endif // PLATFORM_METRICS_H


#ifdef PLATFORM_METRICS_IMPLEMENTATION

#if _WIN32

#include <intrin.h>
#include <windows.h>
#include <stdio.h>

#include "base.h"
#include "base_memory.h"


global_variable timings_profile GlobalProfiler;
global_variable U32 GlobalProfilerParent;


global_function void StartTimingsProfile()
{
    MemoryZeroArray(GlobalProfiler.Timings);

    GlobalProfiler.TSCElapsed = 0;
#if DETECT_ORPHAN_TIMINGS
    GlobalProfiler.Started = TRUE;
    GlobalProfiler.Ended = FALSE;
#endif
    GlobalProfiler.CPUFrequency = GetCPUFrequency(CPU_FREQUENCY_MS);
    GlobalProfiler.Start = ReadCPUTimer();
}


global_function void EndTimingsProfile()
{
#if DETECT_ORPHAN_TIMINGS
    Assert(GlobalProfiler.Started && "Profile has not been started");
#endif

    GlobalProfiler.TSCElapsed = ReadCPUTimer() - GlobalProfiler.Start;
#if DETECT_ORPHAN_TIMINGS
    GlobalProfiler.Ended = TRUE;
#endif
}


global_function void PrintTimingsProfile()
{
#if DETECT_ORPHAN_TIMINGS
    Assert(GlobalProfiler.Started && "Profile has not been started");
    Assert(GlobalProfiler.Ended && "Profile has not been ended");
#endif

    F64 totalTimeMs = ((F64)GlobalProfiler.TSCElapsed / (F64)GlobalProfiler.CPUFrequency) * 1000.0f;
    S64 unaccounted = GlobalProfiler.TSCElapsed;

    printf("Timings (cycles):\n");
    // Note (Aaron): Timer at index 0 represents "no timer" and should be skipped
    for (int i = 1; i < ArrayCount(GlobalProfiler.Timings); ++i)
    {
        timing_entry *timingPtr = &GlobalProfiler.Timings[i];
        if (!timingPtr->HitCount)
        {
            Assert(!timingPtr->Label && "Timing has a label; most likely RESTART_TIMING has not been called");
            break;
        }

        Assert(timingPtr->Label && "Timing missing label; most likely END_TIMING has not been called");

#if DETECT_ORPHAN_TIMINGS
        Assert((timingPtr->HitCount == timingPtr->EndCount)
               && "Timing started but not finished or finished without starting");
#endif

        U64 elapsed = timingPtr->TSCElapsed - timingPtr->TSCElapsedChildren;
        F64 percent = ((F64)elapsed / (F64)GlobalProfiler.TSCElapsed) * 100.0f;
        printf("  %s[%llu]: %llu (%.2f%%)", timingPtr->Label, timingPtr->HitCount, elapsed, percent);
        if (timingPtr->TSCElapsedChildren)
        {
            F64 percentWithChildren =  (F64)timingPtr->TSCElapsed / (F64)GlobalProfiler.TSCElapsed * 100.0;
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

#else // #if _WIN32

global_function void StartNamedTimingsProfile() { Assert(FALSE && "Not implemented"); }
global_function void EndNamedTimingsProfile() { Assert(FALSE && "Not implemented"); }
global_function void PrintNamedTimingsProfile() { Assert(FALSE && "Not implemented"); }

global_function U64 GetOSTimerFreq(void) { Assert(FALSE && "Not implemented"); }
global_function U64 ReadOSTimer(void) { Assert(FALSE && "Not implemented"); }
global_function U64 ReadCPUTimer() { Assert(FALSE && "Not implemented"); }
global_function U64 GetCPUFrequency(U64 millisecondsToWait) { Assert(FALSE && "Not implemented"); }

#endif // #else

#endif // PLATFORM_METRICS_IMPLEMENTATION
