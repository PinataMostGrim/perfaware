// source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0168_osread_sum_main.cpp

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "tester_common.h"

#include "../../common/src/buffer.h"
#include "../../common/src/buffer.c"
#include "tester_common.c"

#define REPETITION_TESTER_IMPLEMENTATION
#include "../../common/src/repetition_tester.h"

typedef float f32;
typedef double f64;
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define EXCESSIVE_FENCE _mm_mfence()

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t b32;

typedef struct test_function test_function;
typedef u64 file_process_func(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize);

static u64 OpenAllocateAndFRead(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize);
static u64 OpenAllocateAndSum(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize);
static u64 OpenAllocateAndSumOverlapped(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize);


struct test_function
{
    char const *Name;
    file_process_func *Func;
};

test_function TestFunctions[] =
{
    { "OpenAllocateAndFRead", OpenAllocateAndFRead},
    { "OpenAllocateAndSum", OpenAllocateAndSum},
    { "OpenAllocateAndSumOverlapped", OpenAllocateAndSumOverlapped},
};


static u64 Sum64s(u64 dataSize, void *data)
{
    u64 *source = (u64 *)data;
    u64 sum0 = 0;
    u64 sum1 = 0;
    u64 sum2 = 0;
    u64 sum3 = 0;
    u64 sumCount = dataSize / (4*8);
    while(sumCount--)
    {
        sum0 += source[0];
        sum1 += source[1];
        sum2 += source[2];
        sum3 += source[3];
        source += 4;
    }

    u64 result = sum0 + sum1 + sum2 + sum3;

    return result;
}


static u64 OpenAllocateAndFRead(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize)
{
    u64 result = 0;

    FILE *file = fopen(fileName, "rb");
    buffer buff = BufferAllocate(bufferSize);
    if (file && BufferIsValid(buff))
    {
        u64 sizeRemaining = totalFileSize;
        while (sizeRemaining)
        {
            u64 readSize = buff.SizeBytes;
            if (readSize > sizeRemaining)
            {
                readSize = sizeRemaining;
            }

            if (fread(buff.Data, readSize, 1, file) == 1)
            {
                CountBytes(tester, readSize);
            }
            else
            {
                Error(tester, "fread failed");
            }

            sizeRemaining -= readSize;
        }
    }
    else
    {
        Error(tester, "Couldn't acquire resources");
    }

    BufferFree(&buff);
    fclose(file);

    return result;
}


static u64 OpenAllocateAndSum(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize)
{
    u64 result = 0;

    FILE *file = fopen(fileName, "rb");
    buffer buff = BufferAllocate(bufferSize);
    if (file && BufferIsValid(buff))
    {
        u64 sizeRemaining = totalFileSize;
        while (sizeRemaining)
        {
            u64 readSize = buff.SizeBytes;
            if(readSize > sizeRemaining)
            {
                readSize = sizeRemaining;
            }

            if(fread(buff.Data, readSize, 1, file) == 1)
            {
                result += Sum64s(readSize, buff.Data);
                CountBytes(tester, readSize);
            }
            else
            {
                Error(tester, "fread failed");
            }

            sizeRemaining -= readSize;
        }
    }
    else
    {
        Error(tester, "Couldn't acquire resources");
    }

    BufferFree(&buff);
    fclose(file);

    return result;
}


typedef enum
{
    Buffer_Unused,
    Buffer_ReadCompleted,
} overlapped_buffer_state;


typedef struct overlapped_buffer overlapped_buffer;
struct overlapped_buffer
{
    buffer Value;
    volatile u64 ReadSize;
    volatile overlapped_buffer_state State;
};


typedef struct threaded_io threaded_io;
struct threaded_io
{
    overlapped_buffer Buffers[2];
    u64 TotalFileSize;
    FILE *File;
    b32 ReadError;
};


THREAD_ENTRY_POINT(IOThreadRoutine, arg)
{
    threaded_io *threadedIO = (threaded_io *)arg;

    FILE *file = threadedIO->File;
    u32 bufferIndex = 0;
    u64 sizeRemaining = threadedIO->TotalFileSize;
    while(sizeRemaining)
    {
        overlapped_buffer *buff = &threadedIO->Buffers[bufferIndex++ & 1];
        u64 readSize = buff->Value.SizeBytes;
        if(readSize > sizeRemaining)
        {
            readSize = sizeRemaining;
        }

        while(buff->State != Buffer_Unused) {_mm_pause();}

        EXCESSIVE_FENCE;

        if(fread(buff->Value.Data, readSize, 1, file) != 1)
        {
            threadedIO->ReadError = true;
        }

        buff->ReadSize = readSize;

        EXCESSIVE_FENCE;

        buff->State = Buffer_ReadCompleted;

        sizeRemaining -= readSize;
    }

    return 0;
}


