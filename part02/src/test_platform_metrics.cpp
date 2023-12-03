#include <stdio.h>
#include <stdint.h>

#define PLATFORM_METRICS_IMPLEMENTATION
#define PROFILER 1
#include "../../common/src/platform_metrics.h"


static void TestManualTimings(uint64_t loops)
{
    START_TIMING(TestManualTimings)

    uint64_t value = 0;
    for (uint64_t i = 0; i < loops; ++i)
    {
        value = value + 1;
    }

    END_TIMING(TestManualTimings)
}


static void TestAutomaticTimings(uint64_t loops)
{
    FUNCTION_TIMING

    uint64_t value = 0;
    for (uint64_t i = 0; i < loops; ++i)
    {
        value = value + 1;
    }
}


static void TestBandwidthTimings(uint64_t loops, uint64_t byteCount)
{
    BANDWIDTH_TIMING(BandwidthTiming, byteCount);

    uint64_t value = 0;
    for (uint64_t i = 0; i < loops; ++i)
    {
        value = value + 1;
    }
}


int main(int argc, char const *argv[])
{
    StartTimingsProfile();

    uint64_t loops = 1000000;
    uint64_t bytes = 1024 * 1024 * 1;

    TestManualTimings(loops);
    TestAutomaticTimings(loops);
    TestBandwidthTimings(loops, bytes);

    EndTimingsProfile();
    PrintProfileTimings();

    return 0;
}


ProfilerEndOfCompilationUnit
