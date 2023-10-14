#ifndef REPETITION_TESTER_H ///////////////////////////////////////////////////
#define REPETITION_TESTER_H

#include <stdint.h>
typedef uint8_t rt__u8;
typedef uint32_t rt__u32;
typedef uint64_t rt__u64;
typedef int64_t rt__s64;
typedef double rt__f64;
typedef int8_t rt__b32;

#define RT__TRUE 1
#define RT__FALSE 0

#define RT__ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))


typedef enum
{
    TestMode_Uninitialized,
    TestMode_Testing,
    TestMode_Completed,
    TestMode_Error,
} test_mode;


typedef enum
{
    RepValue_TestCount,

    RepValue_CPUTimer,
    RepValue_MemPageFaults,
    RepValue_ByteCount,

    RepValue_Count,
} repetition_value_type;


typedef struct repetition_value repetition_value;
struct repetition_value
{
   rt__u64 E[RepValue_Count];
};


typedef struct test_results test_results;
struct test_results
{
    repetition_value Total;
    repetition_value Min;
    repetition_value Max;
};


typedef struct repetition_tester repetition_tester;
struct repetition_tester
{
    rt__u64 TargetProcessedByteCount;
    rt__u64 CPUTimerFrequency;
    rt__u64 TryForTime;
    rt__u64 TestsStartedAt;

    test_mode Mode;
    rt__b32 PrintNewMinimums;
    rt__u32 OpenBlockCount;
    rt__u32 CloseBlockCount;

    repetition_value AccumulatedOnThisTest;
    test_results Results;
};


static rt__u64 ReadCPUTimer();
static rt__u64 EstimateCPUTimerFrequency();
static rt__u64 GetOSTimerFrequency();
static rt__u64 ReadOSTimer();
static rt__u64 ReadOSPageFaultCount();

#endif // REPETITION_TESTER_H /////////////////////////////////////////////////


#ifdef REPETITION_TESTER_IMPLEMENTATION ///////////////////////////////////////

#include <stdio.h>


static rt__f64 SecondsFromCPUTime(rt__f64 cpuTime, rt__u64 cpuTimerFrequency)
{
    rt__f64 result = 0.0;
    if (cpuTimerFrequency)
    {
        result = (cpuTime / (rt__f64)cpuTimerFrequency);
    }

    return result;
}


static void PrintValue(char const *label, repetition_value value, rt__u64 cpuTimerFrequency)
{
    rt__u64 testCount = value.E[RepValue_TestCount];
    rt__f64 divisor = testCount ? (rt__f64)testCount : 1;

    rt__f64 e[RepValue_Count];
    for (rt__u32 i = 0; i < RT__ArrayCount(e); ++i)
    {
        e[i] = (rt__f64)value.E[i] / divisor;
    }

    printf("%s: %.0f", label, e[RepValue_CPUTimer]);
    if (cpuTimerFrequency)
    {
        rt__f64 seconds = SecondsFromCPUTime(e[RepValue_CPUTimer], cpuTimerFrequency);
        printf(" (%fms)", 1000.0f * seconds);

        if (e[RepValue_ByteCount] > 0)
        {
            rt__f64 gigabyte = (1024.0f * 1024.0f * 1024.0f);
            rt__f64 bestbandwidth = e[RepValue_ByteCount] / (gigabyte * seconds);
            printf(" %f gb/s", bestbandwidth);
        }
    }

    if(e[RepValue_MemPageFaults] > 0)
    {
        printf(" PF: %0.4f (%0.4fk/fault)", e[RepValue_MemPageFaults], e[RepValue_ByteCount] / (e[RepValue_MemPageFaults] * 1024.0));
    }
}


static void PrintResults(test_results results, rt__u64 cpuTimerFrequency)
{
    PrintValue("Min", results.Min, cpuTimerFrequency);
    printf("\n");
    PrintValue("Max", results.Max, cpuTimerFrequency);
    printf("\n");
    PrintValue("Avg", results.Total, cpuTimerFrequency);
    printf("\n");
}


static void Error(repetition_tester *tester, char const *message)
{
    tester->Mode = TestMode_Error;
    fprintf(stderr, "[ERROR]: %s\n", message);
}


static void NewTestWave(repetition_tester *tester, rt__u64 targetProcessedByteCount, rt__u64 cpuTimerFrequency, rt__u32 secondsToTry)
{
    if (tester->Mode == TestMode_Uninitialized)
    {
        tester->Mode = TestMode_Testing;
        tester->TargetProcessedByteCount = targetProcessedByteCount;
        tester->CPUTimerFrequency = cpuTimerFrequency;
        tester->PrintNewMinimums = RT__TRUE;
        tester->Results.Min.E[RepValue_CPUTimer] = (rt__u64)-1;
    }
    else if (tester->Mode == TestMode_Completed)
    {
        tester->Mode = TestMode_Testing;

        if (tester->TargetProcessedByteCount != targetProcessedByteCount)
        {
            Error(tester, "TargetProcessedByteCount changed");
        }

        if (tester->CPUTimerFrequency != cpuTimerFrequency)
        {
            Error(tester, "CPU frequency changed");
        }
    }

    tester->TryForTime = secondsToTry * cpuTimerFrequency;
    tester->TestsStartedAt = ReadCPUTimer();
}


