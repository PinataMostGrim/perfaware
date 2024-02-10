// source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0133_front_end_test_main.cpp

#include <sys/stat.h>
#include <stdint.h>

#include "tester_common.h"

#include "../../common/src/buffer.h"
#include "../../common/src/buffer.c"
#include "tester_common.c"

#define REPETITION_TESTER_IMPLEMENTATION
#include "../../common/src/repetition_tester.h"


#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))


typedef struct test_function test_function;


static void WriteToAllBytes(repetition_tester *tester, read_parameters *params);

static void MOVAllBytes(repetition_tester *tester, read_parameters *params);
static void NOPAllBytes(repetition_tester *tester, read_parameters *params);
static void CMPAllBytes(repetition_tester *tester, read_parameters *params);
static void DECAllBytes(repetition_tester *tester, read_parameters *params);

extern void MOVAllBytesASM(uint64_t Count, uint8_t *Data);
extern void NOPAllBytesASM(uint64_t Count, uint8_t *Data);
extern void CMPAllBytesASM(uint64_t Count, uint8_t *Data);
extern void DECAllBytesASM(uint64_t Count, uint8_t *Data);


struct test_function
{
    char const *Name;
    read_overhead_test_func *Func;
};


test_function TestFunctions[] =
{
    {"WriteToAllBytes", WriteToAllBytes },
    {"MOVAllBytes", MOVAllBytes},
    {"NOPAllBytesASM", NOPAllBytes},
    {"CMPAllBytesASM", CMPAllBytes},
    {"DECAllBytesASM", DECAllBytes},
};


static void WriteToAllBytes(repetition_tester *tester, read_parameters *params)
{
    while (IsTesting(tester))
    {
        buffer buff = params->Buffer;
        HandleAllocation(params, &buff);

        BeginTime(tester);
        for (uint64_t index = 0; index < buff.SizeBytes; ++index)
        {
            buff.Data[index] = (uint8_t)index;
        }
        EndTime(tester);

        CountBytes(tester, buff.SizeBytes);
        HandleDeallocation(params, &buff);
    }
}


static void MOVAllBytes(repetition_tester *tester, read_parameters *params)
{
    while (IsTesting(tester))
    {
        buffer buff = params->Buffer;
        HandleAllocation(params, &buff);

        BeginTime(tester);
        MOVAllBytesASM(buff.SizeBytes, buff.Data);
        EndTime(tester);

        CountBytes(tester, buff.SizeBytes);
        HandleDeallocation(params, &buff);
    }
}


static void NOPAllBytes(repetition_tester *tester, read_parameters *params)
{
    while (IsTesting(tester))
    {
        buffer buff = params->Buffer;
        HandleAllocation(params, &buff);

        BeginTime(tester);
        NOPAllBytesASM(buff.SizeBytes, buff.Data);
        EndTime(tester);

        CountBytes(tester, buff.SizeBytes);
        HandleDeallocation(params, &buff);
    }
}


static void CMPAllBytes(repetition_tester *tester, read_parameters *params)
{
    while (IsTesting(tester))
    {
        buffer buff = params->Buffer;
        HandleAllocation(params, &buff);

        BeginTime(tester);
        CMPAllBytesASM(buff.SizeBytes, buff.Data);
        EndTime(tester);

        CountBytes(tester, buff.SizeBytes);
        HandleDeallocation(params, &buff);
    }
}


static void DECAllBytes(repetition_tester *tester, read_parameters *params)
{
    while (IsTesting(tester))
    {
        buffer buff = params->Buffer;
        HandleAllocation(params, &buff);

        BeginTime(tester);
        DECAllBytesASM(buff.SizeBytes, buff.Data);
        EndTime(tester);

        CountBytes(tester, buff.SizeBytes);
        HandleDeallocation(params, &buff);
    }
}


int main(int argCount, char const *args[])
{
    uint64_t cpuTimerFrequency = EstimateCPUTimerFrequency();

    if (argCount != 2)
    {
        fprintf(stderr, "Usage: %s [existing filename]\n", args[0]);
        return 0;
    }

    const char *fileName = args[1];
#if _WIN32
    struct __stat64 stat;
    _stat64(fileName, &stat);
#else
    struct stat stats;
    stat(fileName, &stats);
#endif

    read_parameters params = {0};
    params.FileName = fileName;
    params.Buffer = BufferAllocate(stats.st_size);
    if (params.Buffer.SizeBytes == 0)
    {
        fprintf(stderr, "[ERROR] Test data size must be non-zero\n");
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

            printf("\n--- %s%s%s ---\n",
                   DescribeAllocationType(params.AllocType),
                   params.AllocType ? " + " : "",
                   testFunc.Name);
            NewTestWave(tester, params.Buffer.SizeBytes, cpuTimerFrequency, secondsToTry);
            testFunc.Func(tester, &params);
        }
    }

    return 0;
}
