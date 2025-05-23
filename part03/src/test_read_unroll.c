// source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0145_read_unroll_main.cpp


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>

#include "buffer.h"
#include "buffer.c"

#define REPETITION_TESTER_IMPLEMENTATION
#include "repetition_tester.h"

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t b32;

typedef float f32;
typedef double f64;

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

typedef struct test_function test_function;
typedef void ASMFunction(u64 Count, u8 *Data);

extern void Read_x1(uint64_t Count, uint8_t *Data);
extern void Read_x2(uint64_t Count, uint8_t *Data);
extern void Read_x3(uint64_t Count, uint8_t *Data);
extern void Read_x4(uint64_t Count, uint8_t *Data);

struct test_function
{
    char const *Name;
    ASMFunction *Func;
};


test_function TestFunctions[] =
{
    {"Read_x1", Read_x1},
    {"Read_x2", Read_x2},
    {"Read_x3", Read_x3},
    {"Read_x4", Read_x4},
};

int main(void)
{
    u64 cpuTimerFrequency = EstimateCPUTimerFrequency();
    u64 RepeatCount = 1024*1024*1024ull;

    buffer buff = BufferAllocate(4096);
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
            NewTestWave(Tester, RepeatCount, cpuTimerFrequency, secondsToTry);

            while(IsTesting(Tester))
            {
                BeginTime(Tester);
                TestFunc.Func(RepeatCount, buff.Data);
                EndTime(Tester);
                CountBytes(Tester, RepeatCount);
            }
        }
    }

    return 0;
}