static void BeginTime(repetition_tester *tester)
{
    ++tester->OpenBlockCount;

    repetition_value *accumulated = &tester->AccumulatedOnThisTest;
    accumulated->E[RepValue_MemPageFaults] -= ReadOSPageFaultCount();
    accumulated->E[RepValue_CPUTimer] -= ReadCPUTimer();
}


static void EndTime(repetition_tester *tester)
{
    repetition_value *accumulated = &tester->AccumulatedOnThisTest;
    accumulated->E[RepValue_CPUTimer] += ReadCPUTimer();
    accumulated->E[RepValue_MemPageFaults] += ReadOSPageFaultCount();
    ++tester->CloseBlockCount;
}


static void CountBytes(repetition_tester *tester, rt__u64 byteCount)
{
    repetition_value *accumulated = &tester->AccumulatedOnThisTest;
    accumulated->E[RepValue_ByteCount] += byteCount;
}


static rt__b32 IsTesting(repetition_tester *tester)
{
    if (tester->Mode == TestMode_Testing)
    {
        repetition_value accumulated = tester->AccumulatedOnThisTest;
        rt__u64 currentTime = ReadCPUTimer();

        // Note (Aaron): Don't count tests that had no timing blocks
        if (tester->OpenBlockCount)
        {
            if (tester->OpenBlockCount != tester->CloseBlockCount)
            {
                Error(tester, "Unbalanced BeginTime/EndTime");
            }

            if (accumulated.E[RepValue_ByteCount] != tester->TargetProcessedByteCount)
            {
                Error(tester, "Processed byte count mismatch");
            }

            if (tester->Mode == TestMode_Testing)
            {
                test_results *results = &tester->Results;

                accumulated.E[RepValue_TestCount] = 1;
                for(rt__u32 i = 0; i < RT__ArrayCount(accumulated.E); ++i)
                {
                    results->Total.E[i] += accumulated.E[i];
                }

                if (results->Max.E[RepValue_CPUTimer] < accumulated.E[RepValue_CPUTimer])
                {
                    results->Max = accumulated;
                }

                if (results->Min.E[RepValue_CPUTimer] > accumulated.E[RepValue_CPUTimer])
                {
                    results->Min = accumulated;

                    // Note (Aaron): If we hit a new minimum, we want to continue searching so the
                    // start time is reset to current time.
                    tester->TestsStartedAt = currentTime;
                    if (tester->PrintNewMinimums)
                    {
                        PrintValue("Min", results->Min, tester->CPUTimerFrequency);
                        printf("                                   \r");
                    }
                }

                tester->OpenBlockCount = 0;
                tester->CloseBlockCount = 0;

                uint64_t count = sizeof(tester->AccumulatedOnThisTest);
                void *destPtr = &tester->AccumulatedOnThisTest;
                unsigned char *dest = (unsigned char *)destPtr;
                while(count--) *dest++ = (unsigned char)0;
            }
        }

        if ((currentTime - tester->TestsStartedAt) > tester->TryForTime)
        {
            tester->Mode = TestMode_Completed;

            printf("                                                          \r");
            PrintResults(tester->Results, tester->CPUTimerFrequency);
        }
    }

    rt__b32 result = (tester->Mode == TestMode_Testing);
    return result;
}


static rt__u64 EstimateCPUTimerFrequency()
{
    rt__u64 millisecondsToWait = 100;
    rt__u64 osFrequency = GetOSTimerFrequency();

    rt__u64 cpuStart = ReadCPUTimer();
    rt__u64 osStart = ReadOSTimer();
    rt__u64 osEnd = 0;
    rt__u64 osElapsed = 0;

    rt__u64 osWaitTime = osFrequency * millisecondsToWait / 1000;

    while (osElapsed < osWaitTime)
    {
        osEnd = ReadOSTimer();
        osElapsed = osEnd - osStart;
    }

    rt__u64 cpuEnd = ReadCPUTimer();
    rt__u64 cpuElapsed = cpuEnd - cpuStart;
    rt__u64 cpuFrequency = 0;
    if (osElapsed)
    {
        cpuFrequency = osFrequency * cpuElapsed / osElapsed;
    }

    return cpuFrequency;
}

#if _WIN32

#include <intrin.h>
#include <windows.h>

static rt__u64 ReadCPUTimer()
{
    return __rdtsc();
}


static rt__u64 GetOSTimerFrequency()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
}


static rt__u64 ReadOSTimer()
{
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    return value.QuadPart;
}


static rt__u64 ReadOSPageFaultCount()
{
    RT__Assert(RT__FALSE && "Not implemented");
}

#else // _WIN32

#include <x86intrin.h>
#include <sys/time.h>
#include <sys/resource.h>


static rt__u64 ReadCPUTimer()
{
    return __rdtsc();
}


static rt__u64 GetOSTimerFrequency()
{
    return 1000000;
}


static rt__u64 ReadOSTimer()
{
    struct timeval value;
    gettimeofday(&value, 0);

    rt__u64 result = GetOSTimerFrequency()*(rt__u64)value.tv_sec + (rt__u64)value.tv_usec;
    return result;
}


static rt__u64 ReadOSPageFaultCount()
{
    struct rusage usage = {0};
    getrusage(RUSAGE_SELF, &usage);

    // Note (Aaron):
    //  ru_minflt: Page faults serviced without any I/O activity.
    //  ru_majflt: Page faults serviced that required I/O activity.
    rt__u64 result = usage.ru_minflt + usage.ru_majflt;
    return result;
}

#endif

#endif // REPETITION_TESTER_IMPLEMENTATION ////////////////////////////////////

