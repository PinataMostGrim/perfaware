#include <cpuid.h>
#include <stdint.h>
#include <stdio.h>

#define REPETITION_TESTER_IMPLEMENTATION
#include "../../common/src/repetition_tester.h"

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define UNUSED(x) (void)(x)

#define CLANG_COMPILER (defined(__clang__))
#define GCC_COMPILER (defined(__GNUC__) && !defined(__clang__))
#define MSVC_COMPILER (defined(_MSC_VER))

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t b32;

typedef u64 function_ptr(u64 count);

static u64 NoPMCs(u64 count);
static u64 Rdtsc(u64 count);
static u64 MPerf(u64 count);
static u64 APerf(u64 count);

typedef struct test_function test_function;
struct test_function
{
    char const *Name;
    function_ptr *Func;
};


test_function TestFunctions[] =
{
    {"NoPMCs", NoPMCs},
    {"Rdtsc", Rdtsc},
    {"MPerf", MPerf},
    {"APerf", APerf},
};


static b32 is_rdpru_supported()
{
    u32 eax, ebx, ecx, edx;
    u32 result = 0;

    if (__get_cpuid_max(0, NULL) >= 7)
    {
        __cpuid_count(7, 0, eax, ebx, ecx, edx);
        result = (ecx & (1 << 22)) != 0;
    }

    return result;
}


static uint64_t read_mperf()
{
#if defined(CLANG_COMPILER)
#elif defined(GCC_COMPILER)
    u32 low, high;
    __asm__ volatile("mov $0, %%ecx; rdpru"
                 : "=a" (low), "=d" (high)
                 :
                 : "ecx");

    return ((u64)high << 32) | low;

#elif defined(MSVC_COMPILER)
#endif

    return 0;
}


static uint64_t read_aperf()
{
#if defined(CLANG_COMPILER)
#elif defined(GCC_COMPILER)
    u32 low, high;
    __asm__ volatile("mov $1, %%ecx; rdpru"
                 : "=a" (low), "=d" (high)
                 :
                 : "ecx");

    return ((u64)high << 32) | low;

#elif defined(MSVC_COMPILER)
#endif

    return 0;
}


static u64 NoPMCs(u64 count)
{
    u64 sum = 0;
    for (int i = 0; i < count; ++i)
    {
        sum += 1;
    }

    return sum;
}


static u64 Rdtsc(u64 count)
{
    u64 sum = 0;
    for (int i = 0; i < count; ++i)
    {
        u64 start = ReadCPUTimer();
        sum += 1;
        u64 end = ReadCPUTimer();
    }

    return sum;
}


static u64 MPerf(u64 count)
{
    u64 sum = 0;
    for (int i = 0; i < count; ++i)
    {
        u64 start = read_mperf();
        sum += 1;
        u64 end = read_mperf();
    }

    return sum;
}


static u64 APerf(u64 count)
{
    u64 sum = 0;
    for (int i = 0; i < count; ++i)
    {
        u64 start = read_aperf();
        sum += 1;
        u64 end = read_aperf();
    }

    return sum;
}


int main(int argc, char const *argv[])
{
    if (!is_rdpru_supported())
    {
        fprintf(stderr, "'rdpru' instruction is not supported on this processor.\n");
        return 1;
    }

    InitializeTester();
    repetition_tester testers[ArrayCount(TestFunctions)] = {0};
    u64 count = 10000;

    for(u32 testFunctionIndex = 0; testFunctionIndex < ArrayCount(TestFunctions); testFunctionIndex++)
    {
        test_function testFunction = TestFunctions[testFunctionIndex];
        repetition_tester tester = testers[testFunctionIndex];

        NewTestWave(&tester, 0, TesterGlobals.CPUTimerFrequency, TesterGlobals.SecondsToTry);
        fprintf(stdout, "\n--- %s %lu ---\n", testFunction.Name, count);

        while(IsTesting(&tester))
        {
            BeginTime(&tester);
            testFunction.Func(count);
            EndTime(&tester);
        }
    }

    fprintf(stdout, "\n");

    return 0;
}
