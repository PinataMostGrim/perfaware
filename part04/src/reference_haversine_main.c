#include <math.h>
#include <stdio.h>
#include <sys/stat.h>

#include "base_inc.h"

#include "buffer.h"
#include "tester_common.h"

#include "reference_haversine.h"
#include "reference_haversine_lexer.h"
#include "reference_haversine_parser.h"

#include "base_types.c"
#include "base_memory.c"
#include "base_arena.c"
#include "base_string.c"

#include "buffer.c"
#include "tester_common.c"

#include "reference_haversine.c"
#include "reference_haversine_lexer.c"
#include "reference_haversine_parser.c"


#define REPETITION_TESTER_IMPLEMENTATION
#include <repetition_tester.h>


typedef struct test_function test_function;
typedef F64 haversine_compute_func(haversine_setup setup);
typedef U64 haversine_verify_func(haversine_setup setup);


struct test_function
{
    char const *Name;
    haversine_compute_func *Compute;
    haversine_verify_func *Verify;
};

static test_function TestFunctions[] =
{
    {"ReferenceHaversine", ReferenceSumHaversine, ReferenceVerifyHaversine }
};


int main(int argCount, char const *args[])
{
    if (argCount != 3)
    {
        fprintf(stderr, "Usage: %s [haversine pairs file] [haversine answers file]\n", args[0]);
        return 1;
    }

    InitializeTesterGlobals();

    char *haversinePairsFilename = (char *)args[1];
    char *answersFilename = (char *)args[2];

    haversine_setup setup = SetupHaversine(haversinePairsFilename, answersFilename);
    test_series testSeries = TestSeriesAllocate(ArrayCount(TestFunctions), 1);

    if (SetupIsValid(setup) && TestSeriesIsValid(testSeries))
    {
        F64 referenceSum = setup.SumAnswer;
        SetRowLabelLabel(&testSeries, "Test");
        SetRowLabel(&testSeries, "Haversine");

        for(U32 testFunctionIndex = 0; testFunctionIndex < ArrayCount(TestFunctions); ++testFunctionIndex)
        {
            test_function testFunction = TestFunctions[testFunctionIndex];

            SetColumnLabel(&testSeries, "%s", testFunction.Name);

            repetition_tester tester = {0};
            TestSeriesNewTestWave(&testSeries, &tester, setup.ParsedByteCount, TesterGlobals.CPUTimerFrequency, TesterGlobals.SecondsToTry);

            U64 individualErrorCount = testFunction.Verify(setup);
            U64 sumErrorCount = 0;

            while(TestSeriesIsTesting(&testSeries, &tester))
            {
                BeginTime(&tester);
                F64 computedSum = testFunction.Compute(setup);
                CountBytes(&tester, setup.ParsedByteCount);
                EndTime(&tester);

                sumErrorCount += !ApproxAreEqual(computedSum, referenceSum);
            }

            if (sumErrorCount || individualErrorCount)
            {
                fprintf(stderr, "[WARNING]: %lu haversines mismatched, %lu sum mismatches\n",
                        individualErrorCount, sumErrorCount);
            }

            PrintCSVForValue(&testSeries, StatValue_GBPerSecond, stdout, 1.0);
        }
    }
    else
    {
        fprintf(stderr, "[ERROR]: Test data size must be non-zero\n");
    }

    FreeHaversine(&setup);
    TestSeriesFree(&testSeries);

    return 0;
}
