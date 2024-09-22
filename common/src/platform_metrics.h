/* Note (Aaron):
    - Add '#define PLATFORM_METRICS_IMPLEMENTATION' before the inclusion of this file in one C or C++ file to create the implementation
    - Add '#define PROFILER 1' before inclusion of this file to enable the taking of timings
    - Complile under C++ to enable automatic scope and function timings via the FUNCTION_TIMING and ZONE_TIMING(label) macros
*/

#ifndef PLATFORM_METRICS_H ////////////////////////////////////////////////////
#define PLATFORM_METRICS_H


// Note (Aaron): Increase this count as needed
#define MAX_NAMED_TIMINGS 64

// Note (Aaron): Defines number of milliseconds used to determine the CPU frequency. A higher number generates more accurate results.
#define CPU_FREQUENCY_MS 200

// Note (Aaron): Enable this to check for timings that have been started but not ended or vice versa. More necessary when using manual start and stop macros.
#define DETECT_ORPHAN_TIMINGS 0

#ifndef PROFILER
#define PROFILER 0
#endif


#include <stdint.h>
typedef int32_t pm__b32;

#define PM__TRUE 1
#define PM__FALSE 0

#include <assert.h>
#define PM__ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define PM__Assert(expr) assert(expr)


static void StartTimingsProfile();
static void EndTimingsProfile();
static void PrintProfileTimings();

static uint64_t GetCPUFrequency(uint64_t millisecondsToWait);
static uint64_t ReadCPUTimer();
static uint64_t GetOSTimerFrequency();
static uint64_t ReadOSTimer();
static uint64_t ReadOSPageFaultCount();


#if PROFILER ////////////////////////////////////////////////////////

