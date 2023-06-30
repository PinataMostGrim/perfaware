/* TODO (Aaron):
*/

#ifndef PLATFORM_METRICS_H
#define PLATFORM_METRICS_H

#include "base.h"


// Note (Aaron): Increase this count as needed
#define MAX_NAMED_TIMINGS 64

// Note (Aaron): Defines number of milliseconds used to determine the CPU frequency.
// A higher number is more accurate.
#define CPU_FREQUENCY_MS 200


typedef struct
{
    S64 Start;
    S64 End;
    S64 Duration;
} general_timing;


typedef struct
{
    char *Name;
    U64 Start;
    U64 End;
    S64 Duration;
} named_timing;


typedef struct
{
    U64 Start;
    U64 End;
    U64 Duration;
    named_timing Timings[MAX_NAMED_TIMINGS];
    B32 Started;
    B32 Ended;
    U64 CPUFrequency;
} timings_profile;


global_function void InitializeGeneralTiming(general_timing *timing);
global_function void StartCPUTiming(general_timing *timing);
global_function void EndCPUTimingAndIncrementDuration(general_timing *timing);

global_function void InitializeNamedTiming(named_timing *timing, char *name);
global_function void StartNamedTimingsProfile();
global_function void EndNamedTimingsProfile();
global_function void PrintNamedTimingsProfile();

// Note (Aaron): Use the following macros to start and end named timings within the same scope.
#define START_NAMED_TIMING(name) named_timing *name##TimingPtr = &Profile.Timings[__COUNTER__]; name##TimingPtr->Name = #name; name##TimingPtr->Start = ReadCPUTimer();
#define END_NAMED_TIMING(name); name##TimingPtr->End = ReadCPUTimer(); name##TimingPtr->Duration += (name##TimingPtr->End - name##TimingPtr->Start);

// Note (Aaron): The following macros can be used to control the scope a named timing is created in
// and to re-use the same timing later in the same scope.
#define PREWARM_NAMED_TIMING(name) named_timing *name##TimingPtr = &Profile.Timings[__COUNTER__]; name##TimingPtr->Name = #name;
#define RESTART_NAMED_TIMING(name) name##TimingPtr->Start = ReadCPUTimer();

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


global_variable timings_profile Profile;


global_function void InitializeGeneralTiming(general_timing *timing)
{
    timing->Start = 0;
    timing->End = 0;
    timing->Duration = 0;
}


inline
global_function void StartCPUTiming(general_timing *timing)
{
    timing->Start = ReadCPUTimer();
}


inline
global_function void EndCPUTimingAndIncrementDuration(general_timing *timing)
{
    timing->End = ReadCPUTimer();
    U64 duration = timing->End - timing->Start;
    timing->Duration += duration;
}


global_function void InitializeNamedTiming(named_timing *timing, char *name)
{
    timing->Name = name;
    timing->Start = 0;
    timing->End = 0;
    timing->Duration = 0;
}


global_function void StartNamedTimingsProfile()
{
    for (int i = 0; i < ArrayCount(Profile.Timings); ++i)
    {
        InitializeNamedTiming(&Profile.Timings[i], (char *)"");
    }

    Profile.End = 0;
    Profile.Duration = 0;
    Profile.Started = TRUE;
    Profile.Ended = FALSE;

    Profile.CPUFrequency = GetCPUFrequency(CPU_FREQUENCY_MS);
    Profile.Start = ReadCPUTimer();
}


global_function void EndNamedTimingsProfile()
{
    Profile.End = ReadCPUTimer();
    Profile.Duration = Profile.End - Profile.Start;
    Profile.Ended = TRUE;
}


global_function void PrintNamedTimingsProfile()
{
    Assert(Profile.Ended && "Profile has not been ended");

    F64 totalTimeMs = ((F64)Profile.Duration / (F64)Profile.CPUFrequency) * 1000.0f;
    S64 unaccounted = Profile.Duration;

    printf("Timings (cycles):\n");
    for (int i = 0; i < ArrayCount(Profile.Timings); ++i)
    {
        named_timing *timingPtr = &Profile.Timings[i];
        if (timingPtr->Start == 0 && timingPtr->End == 0)
        {
            break;
        }

        Assert((timingPtr->Start != 0 && timingPtr->End != 0)
               && "Timing started but not finished or finished without starting");

        printf("  %s: %llu (%.2f%s)\n", timingPtr->Name,  timingPtr->Duration, ((F64)timingPtr->Duration / (F64)Profile.Duration) * 100.0f, "%");
        unaccounted -= timingPtr->Duration;
    }

    Assert(unaccounted > 0 && "Unaccounted cycles can't be less than zero!");

    printf("  Unaccounted: %llu (%.2f%s)\n\n", unaccounted, ((F64)unaccounted / (F64)Profile.Duration) * 100.0f, "%");
    printf("Total cycles: %.4llu\n", Profile.Duration);
    printf("Total time:   %.4fms (CPU freq %llu)\n", totalTimeMs, Profile.CPUFrequency);
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

global_function void InitializeGeneralTiming(general_timing *timing) { Assert(FALSE && "Not implemented"); }
global_function void StartCPUTiming(general_timing *timing) { Assert(FALSE && "Not implemented"); }
global_function void EndCPUTimingAndIncrementDuration(general_timing *timing) { Assert(FALSE && "Not implemented"); }

global_function void InitializeNamedTiming(named_timing *timing, char *name) { Assert(FALSE && "Not implemented"); }
global_function void StartNamedTimingsProfile() { Assert(FALSE && "Not implemented"); }
global_function void EndNamedTimingsProfile() { Assert(FALSE && "Not implemented"); }
global_function void PrintNamedTimingsProfile() { Assert(FALSE && "Not implemented"); }

global_function U64 GetOSTimerFreq(void) { Assert(FALSE && "Not implemented"); }
global_function U64 ReadOSTimer(void) { Assert(FALSE && "Not implemented"); }
global_function U64 ReadCPUTimer() { Assert(FALSE && "Not implemented"); }
global_function U64 GetCPUFrequency(U64 millisecondsToWait) { Assert(FALSE && "Not implemented"); }

#endif // #else

#endif // PLATFORM_METRICS_IMPLEMENTATION