static u64 OpenAllocateAndSumOverlapped(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize)
{
    u64 result = 0;

    threaded_io threadedIO = {0};
    threadedIO.File = fopen(fileName, "rb");
    threadedIO.TotalFileSize = totalFileSize;
    threadedIO.Buffers[0].Value = BufferAllocate(bufferSize);
    threadedIO.Buffers[1].Value = BufferAllocate(bufferSize);

    thread_handle ioThread = {0};
    if(threadedIO.File
       && BufferIsValid(threadedIO.Buffers[0].Value)
       && BufferIsValid(threadedIO.Buffers[1].Value))
    {
        ioThread = CreateAndStartThread(IOThreadRoutine, &threadedIO);
    }

    if(ThreadIsValid(ioThread))
    {
        u64 bufferIndex = 0;
        u64 sizeRemaining = totalFileSize;
        while(sizeRemaining)
        {
            overlapped_buffer *buff = &threadedIO.Buffers[bufferIndex++ & 1];

            while(buff->State != Buffer_ReadCompleted) {_mm_pause();}

            EXCESSIVE_FENCE;

            u64 readSize = buff->ReadSize;
            result += Sum64s(readSize, buff->Value.Data);
            CountBytes(tester, readSize);

            EXCESSIVE_FENCE;

            buff->State = Buffer_Unused;

            sizeRemaining -= readSize;
        }

        if(threadedIO.ReadError)
        {
            Error(tester, "fread failed");
        }
    }
    else
    {
        Error(tester, "Couldn't acquire resources");
    }

    BufferFree(&threadedIO.Buffers[0].Value);
    BufferFree(&threadedIO.Buffers[1].Value);
    fclose(threadedIO.File);

    return result;
}


int main(int argCount, char const *args[])
{
    if (argCount != 2)
    {
        fprintf(stderr, "Usage: %s [existing filename]\n", args[0]);
        return 0;
    }

    InitializeTester();

    const char *fileName = args[1];
    buffer buff = ReadEntireFile((char *)fileName);
    test_series testSeries = TestSeriesAllocate(ArrayCount(TestFunctions), 1024);

    if (!BufferIsValid(buff) || !TestSeriesIsValid(testSeries))
    {
        fprintf(stderr, "[ERROR]: Test data size must be non-zero\n");
        BufferFree(&buff);
        TestSeriesFree(&testSeries);

        return 1;
    }

    u64 fileSize = buff.SizeBytes;
    u64 referenceSum = Sum64s(buff.SizeBytes, buff.Data);

    SetRowLabelLabel(&testSeries, "ReadBufferSize");
    for(u64 readBufferSize = 256*1024; readBufferSize < buff.SizeBytes; readBufferSize*=2)
    {
        SetRowLabel(&testSeries, "%lluk", readBufferSize/1024);
        for(u32 testFunctionIndex = 0; testFunctionIndex < ArrayCount(TestFunctions); ++testFunctionIndex)
        {
            test_function function = TestFunctions[testFunctionIndex];

            SetColumnLabel(&testSeries, "%s", function.Name);

            repetition_tester tester = {0};
            TestSeriesNewTestWave(&testSeries, &tester, fileSize, TesterGlobals.CPUTimerFrequency, TesterGlobals.SecondsToTry);

            b32 passed = true;
            while(TestSeriesIsTesting(&testSeries, &tester))
            {
                BeginTime(&tester);
                u64 check = function.Func(&tester, fileName, fileSize, readBufferSize);
                if(check != referenceSum)
                {
                    passed = false;
                }
                EndTime(&tester);
            }

            if (!passed)
            {
                fprintf(stderr, "WARNING: Checksum mismatch\n");
            }
        }
    }

    PrintCSVForValue(&testSeries, StatValue_GBPerSecond, stdout, 1.0f);

    BufferFree(&buff);
    TestSeriesFree(&testSeries);

    return 0;
}