// Note (Aaron): Use the following macro to start a named timing that tracks data throughput.
#define START_BANDWIDTH_TIMING(label, byteCount)    zone_block label##Block = {0}; \
                                                    _StartTiming(&label##Block, #label, __COUNTER__ + 1, byteCount);

// Note (Aaron): Use the following macro to start a named timing
#define START_TIMING(label)                 START_BANDWIDTH_TIMING(label, 0);

// Note (Aaron): Use the following macro to end a named timing that was started within the same scope.
#define END_TIMING(label)                   _EndTiming(&label##Block);

// Note (Aaron): Place this macro at the end of the profiler's compilation unit
// to assert there is enough room in the GlobalProfiler.Timings array to cover all timings.
#define ProfilerEndOfCompilationUnit static_assert(__COUNTER__ <= PM__ArrayCount(GlobalProfiler.Timings) , "__COUNTER__ exceeds the number of timings available");

#ifdef __cplusplus

// Note (Aaron): Use the following macros to automatically start and stop timings when entering and exiting scope.
// They have more of an impact on timings than the START / END timing macros above but do not need to be manually ended.
#define BANDWIDTH_TIMING(label, byteCount)  zone_block_autostart label##Block (#label, __COUNTER__ + 1, byteCount);
#define ZONE_TIMING(label)                  BANDWIDTH_TIMING(label, 0)
#define FUNCTION_TIMING                     zone_block_autostart __func__##Block (__func__, __COUNTER__ + 1, 0);

#endif // __cplusplus

#else // PROFILER ///////////////////////////////////////////////////

// Note (Aaron): Macro stubs for disabled timings
#define START_BANDWIDTH_TIMING(...)
#define START_TIMING(...)
#define END_TIMING(...)
#define ProfilerEndOfCompilationUnit

#ifdef __cplusplus
#define BANDWIDTH_TIMING(...)
#define ZONE_TIMING(...)
#define FUNCTION_TIMING
#endif // __cplusplus

#endif // PROFILER //////////////////////////////////////////////////


typedef struct zone_timing zone_timing;
struct zone_timing
{
    uint64_t TSCElapsed;
    uint64_t TSCElapsedChildren;
    uint64_t TSCElapsedOriginal;
    uint64_t HitCount;
    uint64_t ProcessedByteCount;
    char const *Label;

#if DETECT_ORPHAN_TIMINGS
    uint64_t EndCount;
#endif // DETECT_ORPHAN_TIMINGS
};


typedef struct zone_block zone_block;
struct zone_block
{
    uint32_t ParentIndex;
    uint32_t Index;
    uint64_t TSCElapsedOriginal;
    uint64_t Start;
    char const *Label;
};


typedef struct timings_profile timings_profile;
struct timings_profile
{
    uint64_t Start;
    uint64_t TSCElapsed;
    uint64_t CPUFrequency;
    zone_timing Timings[MAX_NAMED_TIMINGS];

#if DETECT_ORPHAN_TIMINGS
    pm__b32 Started;
    pm__b32 Ended;
#endif // DETECT_ORPHAN_TIMINGS
};


#ifdef __cplusplus
static void _StartTiming(zone_block *block, char const *label, uint32_t timingIndex, uint64_t byteCount);
static void _EndTiming(zone_block *block);

typedef struct zone_block_autostart zone_block_autostart;
struct zone_block_autostart
{
    zone_block Block;

    zone_block_autostart(char const *label, uint32_t index, uint64_t byteCount)
    {
        Block = {};
        _StartTiming(&Block, label, index, byteCount);
    }

    ~zone_block_autostart(void)
    {
        _EndTiming(&Block);
    }
};
#endif // __cplusplus

#endif // PLATFORM_METRICS_H //////////////////////////////////////////////////


#ifdef PLATFORM_METRICS_IMPLEMENTATION ////////////////////////////////////////

#include <stdio.h>
#include <inttypes.h>


static timings_profile GlobalProfiler;
static uint32_t GlobalProfilerParent = 0;


static void StartTimingsProfile()
{
    // Note (Aaron): Zero out the Timings array
    uint64_t count = sizeof(GlobalProfiler.Timings);
    void *destPtr = &GlobalProfiler.Timings;
    unsigned char *dest = (unsigned char *)destPtr;
    while(count--) *dest++ = (unsigned char)0;

#if DETECT_ORPHAN_TIMINGS
    GlobalProfiler.Started = TRUE;
    GlobalProfiler.Ended = FALSE;
#endif // DETECT_ORPHAN_TIMINGS

    GlobalProfiler.TSCElapsed = 0;
    GlobalProfiler.Start = ReadCPUTimer();
}


static void EndTimingsProfile()
{
    GlobalProfiler.TSCElapsed = ReadCPUTimer() - GlobalProfiler.Start;
    GlobalProfiler.CPUFrequency = GetCPUFrequency(CPU_FREQUENCY_MS);

#if DETECT_ORPHAN_TIMINGS
    PM__Assert(GlobalProfiler.Started && "Profile has not been started");
    GlobalProfiler.Ended = TRUE;
#endif // DETECT_ORPHAN_TIMINGS
}


#if PROFILER //////////////////////////////////////////////////////////////////

inline
static void _StartTiming(zone_block *block, char const *label, uint32_t timingIndex, uint64_t byteCount)
{
    zone_timing *timing = &GlobalProfiler.Timings[timingIndex];
    block->TSCElapsedOriginal = timing->TSCElapsedOriginal;
    timing->ProcessedByteCount += byteCount;

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
static void _EndTiming(zone_block *block)
{
    uint64_t elapsed = ReadCPUTimer() - block->Start;
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


static void PrintProfileTimings()
{
    double totalTimeMs = ((double)GlobalProfiler.TSCElapsed / (double)GlobalProfiler.CPUFrequency) * 1000.0f;

#if DETECT_ORPHAN_TIMINGS
    PM__Assert(GlobalProfiler.Started && "Profile has not been started");
    PM__Assert(GlobalProfiler.Ended && "Profile has not been ended");
#endif // DETECT_ORPHAN_TIMINGS

    int64_t unaccounted = GlobalProfiler.TSCElapsed;

    printf("Timings (cycles):\n");
    // Note (Aaron): Timer at index 0 represents "no timer" and should be skipped
    for (uint64_t i = 1; i < PM__ArrayCount(GlobalProfiler.Timings); ++i)
    {
        zone_timing *timingPtr = &GlobalProfiler.Timings[i];
        if (!timingPtr->HitCount)
        {
            PM__Assert(!timingPtr->Label && "Timing has a label; most likely RESTART_TIMING has not been called");
            continue;
        }

        PM__Assert(timingPtr->Label && "Timing missing label; most likely END_TIMING has not been called");

#if DETECT_ORPHAN_TIMINGS
        PM__Assert((timingPtr->HitCount == timingPtr->EndCount)
               && "Timing started but not finished or finished without starting");
#endif // DETECT_ORPHAN_TIMINGS

        uint64_t elapsed = timingPtr->TSCElapsed - timingPtr->TSCElapsedChildren;
        double percent = ((double)elapsed / (double)GlobalProfiler.TSCElapsed) * 100.0f;
        printf("  %s[%" PRIu64"]: %" PRIu64" (%.2f%%)", timingPtr->Label, timingPtr->HitCount, elapsed, percent);

        if (timingPtr->TSCElapsedOriginal != elapsed)
        {
            double percentWithChildren = (double)timingPtr->TSCElapsedOriginal / (double)GlobalProfiler.TSCElapsed * 100.0;
            printf(", %.2f%% w/children", percentWithChildren);
        }

        if (timingPtr->ProcessedByteCount)
        {
            double megabyte = 1024.0f * 1024.0f;
            double gigabyte = megabyte * 1024.0f;

            double seconds = timingPtr->TSCElapsed / (double)GlobalProfiler.CPUFrequency;
            double bytesPerSecond = timingPtr->ProcessedByteCount / seconds;
            double megabytes = timingPtr->ProcessedByteCount / megabyte;
            double gigabytesPerSecond = bytesPerSecond / gigabyte;

            printf(", %.3f mb at %.2f gb/sec", megabytes, gigabytesPerSecond);
        }

        printf("\n");
        unaccounted -= elapsed;
    }

    PM__Assert(unaccounted > 0 && "Unaccounted cycles can't be less than zero!");

    double percent = ((double)unaccounted / (double)GlobalProfiler.TSCElapsed) * 100.0f;
    printf("  Unaccounted: %" PRId64" (%.2f%s)\n\n", unaccounted, percent, "%");

    printf("Total cycles: %.4" PRIu64"\n", GlobalProfiler.TSCElapsed);
    printf("Total time:   %.4fms (CPU freq %" PRIu64")\n", totalTimeMs, GlobalProfiler.CPUFrequency);
}

#else // PROFILER ////////////////////////////////////////////////////////////

static void _StartTiming(zone_block *block, char const *label, uint32_t timingIndex, uint64_t byteCount) {}
static void _EndTiming(zone_block *block) {}


static void PrintProfileTimings()
{
    double totalTimeMs = ((double)GlobalProfiler.TSCElapsed / (double)GlobalProfiler.CPUFrequency) * 1000.0f;

    printf("Total cycles: %.4" PRIu64"\n", GlobalProfiler.TSCElapsed);
    printf("Total time:   %.4fms (CPU freq %" PRIu64")\n", totalTimeMs, GlobalProfiler.CPUFrequency);
}

#endif // PROFILER ////////////////////////////////////////////////////////////


static uint64_t GetCPUFrequency(uint64_t millisecondsToWait)
{
    uint64_t osFrequency = GetOSTimerFrequency();

    uint64_t cpuStart = ReadCPUTimer();
    uint64_t osStart = ReadOSTimer();
    uint64_t osEnd = 0;
    uint64_t osElapsed = 0;

    uint64_t osWaitTime = osFrequency * millisecondsToWait / 1000;

    while (osElapsed < osWaitTime)
    {
        osEnd = ReadOSTimer();
        osElapsed = osEnd - osStart;
    }

    uint64_t cpuEnd = ReadCPUTimer();
    uint64_t cpuElapsed = cpuEnd - cpuStart;
    uint64_t cpuFrequency = 0;
    if (osElapsed)
    {
        cpuFrequency = osFrequency * cpuElapsed / osElapsed;
    }

    return cpuFrequency;
}


#if _WIN32

#include <intrin.h>
#include <windows.h>

static uint64_t ReadCPUTimer()
{
    return __rdtsc();
}


static uint64_t GetOSTimerFrequency()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
}


static uint64_t ReadOSTimer()
{
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    return value.QuadPart;
}


static uint64_t ReadOSPageFaultCount()
{
    PM__Assert(PM__FALSE && "Not implemented");
}


#else // _WIN32

#include <x86intrin.h>
#include <sys/time.h>
#include <sys/resource.h>


static uint64_t ReadCPUTimer()
{
    return __rdtsc();
}


static uint64_t GetOSTimerFrequency()
{
    return 1000000;
}


static uint64_t ReadOSTimer()
{
    struct timeval value;
    gettimeofday(&value, 0);

    uint64_t result = GetOSTimerFrequency()*(uint64_t)value.tv_sec + (uint64_t)value.tv_usec;
    return result;
}


static uint64_t ReadOSPageFaultCount()
{
    struct rusage usage = {0};
    getrusage(RUSAGE_SELF, &usage);

    // Note (Aaron):
    //  ru_minflt: Page faults serviced without any I/O activity.
    //  ru_majflt: Page faults serviced that required I/O activity.
    uint64_t result = usage.ru_minflt + usage.ru_majflt;
    return result;
}

#endif // _WIN32

#endif // PLATFORM_METRICS_IMPLEMENTATION /////////////////////////////////////
