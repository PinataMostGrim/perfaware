#ifndef REPETITION_TESTER_H ///////////////////////////////////////////////////
#define REPETITION_TESTER_H

#include <stdint.h>
typedef int32_t rt__b32;

#define RT__TRUE 1
#define RT__FALSE 0

#define RT__ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))


typedef enum
{
    TestMode_Uninitialized,
    TestMode_Testing,
    TestMode_Completed,
    TestMode_Error,
} test_modes;


typedef enum
{
    MType_TestCount,

    MType_CPUTimer,
    MType_MemPageFaults,
    MType_ByteCount,

    MType_Count,
} measurement_types;


typedef struct test_measurements test_measurements;
struct test_measurements
{
    uint64_t E[MType_Count];
};


typedef struct test_results test_results;
struct test_results
{
    test_measurements Total;
    test_measurements Min;
    test_measurements Max;
};


typedef struct repetition_tester repetition_tester;
struct repetition_tester
{
    uint64_t TargetProcessedByteCount;
    uint64_t CPUTimerFrequency;
    uint64_t TryForTime;
    uint64_t TestsStartedAt;

    test_modes Mode;
    rt__b32 PrintNewMinimums;
    uint32_t OpenBlockCount;
    uint32_t CloseBlockCount;

    test_measurements AccumulatedOnThisTest;
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


static void PrintValue(char const *label, test_measurements value, uint64_t cpuTimerFrequency)
{
    uint64_t testCount = value.E[MType_TestCount];
    double divisor = testCount ? (double)testCount : 1;

    double e[MType_Count];
    for (uint32_t i = 0; i < RT__ArrayCount(e); ++i)
    {
        e[i] = (double)value.E[i] / divisor;
    }

    printf("%s: %.0f", label, e[MType_CPUTimer]);
    if (cpuTimerFrequency)
    {
        double seconds = SecondsFromCPUTime(e[MType_CPUTimer], cpuTimerFrequency);
        printf(" (%fms)", 1000.0f * seconds);

        if (e[MType_ByteCount] > 0)
        {
            double gigabyte = (1024.0f * 1024.0f * 1024.0f);
            double bestbandwidth = e[MType_ByteCount] / (gigabyte * seconds);
            printf(" %f gb/s", bestbandwidth);
        }
    }

    if(e[MType_MemPageFaults] > 0)
    {
        printf(" PF: %0.4f (%0.4fk/fault)", e[MType_MemPageFaults], e[MType_ByteCount] / (e[MType_MemPageFaults] * 1024.0));
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
        tester->Results.Min.E[MType_CPUTimer] = (uint64_t)-1;
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

    test_measurements *accumulated = &tester->AccumulatedOnThisTest;
    accumulated->E[MType_MemPageFaults] -= ReadOSPageFaultCount();
    accumulated->E[MType_CPUTimer] -= ReadCPUTimer();
}


static void EndTime(repetition_tester *tester)
{
    test_measurements *accumulated = &tester->AccumulatedOnThisTest;
    accumulated->E[MType_CPUTimer] += ReadCPUTimer();
    accumulated->E[MType_MemPageFaults] += ReadOSPageFaultCount();
    ++tester->CloseBlockCount;
}


static void CountBytes(repetition_tester *tester, uint64_t byteCount)
{
    test_measurements *accumulated = &tester->AccumulatedOnThisTest;
    accumulated->E[MType_ByteCount] += byteCount;
}


static rt__b32 IsTesting(repetition_tester *tester)
{
    if (tester->Mode == TestMode_Testing)
    {
        test_measurements accumulated = tester->AccumulatedOnThisTest;
        uint64_t currentTime = ReadCPUTimer();

        // Note (Aaron): Don't count tests that had no timing blocks
        if (tester->OpenBlockCount)
        {
            if (tester->OpenBlockCount != tester->CloseBlockCount)
            {
                Error(tester, "Unbalanced BeginTime/EndTime");
            }

            if (accumulated.E[MType_ByteCount] != tester->TargetProcessedByteCount)
            {
                Error(tester, "Processed byte count mismatch");
            }

            if (tester->Mode == TestMode_Testing)
            {
                test_results *results = &tester->Results;

                accumulated.E[MType_TestCount] = 1;
                for(uint32_t i = 0; i < RT__ArrayCount(accumulated.E); ++i)
                {
                    results->Total.E[i] += accumulated.E[i];
                }

                if (results->Max.E[MType_CPUTimer] < accumulated.E[MType_CPUTimer])
                {
                    results->Max = accumulated;
                }

                if (results->Min.E[MType_CPUTimer] > accumulated.E[MType_CPUTimer])
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

