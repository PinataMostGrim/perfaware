// source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0135_multinop_loops_main.cpp

#include <sys/stat.h>
#include <stdint.h>

#include "buffer.h"
#include "buffer.c"

#define REPETITION_TESTER_IMPLEMENTATION
#include "repetition_tester.h"


#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef struct test_function test_function;
typedef void ASMFunction(uint64_t Count, uint8_t *Data);


extern void NOP3x1AllBytes(uint64_t Count, uint8_t *Data);
extern void NOP1x3AllBytes(uint64_t Count, uint8_t *Data);
extern void NOP1x9AllBytes(uint64_t Count, uint8_t *Data);


struct test_function
{
    char const *Name;
    ASMFunction *Func;
};


test_function TestFunctions[] =
{
    {"NOP3x1AllBytes", NOP3x1AllBytes},
    {"NOP1x3AllBytes", NOP1x3AllBytes},
    {"NOP1x9AllBytes", NOP1x9AllBytes},
};


int main(int argCount, char const *args[])
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
