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


typedef enum
{
    TestMode_Uninitialized,
    TestMode_Testing,
    TestMode_Completed,
    TestMode_Error,
} test_mode;


typedef struct test_results test_results;
struct test_results
{
    rt__u64 TestCount;
    rt__u64 TotalTime;
    rt__u64 MaxTime;
    rt__u64 MinTime;
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
    rt__u64 TimeAccumulatedOnTestInstance;
    rt__u64 BytesAccumulatedOnTestInstance;

    test_results Results;
};


static rt__u64 ReadCPUTimer();
static rt__u64 EstimateCPUTimerFrequency();
static rt__u64 ReadOSTimer();
static rt__u64 GetOSTimerFrequency();

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


static void PrintTime(char const *label, rt__f64 cpuTime, rt__u64 cpuTimerFrequency, rt__u64 byteCount)
{
    printf("%s: %.0f", label, cpuTime);
    if (cpuTimerFrequency)
    {
        rt__f64 seconds = SecondsFromCPUTime(cpuTime, cpuTimerFrequency);
        printf(" (%fms)", 1000.0f * seconds);

        if (byteCount)
        {
            rt__f64 gigabyte = (1024.0f * 1024.0f * 1024.0f);
            rt__f64 bestbandwidth = byteCount / (gigabyte * seconds);
            printf(" %f gb/s", bestbandwidth);
        }
    }
}


static void PrintResults(test_results results, rt__u64 cpuTimerFrequency, rt__u64 byteCount)
{
    PrintTime("Min", (rt__f64)results.MinTime, cpuTimerFrequency, byteCount);
    printf("\n");

    PrintTime("Max", (rt__f64)results.MaxTime, cpuTimerFrequency, byteCount);
    printf("\n");

    if (results.TestCount)
    {
        PrintTime("Avg", (rt__f64)results.TotalTime / (rt__f64)results.TestCount, cpuTimerFrequency, byteCount);
        printf("\n");
    }
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
        tester->Results.MinTime = (rt__u64)-1;
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
    tester->TimeAccumulatedOnTestInstance -= ReadCPUTimer();
}


static void EndTime(repetition_tester *tester)
{
    ++tester->CloseBlockCount;
    tester->TimeAccumulatedOnTestInstance += ReadCPUTimer();
}


static void CountBytes(repetition_tester *tester, rt__u64 byteCount)
{
    tester->BytesAccumulatedOnTestInstance += byteCount;
}


static rt__b32 IsTesting(repetition_tester *tester)
{
    if (tester->Mode == TestMode_Testing)
    {
        rt__u64 currentTime = ReadCPUTimer();

        if (tester->OpenBlockCount)
        {
            if (tester->OpenBlockCount != tester->CloseBlockCount)
            {
                Error(tester, "Unbalanced BeginTime/EndTime");
            }

            if (tester->BytesAccumulatedOnTestInstance != tester->TargetProcessedByteCount)
            {
                Error(tester, "Processed byte count mismatch");
            }

            if (tester->Mode == TestMode_Testing)
            {
                test_results *results = &tester->Results;
                rt__u64 elapsedTime = tester->TimeAccumulatedOnTestInstance;
                results->TestCount++;
                results->TotalTime += elapsedTime;

                if(results->MaxTime < elapsedTime)
                {
                    results->MaxTime = elapsedTime;
                }

                if (results->MinTime > elapsedTime)
                {
                    results->MinTime = elapsedTime;

                    // Note (Aaron): If we hit a new minimum, we want to continue searching so the
                    // start time is reset to current time.
                    tester->TestsStartedAt = currentTime;

                    if (tester->PrintNewMinimums)
                    {
                        PrintTime("Min", (rt__f64)results->MinTime, tester->CPUTimerFrequency, tester->BytesAccumulatedOnTestInstance);
                        printf("               \r");
                    }
                }

                tester->OpenBlockCount = 0;
                tester->CloseBlockCount = 0;
                tester->TimeAccumulatedOnTestInstance = 0;
                tester->BytesAccumulatedOnTestInstance = 0;
            }
        }

        if ((currentTime - tester->TestsStartedAt) > tester->TryForTime)
        {
            tester->Mode = TestMode_Completed;

            printf("                                                          \r");
            PrintResults(tester->Results, tester->CPUTimerFrequency, tester->TargetProcessedByteCount);
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


static rt__u64 ReadOSTimer()
{
    LARGE_INTEGER value;
    QueryPerformanceCounter(&value);
    return value.QuadPart;
}


static rt__u64 GetOSTimerFrequency()
{
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    return frequency.QuadPart;
}

#else // _WIN32

#include <x86intrin.h>
#include <sys/time.h>


static rt__u64 ReadCPUTimer()
{
    return __rdtsc();
}


static rt__u64 ReadOSTimer()
{
    struct timeval value;
    gettimeofday(&value, 0);

    rt__u64 result = GetOSTimerFrequency()*(rt__u64)value.tv_sec + (rt__u64)value.tv_usec;
    return result;
}


static rt__u64 GetOSTimerFrequency()
{
    return 1000000;
}

#endif

#endif // REPETITION_TESTER_IMPLEMENTATION ////////////////////////////////////

