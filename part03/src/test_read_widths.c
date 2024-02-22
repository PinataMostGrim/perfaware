// source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0151_read_widths_main.cpp

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>

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

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

typedef struct test_function test_function;
typedef void ASMFunction(u64 Count, u8 *Data);

extern void Read_4x2(u64 Count, u8 *Data);
extern void Read_8x2(u64 Count, u8 *Data);
extern void Read_16x2(u64 Count, u8 *Data);
extern void Read_32x2(u64 Count, u8 *Data);

struct test_function
{
    char const *Name;
    ASMFunction *Func;
};

test_function TestFunctions[] =
{
    {"Read_4x2", Read_4x2},
    {"Read_8x2", Read_8x2},
    {"Read_16x2", Read_16x2},
    {"Read_32x2", Read_32x2},
};

int main(void)
{
    u64 cpuTimerFrequency = EstimateCPUTimerFrequency();

    buffer buff = BufferAllocate(1*1024*1024*1024);
    if (buff.SizeBytes == 0)
    {
        fprintf(stderr, "Unable to allocate memory buffer for testing");
        return 1;
    }

    repetition_tester Testers[ArrayCount(TestFunctions)] = {};
    for(;;)
    {
        for(u32 FuncIndex = 0; FuncIndex < ArrayCount(TestFunctions); ++FuncIndex)
        {
            repetition_tester *Tester = &Testers[FuncIndex];
            test_function TestFunc = TestFunctions[FuncIndex];
            u32 secondsToTry = 10;

            printf("\n--- %s ---\n", TestFunc.Name);
            NewTestWave(Tester, buff.SizeBytes, cpuTimerFrequency, secondsToTry);

            while(IsTesting(Tester))
            {
                BeginTime(Tester);
                TestFunc.Func(buff.SizeBytes, buff.Data);
                EndTime(Tester);
                CountBytes(Tester, buff.SizeBytes);
            }
        }
    }

    return 0;
}
