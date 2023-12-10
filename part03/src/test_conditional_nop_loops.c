// source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0138_conditional_nop_loops_main.cpp

#include <stdint.h>

#include "tester_common.h"
#include "../../common/src/buffer.h"

#define REPETITION_TESTER_IMPLEMENTATION
#include "../../common/src/repetition_tester.h"

#include "../../common/src/buffer.c"
#include "tester_common.c"


#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))


typedef struct test_function test_function;
typedef void ASMFunction(uint64_t Count, uint8_t *Data);

extern void ConditionalNOP(uint64_t Count, uint8_t *Data);


struct test_function
{
    char const *Name;
    ASMFunction *Func;
};

typedef enum
{
    BranchPattern_NeverTaken,
    BranchPattern_AlwaysTaken,
    BranchPattern_Every2,
    BranchPattern_Every3,
    BranchPattern_Every4,
    BranchPattern_CRTRandom,
    BranchPattern_OSRandom,

    BranchPattern_Count,
} branch_pattern;

test_function TestFunctions[] =
{
    {"ConditionalNOP", ConditionalNOP}
};


static char const *FillWithBranchPattern(branch_pattern pattern, buffer buff)
{
    char const *patternName = "UNKNOWN";

    if (pattern == BranchPattern_OSRandom)
    {
        patternName = "OSRandom";
        FillWithRandomBytes(buff);

        return patternName;
    }

    for (int index = 0; index < buff.SizeBytes; ++index)
    {
        uint8_t value = 0;

        switch (pattern)
        {
            case BranchPattern_NeverTaken:
            {
                patternName = "Never Taken";
                value = 0;
                break;
            }

            case BranchPattern_AlwaysTaken:
            {
                patternName = "Always Taken";
                value = 1;
                break;
            }
            case BranchPattern_Every2:
            {
                patternName = "Every 2";
                value = ((index % 2) == 0);
                break;
            }
            case BranchPattern_Every3:
            {
                patternName = "Every 3";
                value = ((index % 3) == 0);
                break;
            }
            case BranchPattern_Every4:
            {
                patternName = "Every 4";
                value = ((index % 4) == 0);
                break;
            }
            case BranchPattern_CRTRandom:
            {
                patternName = "CRTRandom";
                value = (uint8_t)rand();
                break;
            }

            default:
            {
                fprintf(stderr, "Unrecognized branch pattern.\n");
                break;
            }
        }

        buff.Data[index] = value;
    }

    return patternName;
}


int main()
{
    uint64_t cpuTimerFrequency = EstimateCPUTimerFrequency();
    buffer buff = BufferAllocate(1*1024*1024*1024);
    if (buff.SizeBytes == 0)
    {
        fprintf(stderr, "Unable to allocate memory buffer for testing");
        return 1;
    }

    repetition_tester testers[BranchPattern_Count][ArrayCount(TestFunctions)] = {0};
    for(;;)
    {
        for(uint32_t pattern = 0; pattern < BranchPattern_Count; ++pattern)
        {
            char const *patternName = FillWithBranchPattern((branch_pattern)pattern, buff);

            for (int funcIndex = 0; funcIndex < ArrayCount(TestFunctions); ++funcIndex)
            {
                repetition_tester *tester = &testers[pattern][funcIndex];
                test_function testFunc = TestFunctions[funcIndex];
                uint32_t secondsToTry = 10;

                printf("\n--- %s, %s ---\n", testFunc.Name, patternName);
                NewTestWave(tester, buff.SizeBytes, cpuTimerFrequency, secondsToTry);

                while (IsTesting(tester))
                {
                    BeginTime(tester);
                    testFunc.Func(buff.SizeBytes, buff.Data);
                    EndTime(tester);
                    CountBytes(tester, buff.SizeBytes);
                }

            }
        }
    }

    return 0;
}
