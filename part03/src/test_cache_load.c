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
extern void Load_32x8(u64 byteCount, u8 *data, u64 repeatCount);

struct test_function
{
    char const *Name;
    AsmFunction *Func;
    u64 ReadSizeBytes;
    u64 ReadOffset;
};

#define TEST_FUNCTION_ENTRY(bytes, human_readable) \
    { "load_bytes_" human_readable, Load_32x8, bytes, 0 }

// Note (Aaron): Using this macro with too small of a buffer and too large of an offset
// will result in segmentation faults.
#define TEST_FUNCTION_ENTRY_WITH_OFFSET(bytes, offset, human_readable) \
    { "load_bytes_" human_readable, Load_32x8, bytes, offset }


#if 1 // Test cache load speeds
test_function TestFunctions[] =
{
    // Note (Aaron): 256 bytes is the minimum read span for Load_32x8

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
    TEST_FUNCTION_ENTRY(Gigabytes(1), "1gb"),
};

#endif

#if 0 // Test unaligned load penalties for the L1 cache
test_function TestFunctions[] =
{
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 0, "30kb_offset_0"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 1, "30kb_offset_1"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 2, "30kb_offset_2"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 3, "30kb_offset_3"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 4, "30kb_offset_4"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 5, "30kb_offset_5"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 6, "30kb_offset_6"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 7, "30kb_offset_7"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 8, "30kb_offset_8"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 9, "30kb_offset_9"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 10, "30kb_offset_10"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 11, "30kb_offset_11"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 12, "30kb_offset_12"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 13, "30kb_offset_13"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 14, "30kb_offset_14"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 15, "30kb_offset_15"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 16, "30kb_offset_16"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 17, "30kb_offset_17"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 18, "30kb_offset_18"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 19, "30kb_offset_19"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 20, "30kb_offset_20"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 21, "30kb_offset_21"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 22, "30kb_offset_22"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 23, "30kb_offset_23"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 24, "30kb_offset_24"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 25, "30kb_offset_25"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 26, "30kb_offset_26"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 27, "30kb_offset_27"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 28, "30kb_offset_28"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 29, "30kb_offset_29"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 30, "30kb_offset_30"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 31, "30kb_offset_31"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 32, "30kb_offset_32"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 33, "30kb_offset_33"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 34, "30kb_offset_34"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 35, "30kb_offset_35"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 36, "30kb_offset_36"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 37, "30kb_offset_37"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 38, "30kb_offset_38"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 39, "30kb_offset_39"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 40, "30kb_offset_40"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 41, "30kb_offset_41"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 42, "30kb_offset_42"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 43, "30kb_offset_43"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 44, "30kb_offset_44"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 45, "30kb_offset_45"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 46, "30kb_offset_46"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 47, "30kb_offset_47"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 48, "30kb_offset_48"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 49, "30kb_offset_49"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 50, "30kb_offset_50"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 51, "30kb_offset_51"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 52, "30kb_offset_52"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 53, "30kb_offset_53"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 54, "30kb_offset_54"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 55, "30kb_offset_55"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 56, "30kb_offset_56"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 57, "30kb_offset_57"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 58, "30kb_offset_58"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 59, "30kb_offset_59"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 60, "30kb_offset_60"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 61, "30kb_offset_61"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 62, "30kb_offset_62"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 63, "30kb_offset_63"),
    TEST_FUNCTION_ENTRY_WITH_OFFSET(Kilobytes(30), 64, "30kb_offset_64"),
};
#endif


int main(void)
{
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
            testFunc.Func(testFunc.ReadSizeBytes, buff.Data + testFunc.ReadOffset, readRepeatCount);
            EndTime(tester);
            CountBytes(tester, testSizeBytes);
        }
    }

    // Note (Aaron): Output CSV data
    printf("\n--- CSV Output ---\n");
    printf("Label, Throughput (gb/s)\n");
    for (int i = 0; i < ArrayCount(testers); ++i)
    {
        test_measurements bestResult = testers[i].Results.Min;
        double gigabyte_f = (1024.0f * 1024.0f * 1024.0f);
        double bestSeconds = SecondsFromCPUTime(bestResult.Raw[MType_CPUTimer], cpuTimerFrequency);
        double bestBandwidth = bestResult.Raw[MType_ByteCount] / (gigabyte_f * bestSeconds);

        printf("%s, %f\n", TestFunctions[i].Name, bestBandwidth);
    }

    printf("\n");

    return 0;
}
