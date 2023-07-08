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


typedef struct
{
    U64 TSCElapsed;
    U64 HitCount;
    char *Label;
} named_timing;


typedef struct
{
    U64 Start;
    U64 End;
    U64 Index;
    U64 TSCElapsed;
    char *Label;
} timings_block;


typedef struct
{
    U64 Start;
    U64 TSCElapsed;
    U64 CPUFrequency;
    named_timing Timings[MAX_NAMED_TIMINGS];
    B8 Started;
    B8 Ended;
} timings_profile;


global_function void StartTimingsProfile();
global_function void EndTimingsProfile();
global_function void PrintTimingsProfile();

// Note (Aaron): Use the following macros to start and end named timings within the same scope.
#define START_NAMED_TIMING(name)    timings_block name##Block = {0}; \
                                    name##Block.Index = __COUNTER__ + 1; \
                                    name##Block.Label = #name; \
                                    name##Block.Start = ReadCPUTimer();

#define END_NAMED_TIMING(name)      name##Block.TSCElapsed = ReadCPUTimer() - name##Block.Start; \
                                    GlobalProfiler.Timings[name##Block.Index].Label = name##Block.Label; \
                                    GlobalProfiler.Timings[name##Block.Index].TSCElapsed += name##Block.TSCElapsed; \
                                    GlobalProfiler.Timings[name##Block.Index].HitCount++;

// Note (Aaron): The following macros can be used to control the scope a named timing is created in
// and to re-use the same timing later in the same scope.
#define PREWARM_NAMED_TIMING(name)  timings_block name##Block = {0}; \
                                    name##Block.Index = __COUNTER__ + 1; \
                                    name##Block.Label = #name; \

#define RESTART_NAMED_TIMING(name)  name##Block.Start = ReadCPUTimer();


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


global_function void StartTimingsProfile()
{
    MemoryZeroArray(GlobalProfiler.Timings);

    GlobalProfiler.TSCElapsed = 0;
    GlobalProfiler.Started = TRUE;
    GlobalProfiler.Ended = FALSE;

    GlobalProfiler.CPUFrequency = GetCPUFrequency(CPU_FREQUENCY_MS);
    GlobalProfiler.Start = ReadCPUTimer();
}


global_function void EndTimingsProfile()
{
    Assert(GlobalProfiler.Started && "Profile has not been started");

    GlobalProfiler.TSCElapsed = ReadCPUTimer() - GlobalProfiler.Start;
    GlobalProfiler.Ended = TRUE;
}


global_function void PrintTimingsProfile()
{
    Assert(GlobalProfiler.Started && "Profile has not been started");
    Assert(GlobalProfiler.Ended && "Profile has not been ended");

    F64 totalTimeMs = ((F64)GlobalProfiler.TSCElapsed / (F64)GlobalProfiler.CPUFrequency) * 1000.0f;
    S64 unaccounted = GlobalProfiler.TSCElapsed;

    printf("Timings (cycles):\n");
    // Note (Aaron): Timer at index 0 represents "no timer" and should be skipped
    for (int i = 1; i < ArrayCount(GlobalProfiler.Timings); ++i)
    {
        named_timing *timingPtr = &GlobalProfiler.Timings[i];
        if (!timingPtr->Label)
        {
            break;
        }


        F64 percent = ((F64)timingPtr->TSCElapsed / (F64)GlobalProfiler.TSCElapsed) * 100.0f;
        printf("  %s[%llu]: %llu (%.2f%s)\n", timingPtr->Label, timingPtr->HitCount, timingPtr->TSCElapsed, percent, "%");
        unaccounted -= timingPtr->TSCElapsed;
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
