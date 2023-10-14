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
#include "buffer.h"
#include "buffer.c"

#define REPETITION_TESTER_IMPLEMENTATION
#include "repetition_tester.h"


typedef struct read_parameters read_parameters;
typedef struct test_function test_function;
typedef void read_overhead_test_func(repetition_tester *tester, read_parameters *params);

static void WriteToAllBytes(repetition_tester *tester, read_parameters *params);
static void ReadViaFRead(repetition_tester *tester, read_parameters *params);
static void ReadViaRead(repetition_tester *tester, read_parameters *params);


typedef enum
{
    AllocType_none,
    AllocType_malloc,

    AllocType_Count,
} allocation_type;


struct read_parameters
{
    char const *FileName;
    buffer Buffer;
    allocation_type AllocType;
};

struct test_function
{
    char const *Name;
    read_overhead_test_func *Func;
};

test_function testFunctions[] =
{
    {"WriteToAllBytes", WriteToAllBytes },
    {"fread", ReadViaFRead },
    {"read", ReadViaRead },
};


static char const *DescribeAllocationType(allocation_type allocType)
{
    char const *result;
    switch(allocType)
    {
        case AllocType_none: { result = ""; break; }
        case AllocType_malloc: { result = "malloc"; break; }
        default: { result = "UNKNOWN"; break; }
    }

    return result;
}


static void HandleAllocation(read_parameters *params, buffer *buff)
{
    switch(params->AllocType)
    {
        case AllocType_none:
        {
            break;
        }
        case AllocType_malloc:
        {
            *buff = BufferAllocate(params->Buffer.SizeBytes);
            break;
        }

        default:
        {
            fprintf(stderr, "[ERROR] Unrecognized allocation type");
            break;
        }
    }
}


static void HandleDeallocation(read_parameters *params, buffer *buff)
{
    switch(params->AllocType)
    {
        case AllocType_none:
        {
            break;
        }
        case AllocType_malloc:
        {
            BufferFree(buff);
            break;
        }

        default:
        {
            fprintf(stderr, "[ERROR] Unrecognized allocation type");
            break;
        }
    }
}


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

        buffer buff = params->Buffer;
        HandleAllocation(params, &buff);

        BeginTime(tester);
        size_t result = fread(buff.Data, buff.SizeBytes, 1, file);
        EndTime(tester);

        if (result == 1)
        {
            CountBytes(tester, buff.SizeBytes);
        }
        else
        {
            Error(tester, "fread failed");
        }

        HandleDeallocation(params, &buff);
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

        buffer buff = params->Buffer;
        HandleAllocation(params, &buff);

        uint8_t *dest = buff.Data;
        uint64_t sizeRemaining = buff.SizeBytes;
        while(sizeRemaining)
        {
            uint32_t readSize = INT32_MAX;
            if ((uint64_t)readSize > sizeRemaining)
            {
                readSize = (uint32_t)sizeRemaining;
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

        HandleDeallocation(params, &buff);
        close(file);
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

    repetition_tester testers[ArrayCount(testFunctions)][AllocType_Count] = {0};

    for(;;)
    {
        for(uint32_t funcIndex = 0; funcIndex < ArrayCount(testFunctions); ++funcIndex)
        {
            for(uint32_t allocType = 0; allocType < AllocType_Count; ++allocType)
            {
                params.AllocType = (allocation_type)allocType;

                repetition_tester *tester = &testers[funcIndex][allocType];
                test_function testFunc = testFunctions[funcIndex];
                uint32_t secondsToTry = 10;

                printf("\n--- %s%s%s ---\n",
                       DescribeAllocationType(params.AllocType),
                       params.AllocType ? " + " : "",
                       testFunc.Name);
                NewTestWave(tester, params.Buffer.SizeBytes, cpuTimerFrequency, secondsToTry);
                testFunc.Func(tester, &params);
            }
        }
    }

    return 0;
}
