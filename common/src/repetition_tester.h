#ifndef REPETITION_TESTER_H ///////////////////////////////////////////////////
#define REPETITION_TESTER_H

#include <stdint.h>
typedef int32_t rt__b32;

#define RT__TRUE 1
#define RT__FALSE 0

#define RT__ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define RT__Assert(condition, ...) \
    do { \
        if (!(condition)) { \
            *(volatile unsigned *)0 = 0; \
        } \
    } while(0)


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

    StatValue_Seconds,
    StatValue_GBPerSecond,
    StatValue_KBPerPageFault,

    MType_Count,
} measurement_types;


typedef struct test_measurements test_measurements;
struct test_measurements
{
    uint64_t Raw[MType_Count];

    // Note (Aaron H): These values are computed from the Raw[] array and the CPUTimerFrequency
    double DerivedValues[MType_Count];
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


static void InitializeTester();
static void NewTestWave(repetition_tester *tester, uint64_t targetProcessedByteCount, uint64_t cpuTimerFrequency, uint32_t secondsToTry);
static rt__b32 IsTesting(repetition_tester *tester);
static void BeginTime(repetition_tester *tester);
static void EndTime(repetition_tester *tester);
static void CountBytes(repetition_tester *tester, uint64_t byteCount);

static void Error(repetition_tester *tester, char const *message);
static void PrintValue(char const *label, test_measurements value);
static void PrintResults(test_results results);

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


static void ComputeDerivedValues(test_measurements *value, uint64_t cpuTimerFrequency)
{
    uint64_t testCount = value->Raw[MType_TestCount];
    double divisor = testCount ? (double)testCount : 1;

    double *derivedValues = value->DerivedValues;
    for(uint32_t i = 0; i < RT__ArrayCount(value->DerivedValues); ++i)
    {
        derivedValues[i] = (double)value->Raw[i] / divisor;
    }

    if(cpuTimerFrequency)
    {
        double seconds = SecondsFromCPUTime(derivedValues[MType_CPUTimer], cpuTimerFrequency);
        derivedValues[StatValue_Seconds] = seconds;

        if(derivedValues[MType_ByteCount] > 0)
        {
            double gigabyte = (1024.0f * 1024.0f * 1024.0f);
            derivedValues[StatValue_GBPerSecond] = derivedValues[MType_ByteCount] / (gigabyte * seconds);
        }
    }

    if(derivedValues[MType_MemPageFaults] > 0)
    {
        derivedValues[StatValue_KBPerPageFault] = derivedValues[MType_ByteCount] / (derivedValues[MType_MemPageFaults] * 1024.0);
    }
}


static void PrintValue(char const *label, test_measurements value)
{
    printf("%s: %.0f", label, value.DerivedValues[MType_CPUTimer]);
    printf(" (%fms)", 1000.0f * value.DerivedValues[StatValue_Seconds]);
    if(value.DerivedValues[MType_ByteCount] > 0)
    {
        printf(" %fgb/s", value.DerivedValues[StatValue_GBPerSecond]);
    }

    if(value.DerivedValues[StatValue_KBPerPageFault])
    {
        printf(" PF: %0.4f (%0.4fkb/fault)", value.DerivedValues[MType_MemPageFaults], value.DerivedValues[StatValue_KBPerPageFault]);
    }
}


static void PrintResults(test_results results)
{
    PrintValue("Min", results.Min);
    printf("\n");
    PrintValue("Max", results.Max);
    printf("\n");
    PrintValue("Avg", results.Total);
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
        tester->Results.Min.Raw[MType_CPUTimer] = (uint64_t)-1;
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
    accumulated->Raw[MType_MemPageFaults] -= ReadOSPageFaultCount();
    accumulated->Raw[MType_CPUTimer] -= ReadCPUTimer();
}


static void EndTime(repetition_tester *tester)
{
    test_measurements *accumulated = &tester->AccumulatedOnThisTest;
    accumulated->Raw[MType_CPUTimer] += ReadCPUTimer();
    accumulated->Raw[MType_MemPageFaults] += ReadOSPageFaultCount();
    ++tester->CloseBlockCount;
}


static void CountBytes(repetition_tester *tester, uint64_t byteCount)
{
    test_measurements *accumulated = &tester->AccumulatedOnThisTest;
    accumulated->Raw[MType_ByteCount] += byteCount;
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

            if (accumulated.Raw[MType_ByteCount] != tester->TargetProcessedByteCount)
            {
                Error(tester, "Processed byte count mismatch");
            }

            if (tester->Mode == TestMode_Testing)
            {
                test_results *results = &tester->Results;

                accumulated.Raw[MType_TestCount] = 1;
                for(uint32_t i = 0; i < RT__ArrayCount(accumulated.Raw); ++i)
                {
                    results->Total.Raw[i] += accumulated.Raw[i];
                }

                if (results->Max.Raw[MType_CPUTimer] < accumulated.Raw[MType_CPUTimer])
                {
                    results->Max = accumulated;
                }

                if (results->Min.Raw[MType_CPUTimer] > accumulated.Raw[MType_CPUTimer])
                {
                    results->Min = accumulated;

                    // Note (Aaron): If we hit a new minimum, we want to continue searching so the
                    // start time is reset to current time.
                    tester->TestsStartedAt = currentTime;
                    if (tester->PrintNewMinimums)
                    {
                        ComputeDerivedValues(&results->Min, tester->CPUTimerFrequency);
                        PrintValue("Min", results->Min);
                        printf("                                   \r");
                    }
                }

                tester->OpenBlockCount = 0;
                tester->CloseBlockCount = 0;

                // Note (Aaron H): Zero out tester->AccumulatedOnThisTest
                uint64_t count = sizeof(tester->AccumulatedOnThisTest);
                void *destPtr = &tester->AccumulatedOnThisTest;
                unsigned char *dest = (unsigned char *)destPtr;
                while(count--) *dest++ = (unsigned char)0;
            }
        }

