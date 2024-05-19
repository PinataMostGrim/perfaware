#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>

#define LINUX 1
#define WRITE_TO_BUFFER 1

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

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

typedef struct test_function test_function;
typedef void AsmFunction(u64 byteCount, u8 *data, u64 repeatCount);
extern void Read_32x8(u64 byteCount, u8 *data, u64 repeatCount);

struct test_function
{
    char const *Name;
    AsmFunction *Func;
    u64 ReadSizeBytes;
};

#define TEST_FUNCTION_ENTRY(bytes, human_readable) \
    { "read_bytes_" human_readable, Read_32x8, bytes }

test_function TestFunctions[] =
{
    // Note (Aaron): 256 bytes is the minimum read span for Read_32x8

    TEST_FUNCTION_ENTRY(Kilobytes(8), "8kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(16), "16kb"),

    TEST_FUNCTION_ENTRY(Kilobytes(30), "30kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(31), "31kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(32), "32kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(33), "33kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(34), "34kb"),

    TEST_FUNCTION_ENTRY(Kilobytes(64), "64kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(128), "128kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(256), "256kb"),

    TEST_FUNCTION_ENTRY(Kilobytes(320), "320kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(384), "384kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(512), "512kb"),

    TEST_FUNCTION_ENTRY(Megabytes(1), "1mb"),
    TEST_FUNCTION_ENTRY(Megabytes(2), "2mb"),
    TEST_FUNCTION_ENTRY(Megabytes(3), "3mb"),
    TEST_FUNCTION_ENTRY(Megabytes(4), "4mb"),

    TEST_FUNCTION_ENTRY(Megabytes(8), "8mb"),
    TEST_FUNCTION_ENTRY(Megabytes(16), "16mb"),
    TEST_FUNCTION_ENTRY(Megabytes(32), "32mb"),
    TEST_FUNCTION_ENTRY(Megabytes(128), "128mb"),
};


int main(void)
{
    // Note (Aaron): The read size of each test function call must divide evenly into the buffer size.
    u64 bufferSizeBytes = Gigabytes(1);
    u64 cpuTimerFrequency = EstimateCPUTimerFrequency();

    buffer buff = BufferAllocate(bufferSizeBytes);
    if (buff.SizeBytes == 0)
    {
        fprintf(stderr, "Unable to allocate memory buffer for testing");
        return 1;
    }

#if WRITE_TO_BUFFER
    // Note (Aaron): Linux maps all allocated pages to a single page full of 0s until
    // a write is attempted. Write to each page so Linux is forced to actually commit
    // them and we pull the expected amount of memory into cache.
    for (int i = 0; i < buff.SizeBytes; i++)
    {
        *(buff.Data + i) = (u8)i;
    }
#endif

    repetition_tester testers[ArrayCount(TestFunctions)] = {0};
    for(u32 funcIndex = 0; funcIndex < ArrayCount(TestFunctions); ++funcIndex)
    {
        repetition_tester *tester = &testers[funcIndex];
        test_function testFunc = TestFunctions[funcIndex];
        u32 secondsToTry = 10;

        u64 readRepeatCount = bufferSizeBytes / testFunc.ReadSizeBytes;
        u64 testSizeBytes = testFunc.ReadSizeBytes * readRepeatCount;

        printf("\n--- %s ---\n", testFunc.Name);
        NewTestWave(tester, testSizeBytes, cpuTimerFrequency, secondsToTry);

        while(IsTesting(tester))
        {
            BeginTime(tester);
            testFunc.Func(testFunc.ReadSizeBytes, buff.Data, readRepeatCount);
            EndTime(tester);
            CountBytes(tester, testSizeBytes);
        }
    }

    // Note (Aaron): Output CSV data
    printf("\n--- CSV Output ---\n");
    printf("Label, Throughput (gb/s)\n");
    for (int i = 0; i < ArrayCount(testers); ++i)
    {
        repetition_value bestResult = testers[i].Results.Min;
        double gigabyte_f = (1024.0f * 1024.0f * 1024.0f);
        double bestSeconds = SecondsFromCPUTime(bestResult.E[RepValue_CPUTimer], cpuTimerFrequency);
        double bestBandwidth = bestResult.E[RepValue_ByteCount] / (gigabyte_f * bestSeconds);

        printf("%s, %f\n", TestFunctions[i].Name, bestBandwidth);
    }

    printf("\n");

    return 0;
}
