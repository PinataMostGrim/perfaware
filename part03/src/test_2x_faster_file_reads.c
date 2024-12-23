// source: https://github.com/cmuratori/computer_enhance/blob/main/perfaware/part3/listing_0166_osread_revisited_main.cpp

#include <sys/stat.h>
#include <stdint.h>
#include <string.h>

#include "tester_common.h"
#include "tester_common.c"

#include "buffer.h"
#include "buffer.c"

#define REPETITION_TESTER_IMPLEMENTATION
#include "repetition_tester.h"

#if _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#endif

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define MIN_MEMORY_PAGE_SIZE 4096

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t b32;

typedef float f32;
typedef double f64;

typedef struct test_function test_function;
typedef void file_process_func(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize, buffer scratch);

static void AllocateAndTouch(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize, buffer scratch);
static void AllocateAndCopy(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize, buffer scratch);
static void OpenAllocateAndRead(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize, buffer scratch);
static void OpenAllocateAndFRead(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize, buffer scratch);

struct test_function
{
    char const *Name;
    file_process_func *Func;
};


test_function TestFunctions[] =
{
    {"AllocateAndTouch", AllocateAndTouch},
    {"AllocateAndCopy", AllocateAndCopy},
    {"OpenAllocateAndRead", OpenAllocateAndRead},
    {"OpenAllocateAndFRead", OpenAllocateAndFRead},
};


#define MIN_MEMORY_PAGE_SIZE 4096
static void AllocateAndTouch(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize, buffer scratch)
{
    buffer buff = BufferAllocate(bufferSize);
    if(BufferIsValid(buff))
    {
        u64 touchCount = (buff.SizeBytes + MIN_MEMORY_PAGE_SIZE - 1) / MIN_MEMORY_PAGE_SIZE;
        for(u64 touchIndex = 0; touchIndex < touchCount; ++touchIndex)
        {
            buff.Data[MIN_MEMORY_PAGE_SIZE * touchIndex] = 0;
        }

        CountBytes(tester, totalFileSize);
    }
    else
    {
        Error(tester, "Couldn't acquire resources");
    }

    BufferFree(&buff);
}


static void AllocateAndCopy(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize, buffer scratch)
{
    buffer buff = BufferAllocate(bufferSize);
    if(BufferIsValid(buff))
    {
        uint8_t *source = scratch.Data;
        u64 sizeRemaining = totalFileSize;
        while(sizeRemaining)
        {
            u64 readSize = buff.SizeBytes;
            if(readSize > sizeRemaining)
            {
                readSize = sizeRemaining;
            }

#if 0
            // Note (Aaron H): On x64, this will do a direct rep movsb, which is often the fastest way to move memory - so
            // it's useful as a spot-check to make sure memcpy isn't introducing too much overhead
            __movsb(buff.Data, source, (uint32_t)readSize);
#else
            memcpy(buff.Data, source, (uint32_t)readSize);
#endif

            CountBytes(tester, readSize);

            sizeRemaining -= readSize;
            source += readSize;
        }
    }
    else
    {
        Error(tester, "Couldn't acquire resources");
    }

    BufferFree(&buff);
}


static void OpenAllocateAndRead(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize, buffer scratch)
{
#if _WIN32
    HANDLE file = CreateFileA(fileName, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    buffer buff = BufferAllocate(bufferSize);

    if (file != INVALID_HANDLE_VALUE && BufferIsValid(buff))
    {
        u64 sizeRemaining = totalFileSize;
        while(sizeRemaining)
        {
            u64 readSize = buff.SizeBytes;
            if(readSize > sizeRemaining)
            {
                readSize = sizeRemaining;
            }

            DWORD bytesRead = 0;
            BOOL result = ReadFile(file, buff.Data, (uint32_t)readSize, &bytesRead, 0);

            if(result && (bytesRead == readSize))
            {
                CountBytes(tester, readSize);
            }
            else
            {
                Error(tester, "ReadFile failed");
            }

            sizeRemaining -= readSize;
        }
    }
    else
    {
        Error(tester, "Couldn't acquire resources");
    }

    BufferFree(&buff);
    CloseHandle(file);

#else
    int file = open(fileName, O_RDONLY, 0);
    buffer buff = BufferAllocate(bufferSize);

    if (file && BufferIsValid(buff))
    {
        u64 sizeRemaining = totalFileSize;
        while(sizeRemaining)
        {
            u64 readSize = buff.SizeBytes;
            if(readSize > sizeRemaining)
            {
                readSize = sizeRemaining;
            }

            ssize_t bytesRead = 0;
            bytesRead = read(file, buff.Data, readSize);

            if (bytesRead > 0 && (bytesRead == readSize))
            {
                CountBytes(tester, readSize);
            }
            else
            {
                Error(tester, "ReadFile failed");
            }

            sizeRemaining -= readSize;
        }
    }
    else
    {
        Error(tester, "Couldn't acquire resources");
    }

    BufferFree(&buff);
    close(file);

#endif
}


static void OpenAllocateAndFRead(repetition_tester *tester, char const *fileName, u64 totalFileSize, u64 bufferSize, buffer scratch)
{
    FILE *file = fopen(fileName, "rb");
    buffer buff = BufferAllocate(bufferSize);

    if(file && BufferIsValid(buff))
    {
        u64 sizeRemaining = totalFileSize;
        while(sizeRemaining)
        {
            u64 readSize = buff.SizeBytes;
            if(readSize > sizeRemaining)
            {
                readSize = sizeRemaining;
            }

            if(fread(buff.Data, readSize, 1, file) == 1)
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

    SetRowLabelLabel(&testSeries, "ReadBufferSize");
    for(u64 readBufferSize = 256*1024; readBufferSize <= buff.SizeBytes; readBufferSize*=2)
    {
        SetRowLabel(&testSeries, "%lluk", readBufferSize/1024);
        for(u32 testFunctionIndex = 0; testFunctionIndex < ArrayCount(TestFunctions); ++testFunctionIndex)
        {
            test_function function = TestFunctions[testFunctionIndex];
            SetColumnLabel(&testSeries, "%s", function.Name);

            repetition_tester tester = {0};
            TestSeriesNewTestWave(&testSeries, &tester, fileSize, TesterGlobals.CPUTimerFrequency, TesterGlobals.SecondsToTry);

            while(TestSeriesIsTesting(&testSeries, &tester))
            {
                BeginTime(&tester);
                function.Func(&tester, fileName, fileSize, readBufferSize, buff);
                EndTime(&tester);
            }
        }
    }

    PrintCSVForValue(&testSeries, StatValue_GBPerSecond, stdout, 1.0f);

    BufferFree(&buff);
    TestSeriesFree(&testSeries);

    return 0;
}
