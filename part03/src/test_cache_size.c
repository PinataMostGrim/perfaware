#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <sys/stat.h>

#define LINUX 1
#define WRITE_TO_BUFFER 1

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

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

typedef struct test_function test_function;

// 1024 * 1024 * 1024 = 1073741824
// 2**32 = 4294967296
// 1073741824 - 4294967296 = -3221225472
// A 32 bit integer will be fine for the mask
typedef void ASMFunction(u64 count, u32 mask, u8 *data);

extern void Read_32x4(u64 count, u32 mask, u8 *data);
extern void Read_32x8(u64 count, u32 mask, u8 *data);
extern void Read_32x16(u64 count, u32 mask, u8 *data);

struct test_function
{
    char const *Name;
    ASMFunction *Func;
    u32 AddressMask;
};

//  64 bits =  8 bytes
// 128 bits = 16 bytes
// 256 bits = 32 bytes
// 512 bits = 64 bytes

// A WORD is 16 bits or 2 bytes
// A QUAD WORD is 64 bits, or 8 bytes

//  1x vmovdqu ymm0, [rsi] moves 32 bytes of memory
//  2x vmovdqu ymm0, [rsi] moves 64 bytes of memory
//  4x vmovdqu ymm0, [rsi] moves 128 bytes of memory
//  8x vmovdqu ymm0, [rsi] moves 256 bytes of memory
// 16x vmovdqu ymm0, [rsi] moves 512 bytes of memory

// 0x3F == 0b0011 1111 == 63
// 0x7F == 0b0111 1111 == 127
// 0xFF == 0b1111 1111 == 255
// 0x1FF == 0b0001 1111 1111 == 511
// 0x3FF == 0b0011 1111 1111 == 1023
// 0x7FF == 0b0111 1111 1111 == 2047
// 0xFFF == 0b1111 1111 1111 == 4095
// 0x1FFF == 0b0001 1111 1111 1111 == 8191
// 0x3FFF == 0b0011 1111 1111 1111 == 16383
// 0x7FFF == 0b0111 1111 1111 1111 == 32768
// 0xFFFF == 0b1111 1111 1111 1111 == 65536
// 0x1FFFF == 0b0001 1111 1111 1111 1111 == 131071
// 0x3FFFF == 0b0011 1111 1111 1111 1111 == 262143
// 0x7FFFF == 0b0111 1111 1111 1111 1111 == 524287
// 0xFFFFF == 0b1111 1111 1111 1111 1111 == 1048575

#define TEST_FUNCTION_ENTRY(bytes, human_readable) \
    { "read_bytes_" human_readable, Read_32x8, (bytes) - 1 }

test_function TestFunctions[] =
{
    // Note (Aaron): 128 bytes is the minimum span read by Read_32x4
    // Note (Aaron): 256 bytes is the minimum span read by Read_32x8
    // Note (Aaron): 512 bytes is the minimum span read by Read_32x16

    // TEST_FUNCTION_ENTRY(128, "128b"),
    TEST_FUNCTION_ENTRY(256, "256b"),
    TEST_FUNCTION_ENTRY(512, "512b"),
    TEST_FUNCTION_ENTRY(Kilobytes(1), "1kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(2), "2kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(4), "4kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(8), "8kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(16), "16kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(32), "32kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(64), "64kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(128), "128kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(256), "256kb"),
    TEST_FUNCTION_ENTRY(Kilobytes(512), "512kb"),
    TEST_FUNCTION_ENTRY(Megabytes(1), "1mb"),
    TEST_FUNCTION_ENTRY(Megabytes(2), "2mb"),
    TEST_FUNCTION_ENTRY(Megabytes(4), "4mb"),
    TEST_FUNCTION_ENTRY(Megabytes(8), "8mb"),
    TEST_FUNCTION_ENTRY(Megabytes(16), "16mb"),
    TEST_FUNCTION_ENTRY(Megabytes(32), "32mb"),
    TEST_FUNCTION_ENTRY(Megabytes(128), "128mb"),
};


int main(void)
{
    // Note (Aaron): The read-size of each test function call must divide evenly into the buffer size.
    u64 bufferSizeBytes = Gigabytes(1);
    u64 cpuTimerFrequency = EstimateCPUTimerFrequency();

    buffer buff = BufferAllocate(bufferSizeBytes);
    if (buff.SizeBytes == 0)
    {
        fprintf(stderr, "Unable to allocate memory buffer for testing");
        return 1;
    }

#if WRITE_TO_BUFFER
    // Note (Aaron): Linux maps all allocated pages to a single page full of 0s until
    // a write is attempted. Write to each page so Linux is forced to actually commit
    // them and we pull the expected amount of memory into cache.
    u32 PAGE_SIZE = 4096;
    for (int i = 0; i < buff.SizeBytes; i+=PAGE_SIZE)
    {
        *(buff.Data + i) = 1;
    }
#endif

    repetition_tester testers[ArrayCount(TestFunctions)] = {0};
    for(;;)
    {
        for(u32 funcIndex = 0; funcIndex < ArrayCount(TestFunctions); ++funcIndex)
        {
            repetition_tester *tester = &testers[funcIndex];
            test_function testFunc = TestFunctions[funcIndex];
            u32 secondsToTry = 10;

            printf("\n--- %s ---\n", testFunc.Name);
            NewTestWave(tester, bufferSizeBytes, cpuTimerFrequency, secondsToTry);

            while(IsTesting(tester))
            {
                BeginTime(tester);
                testFunc.Func(bufferSizeBytes, testFunc.AddressMask, buff.Data);
                EndTime(tester);
                CountBytes(tester, bufferSizeBytes);
            }
        }
    }

    return 0;
}
