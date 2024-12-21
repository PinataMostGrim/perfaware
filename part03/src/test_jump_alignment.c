// source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0140_jump_alignment_main.cpp

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>

#include "buffer.h"
#include "buffer.c"

#define REPETITION_TESTER_IMPLEMENTATION
#include "repetition_tester.h"


#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef struct test_function test_function;
typedef void ASMFunction(uint64_t Count, uint8_t *Data);


extern void NOPAligned64(uint64_t Count, uint8_t *Data);
extern void NOPAligned1(uint64_t Count, uint8_t *Data);
extern void NOPAligned15(uint64_t Count, uint8_t *Data);
extern void NOPAligned31(uint64_t Count, uint8_t *Data);
extern void NOPAligned63(uint64_t Count, uint8_t *Data);


struct test_function
{
    char const *Name;
    ASMFunction *Func;
};


test_function TestFunctions[] =
{
    {"NOPAligned64", NOPAligned64},
    {"NOPAligned1", NOPAligned1},
    {"NOPAligned15", NOPAligned15},
    {"NOPAligned31", NOPAligned31},
    {"NOPAligned63", NOPAligned63},
};


int main(void)
{
    uint64_t cpuTimerFrequency = EstimateCPUTimerFrequency();

    buffer buff = BufferAllocate(1*1024*1024*1024);
    if (buff.SizeBytes == 0)
    {
        fprintf(stderr, "Unable to allocate memory buffer for testing");
        return 1;
    }

    repetition_tester testers[ArrayCount(TestFunctions)] = {0};
    for(;;)
    {
        for(uint32_t funcIndex = 0; funcIndex < ArrayCount(TestFunctions); ++funcIndex)
        {
            repetition_tester *tester = &testers[funcIndex];
            test_function testFunc = TestFunctions[funcIndex];
            uint32_t secondsToTry = 10;

            printf("\n--- %s ---\n", testFunc.Name);
            NewTestWave(tester, buff.SizeBytes, cpuTimerFrequency, secondsToTry);

            while(IsTesting(tester))
            {
                BeginTime(tester);
                testFunc.Func(buff.SizeBytes, buff.Data);
                EndTime(tester);
                CountBytes(tester, buff.SizeBytes);
            }
        }
    }

    return 0;
}
