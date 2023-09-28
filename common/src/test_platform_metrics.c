#include <stdio.h>
#include <stdint.h>

#define PLATFORM_METRICS_IMPLEMENTATION
#define PROFILER 1
#include "platform_metrics.h"


static void TestManualTimings(uint64_t loops)
{
    START_TIMING(TestManualTimings)

    uint64_t value = 0;
    for (int i = 0; i < loops; ++i)
    {
        value = value + 1;
    }

    END_TIMING(TestManualTimings)
}


static void TestBandwidthTiming(uint64_t loops, uint64_t byteCount)
{
    START_BANDWIDTH_TIMING(TestBandwidthTiming, byteCount);

    uint64_t value = 0;
    for (int i = 0; i < loops; ++i)
    {
        value = value + 1;
    }

    END_TIMING(TestBandwidthTiming)
}


int main(int argc, char const *argv[])
{
    StartTimingsProfile();

    uint64_t loops = 100000000;
    uint64_t bytes = 1024 * 1024 * 1;

    TestManualTimings(loops);
    TestBandwidthTiming(loops, bytes);

    EndTimingsProfile();
    PrintProfileTimings();

    return 0;
}


ProfilerEndOfCompilationUnit
