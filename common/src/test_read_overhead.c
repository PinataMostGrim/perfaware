#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <stdlib.h>

#include <unistd.h>
#include <fcntl.h>

#include "base_inc.h"
#include "base_types.c"
#include "base_memory.c"

#define REPETITION_TESTER_IMPLEMENTATION
#include "repetition_tester.h"


typedef struct read_parameters read_parameters;
typedef struct test_function test_function;
typedef void read_overhead_test_func(repetition_tester *tester, read_parameters *params);

static void ReadViaFRead(repetition_tester *tester, read_parameters *params);
static void ReadViaRead(repetition_tester *tester, read_parameters *params);


struct read_parameters
{
    memory_arena Arena;
    char const *FileName;
};

struct test_function
{
    char const *Name;
    read_overhead_test_func *Func;
};

test_function testFunctions[] =
{
    {"fread", ReadViaFRead },
    {"read", ReadViaRead },
};


static void ReadViaFRead(repetition_tester *tester, read_parameters *params)
{
    while(IsTesting(tester))
    {
        FILE *file = fopen(params->FileName, "rb");
        if (!file)
        {
            Error(tester, "fopen failed");
            continue;
        }

        memory_arena arena = params->Arena;

        BeginTime(tester);
        size_t result = fread(arena.BasePtr, arena.Size, 1, file);
        EndTime(tester);

        if (result == 1)
        {
            CountBytes(tester, arena.Size);
        }
        else
        {
            Error(tester, "fread failed");
        }

        fclose(file);
    }
}


static void ReadViaRead(repetition_tester *tester, read_parameters *params)
{
    while (IsTesting(tester))
    {
        int file = open(params->FileName, O_RDONLY);
        if (file == -1)
        {
            Error(tester, "open failed");
            continue;
        }

        memory_arena arena = params->Arena;
        rt__u8 *dest = arena.BasePtr;
        rt__u64 sizeRemaining = arena.Size;

        while(sizeRemaining)
        {
            rt__u32 readSize = INT32_MAX;
            if ((rt__u64)readSize > sizeRemaining)
            {
                readSize = (rt__u32)sizeRemaining;
            }

            BeginTime(tester);
            int result = read(file, dest, readSize);
            EndTime(tester);

            if (result != (int)readSize)
            {
                Error(tester, "read failed");
                break;
            }

            CountBytes(tester, readSize);
            sizeRemaining -= readSize;
        }

        close(file);
    }
}


static memory_arena ArenaAllocate(memory_index size)
{
    memory_arena result = {0};
    U8 *basePtr = (U8 *)malloc(size);
    if (!basePtr)
    {
        return result;
    }

    ArenaInitialize(&result, size, basePtr);

    return result;
}


static void ArenaFree(memory_arena *arena)
{
    if(arena)
    {
        free(arena->BasePtr);
    }

    MemoryZeroStruct(arena);
}


int main(int argCount, char const *args[])
{
    rt__u64 cpuTimerFrequency = EstimateCPUTimerFrequency();

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
    params.Arena = ArenaAllocate(stats.st_size);
    params.FileName = fileName;

    if (params.Arena.Size == 0)
    {
        fprintf(stderr, "[ERROR] Test data size must be non-zero\n");
        return 1;
    }

    repetition_tester testers[ArrayCount(testFunctions)] = {0};

    for(;;)
    {
        for(rt__u32 funcIndex = 0; funcIndex < ArrayCount(testFunctions); ++funcIndex)
        {
            repetition_tester *tester = testers + funcIndex;
            test_function testFunc = testFunctions[funcIndex];
            rt__u32 secondsToTry = 10;

            printf("\n--- %s ---\n", testFunc.Name);
            NewTestWave(tester, params.Arena.Size, cpuTimerFrequency, secondsToTry);
            testFunc.Func(tester, &params);
        }
    }

    return 0;
}
