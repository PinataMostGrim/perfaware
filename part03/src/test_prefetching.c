// source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0162_prefetching_main.cpp
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>

#define LINUX 1

#include "tester_common.h"
#include "tester_common.c"
#include "../../common/src/buffer.h"
#include "../../common/src/buffer.c"

#define REPETITION_TESTER_IMPLEMENTATION
#include "../../common/src/repetition_tester.h"

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t b32;

typedef float f32;
typedef double f64;

#define Kilobytes(Value) ((Value) * 1024LL)
#define Megabytes(Value) (Kilobytes(Value) * 1024LL)
#define Gigabytes(Value) (Megabytes(Value) * 1024LL)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef struct test_function test_function;
typedef void AsmFunction(u64 outerLoopCount, u8 *data, u64 innerLoopCount);

extern void PeriodicRead(u64 outerLoopCount, u8 *data, u64 innerLoopCount);
extern void PeriodicPrefetchedRead(u64 outerLoopCount, u8 *data, u64 innerLoopCount);

struct test_function
{
    char const *Name;
    AsmFunction *Func;
};

test_function TestFunctions[] =
{
    {"PeriodicRead", PeriodicRead},
    {"PeriodicPrefetchedRead", PeriodicPrefetchedRead},
};

int main(void)
{
    u64 cpuTimerFrequency = EstimateCPUTimerFrequency();

    repetition_tester testers[32][ArrayCount(TestFunctions)] = {0};
    u64 innerLoopCounts[ArrayCount(testers)] = {0};
    // TODO: Why did he use a #define here instead?
    u64 outerLoopCount = 1024 * 1024;
    u64 cacheLineSize = 64;

    buffer buff = BufferAllocate(Gigabytes(1));
    if (buff.SizeBytes == 0)
    {
        fprintf(stderr, "Unable to allocate memory buffer for testing");
        return 1;
    }

    u64 testSize = outerLoopCount * cacheLineSize;

    // Note: Initialize the buffer to a random jump pattern
    u64 cacheLineCount = buff.SizeBytes / cacheLineSize;
    u64 jumpOffset = 0;
    for (u64 outerLoopIndex = 0; outerLoopIndex < outerLoopCount; ++outerLoopIndex)
    {
        // Note: Make sure we don't get a collision in the random offsets
        u64 nextOffset = 0;
        u64 *nextPointer = 0;

        u64 randomValue = 0;
        ReadOSRandomBytes(sizeof(randomValue), &randomValue);
        b32 found = 0;
        for (u64 searchIndex = 0; searchIndex < cacheLineCount; ++searchIndex)
        {
            nextOffset = (randomValue + searchIndex) % cacheLineCount;
            nextPointer = (u64 *)(buff.Data + nextOffset*cacheLineSize);
            if (*nextPointer == 0)
            {
                found = 1;
                break;
            }
        }

        if (!found)
        {
            fprintf(stderr, "ERROR: Unable to create a single continuous pointer chain.\n");
        }

        // Note: Write the next pointer and some "data" to the cache line.
        u64*jumpData = (u64 *)(buff.Data + jumpOffset * cacheLineSize);
        // TODO (Aaron): Why is nextPointer in parenthesis?
        jumpData[0] = (u64)(nextPointer);
        jumpData[1] = outerLoopIndex;

        jumpOffset = nextOffset;
    }

    for (u64 innerLoopIndex = 0; innerLoopIndex < ArrayCount(testers); ++innerLoopIndex)
    {
        u64 innerLoopCount = 4 * (innerLoopIndex + 1);
        if (innerLoopIndex >= 16)
        {
            // TODO (Aaron): Why is the right half of the statement enclosed in parenthesis?
            innerLoopCount = (64 * (innerLoopIndex - 14));
        }

        innerLoopCounts[innerLoopIndex] = innerLoopCount;

        for (u32 functionIndex = 0; functionIndex < ArrayCount(TestFunctions); ++functionIndex)
        {
            repetition_tester *tester = &testers[innerLoopIndex][functionIndex];
            u32 secondsToTry = 10;
            test_function testFunction = TestFunctions[functionIndex];

            printf("\n--- %s (%lu inner loop iterations)---\n", testFunction.Name, innerLoopCount);
            NewTestWave(tester, testSize, cpuTimerFrequency, secondsToTry);

            while (IsTesting(tester))
            {
                BeginTime(tester);
                testFunction.Func(outerLoopCount, buff.Data, innerLoopCount);
                EndTime(tester);
                CountBytes(tester, testSize);
            }
        }
    }

    printf("InnerLoopCount");
    for (u32 functionIndex = 0; functionIndex < ArrayCount(TestFunctions); ++functionIndex)
    {
        printf(", %s", TestFunctions[functionIndex].Name);
    }
    printf("\n");

    for (u64 innerLoopIndex = 0; innerLoopIndex < ArrayCount(testers); ++innerLoopIndex)
    {
        printf("%lu", innerLoopCounts[innerLoopIndex]);
        for (u32 functionIndex = 0; functionIndex < ArrayCount(TestFunctions); ++functionIndex)
        {
            repetition_tester *tester = &testers[innerLoopIndex][functionIndex];

            test_measurements value = tester->Results.Min;
            f64 seconds = SecondsFromCPUTime((f64)value.E[MType_CPUTimer], tester->CPUTimerFrequency);
            f64 gigabyte = (f64)(Gigabytes(1));
            f64 bandwidth = value.E[MType_ByteCount] / (gigabyte * seconds);

            printf(",%f", bandwidth);
        }
        printf("\n");
    }

    return 0;
}