        if ((currentTime - tester->TestsStartedAt) > tester->TryForTime)
        {
            tester->Mode = TestMode_Completed;

            ComputeDerivedValues(&tester->Results.Total, tester->CPUTimerFrequency);
            ComputeDerivedValues(&tester->Results.Min, tester->CPUTimerFrequency);
            ComputeDerivedValues(&tester->Results.Max, tester->CPUTimerFrequency);

            printf("                                                          \r");
            PrintResults(tester->Results);
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
#include <psapi.h>

typedef struct tester_globals tester_globals;
struct tester_globals
{
    uint32_t Initialized;
    HANDLE ProcessHandle;
    uint64_t CPUTimerFrequency;
    uint32_t SecondsToTry;
};
static tester_globals TesterGlobals;

static void InitializeTester()
{
    if (!TesterGlobals.Initialized)
    {
        TesterGlobals.Initialized = true;
        TesterGlobals.ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
        TesterGlobals.CPUTimerFrequency = EstimateCPUTimerFrequency();
        TesterGlobals.SecondsToTry = 10;
    }
}


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
    PROCESS_MEMORY_COUNTERS_EX memoryCounters = {0};
    memoryCounters.cb = sizeof(memoryCounters);
    GetProcessMemoryInfo(TesterGlobals.ProcessHandle, (PROCESS_MEMORY_COUNTERS *)&memoryCounters, sizeof(memoryCounters));

    uint64_t result = memoryCounters.PageFaultCount;
    return result;
}

#else // _WIN32

#include <x86intrin.h>
#include <sys/time.h>
#include <sys/resource.h>


typedef struct tester_globals tester_globals;
struct tester_globals
{
    uint32_t Initialized;
    uint64_t CPUTimerFrequency;
    uint32_t SecondsToTry;
};
static tester_globals TesterGlobals;


static void InitializeTester()
{
    if (!TesterGlobals.Initialized)
    {
        TesterGlobals.Initialized = true;
        TesterGlobals.CPUTimerFrequency = EstimateCPUTimerFrequency();
        TesterGlobals.SecondsToTry = 10;
    }
}


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

