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
typedef void AsmFunction(u64 byteCount, u8 *data, u64 repeatCount, u64 loadOffset);

extern void Load_32x2(u64 byteCount, u8 *data, u64 repeatCount, u64 stride);

struct test_function
{
    char const *Name;
    AsmFunction *Func;
    u64 LoadSizeBytes;
    u64 LoadOffset;
};


int main(void)
{
    u64 cpuTimerFrequency = EstimateCPUTimerFrequency();

    repetition_tester testers[128] = {0};
    u64 cacheLineSize = 64;
    u64 loopCount = 64;
    u64 readCount = 256;
    u64 totalBytes = loopCount * readCount * cacheLineSize;

    buffer buff = BufferAllocate(totalBytes);
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

    for (u64 strideIndex = 0; strideIndex < ArrayCount(testers); ++strideIndex)
    {
        repetition_tester *tester = testers + strideIndex;
        u32 secondsToTry = 10;
        u64 stride = strideIndex * cacheLineSize;

        printf("\n--- Load_32x2 of %lu lines spaced %lu bytes apart (total span: %lu) ---\n",
               readCount, stride, readCount * stride);
        NewTestWave(tester, totalBytes, cpuTimerFrequency, secondsToTry);
        while(IsTesting(tester))
        {
            BeginTime(tester);
            Load_32x2(readCount, buff.Data, loopCount, stride);
            EndTime(tester);
            CountBytes(tester, totalBytes);
        }
    }

    // Note (Aaron): Output CSV data
    printf("\n--- CSV Output ---\n");
    printf("Stride, Throughput (gb/s)\n");
    for (u64 strideIndex = 0; strideIndex < ArrayCount(testers); ++strideIndex)
    {
        test_measurements bestResult = testers[strideIndex].Results.Min;

        double gigabyte_f = (1024.0f * 1024.0f * 1024.0f);
        double bestSeconds = SecondsFromCPUTime(bestResult.E[MType_CPUTimer], cpuTimerFrequency);
        double bestBandwidth = bestResult.E[MType_ByteCount] / (gigabyte_f * bestSeconds);

        u64 stride = cacheLineSize * strideIndex;

        printf("%lu, %f\n", stride, bestBandwidth);
    }

    printf("\n");

    return 0;
}
