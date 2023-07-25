#include <assert.h>
#include <limits.h>

// #include "base_inc.h"
#include "base_types.c"
#include "base_memory.c"

#define PLATFORM_METRICS_IMPLEMENTATION
#include "platform_metrics.h"


static U64 FunctionA(U64 value);
static U64 FunctionB(U64 value);
static U64 FunctionC(U64 value);


static U64 FunctionA(U64 value)
{
    START_TIMING(FunctionA)
    for (int i = 0; i < 100; ++i)
    {
        value = FunctionB(value);
    }
    END_TIMING(FunctionA)

    return value;
}


static U64 FunctionB(U64 value)
{
    START_TIMING(FunctionB)
    for (int i = 0; i < 1000; ++i)
    {
        value += FunctionC(value);
    }
    END_TIMING(FunctionB)

    return value;
}


static U64 FunctionC(U64 value)
{
    START_TIMING(FunctionC)
    for (int i = 0; i < 10000; ++i)
    {
        value = value + 1;
    }
    END_TIMING(FunctionC)

    return value;
}


static void Recursion(int *total, int *level)
{
    START_TIMING(Recursion)

    if (*level > 0)
    {
        *level = *level - 1;
        Recursion(total, level);
    }

    for (int i = 0; i < 1000; ++i)
    {
        *total = *total + 1;
    }

    END_TIMING(Recursion)
}



int main()
{
#if 0
    {
        StartTimingsProfile();

        // START_TIMING(MainLoop);

        PREWARM_TIMING(MainLoop);
        RESTART_TIMING(MainLoop);

        U64 total = 0;
        for (int i = 0; i < 10000000; ++i)
        {
            total++;
        }

        END_TIMING(MainLoop);

        EndTimingsProfile();
        PrintTimingsProfile();
        printf("\n");
    }
#endif

#if 0
    {
        StartTimingsProfile();
        int levels = 3;
        int total = 0;

        Recursion(&total, &levels);

        EndTimingsProfile();
        PrintTimingsProfile();
        printf("\n");
    }
#endif

#if 0
    {
        StartTimingsProfile();

        U64 total = 0;
        total = FunctionA(total);
        printf("total: %llu\n", total);

        EndTimingsProfile();
        PrintTimingsProfile();
        printf("\n");
    }
#endif

#if 0
    {
        StartTimingsProfile();

        for (int i = 0; i < 100000000 ; ++i)
        {
            START_TIMING(TimingsTest);
            END_TIMING(TimingsTest);
        }

        EndTimingsProfile();
        PrintTimingsProfile();
    }
#endif

    return 0;
}

static_assert(__COUNTER__ <= ArrayCount(GlobalProfiler.Timings) , "__COUNTER__ exceeds the number of timings available");
