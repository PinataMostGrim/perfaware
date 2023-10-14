#ifndef REPETITION_TESTER_H ///////////////////////////////////////////////////
#define REPETITION_TESTER_H

#include <stdint.h>
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
   uint64_t E[RepValue_Count];
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
    uint64_t TargetProcessedByteCount;
    uint64_t CPUTimerFrequency;
    uint64_t TryForTime;
    uint64_t TestsStartedAt;

    test_mode Mode;
    rt__b32 PrintNewMinimums;
    uint32_t OpenBlockCount;
    uint32_t CloseBlockCount;

    repetition_value AccumulatedOnThisTest;
    test_results Results;
};


static uint64_t ReadCPUTimer();
static uint64_t EstimateCPUTimerFrequency();
static uint64_t GetOSTimerFrequency();
static uint64_t ReadOSTimer();
static uint64_t ReadOSPageFaultCount();

#endif // REPETITION_TESTER_H /////////////////////////////////////////////////


#ifdef REPETITION_TESTER_IMPLEMENTATION ///////////////////////////////////////

#include <stdio.h>


static double SecondsFromCPUTime(double cpuTime, uint64_t cpuTimerFrequency)
{
    double result = 0.0;
    if (cpuTimerFrequency)
    {
        result = (cpuTime / (double)cpuTimerFrequency);
    }

    return result;
}


static void PrintValue(char const *label, repetition_value value, uint64_t cpuTimerFrequency)
{
    uint64_t testCount = value.E[RepValue_TestCount];
    double divisor = testCount ? (double)testCount : 1;

    double e[RepValue_Count];
    for (uint32_t i = 0; i < RT__ArrayCount(e); ++i)
    {
        e[i] = (double)value.E[i] / divisor;
    }

    printf("%s: %.0f", label, e[RepValue_CPUTimer]);
    if (cpuTimerFrequency)
    {
        double seconds = SecondsFromCPUTime(e[RepValue_CPUTimer], cpuTimerFrequency);
        printf(" (%fms)", 1000.0f * seconds);

        if (e[RepValue_ByteCount] > 0)
        {
            double gigabyte = (1024.0f * 1024.0f * 1024.0f);
            double bestbandwidth = e[RepValue_ByteCount] / (gigabyte * seconds);
            printf(" %f gb/s", bestbandwidth);
        }
    }

    if(e[RepValue_MemPageFaults] > 0)
    {
        printf(" PF: %0.4f (%0.4fk/fault)", e[RepValue_MemPageFaults], e[RepValue_ByteCount] / (e[RepValue_MemPageFaults] * 1024.0));
    }
}


static void PrintResults(test_results results, uint64_t cpuTimerFrequency)
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


static void NewTestWave(repetition_tester *tester, uint64_t targetProcessedByteCount, uint64_t cpuTimerFrequency, uint32_t secondsToTry)
{
    if (tester->Mode == TestMode_Uninitialized)
    {
        tester->Mode = TestMode_Testing;
        tester->TargetProcessedByteCount = targetProcessedByteCount;
        tester->CPUTimerFrequency = cpuTimerFrequency;
        tester->PrintNewMinimums = RT__TRUE;
        tester->Results.Min.E[RepValue_CPUTimer] = (uint64_t)-1;
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


static void CountBytes(repetition_tester *tester, uint64_t byteCount)
{
    repetition_value *accumulated = &tester->AccumulatedOnThisTest;
    accumulated->E[RepValue_ByteCount] += byteCount;
}


static rt__b32 IsTesting(repetition_tester *tester)
{
    if (tester->Mode == TestMode_Testing)
    {
        repetition_value accumulated = tester->AccumulatedOnThisTest;
        uint64_t currentTime = ReadCPUTimer();

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
                for(uint32_t i = 0; i < RT__ArrayCount(accumulated.E); ++i)
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


static uint64_t EstimateCPUTimerFrequency()
{
    uint64_t millisecondsToWait = 100;
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
    RT__Assert(RT__FALSE && "Not implemented");
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

#endif

#endif // REPETITION_TESTER_IMPLEMENTATION ////////////////////////////////////

