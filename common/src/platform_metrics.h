/* TODO (Aaron):
*/

#ifndef PLATFORM_METRICS_H
#define PLATFORM_METRICS_H

#include "base.h"


typedef struct
{
    S64 Start;
    S64 End;
    S64 Duration;
} metric_timing;


global_function void InitializeMetricTiming(metric_timing *timing);
global_function U64 GetOSTimerFrequency();
global_function U64 ReadOSTimer();
global_function U64 ReadCPUTimer();
global_function U64 GetCPUFrequency(U64 millisecondsToWait);

#endif // PLATFORM_METRICS_H


#ifdef PLATFORM_METRICS_IMPLEMENTATION

#if _WIN32

#include <intrin.h>
#include <windows.h>

#include "base.h"


global_function void InitializeMetricTiming(metric_timing *timing)
{
    timing->Start = 0;
    timing->End = 0;
    timing->Duration = 0;
}


inline
global_function void StartCPUTiming(metric_timing *timing)
{
    timing->Start = ReadCPUTimer();
}


inline
global_function void EndCPUTimingAndIncrementDuration(metric_timing *timing)
{
    timing->End = ReadCPUTimer();
    U64 duration = timing->End - timing->Start;
    timing->Duration += duration;
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

#else

global_function U64 GetOSTimerFreq(void)
{
    Assert(FALSE && "Not implemented");
}

global_function U64 ReadOSTimer(void)
{
    Assert(FALSE && "Not implemented");
}

global_function U64 ReadCPUTimer()
{
    Assert(FALSE && "Not implemented");
}

global_function U64 GetCPUFrequency(U64 millisecondsToWait)
{
    Assert(FALSE && "Not implemented");
}

#endif

#endif // PLATFORM_METRICS_IMPLEMENTATION
