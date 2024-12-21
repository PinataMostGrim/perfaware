#include <math.h>
#include <stdio.h>
#include <sys/stat.h>

#include "base_inc.h"

#include "buffer.h"
#include "tester_common.h"

#include "haversine.h"
#include "haversine_lexer.h"
#include "haversine_parser.h"

#include "base_types.c"
#include "base_memory.c"
#include "base_arena.c"
#include "base_string.c"

#include "buffer.c"
#include "tester_common.c"

#include "haversine.c"
#include "haversine_lexer.c"
#include "haversine_parser.c"


#define REPETITION_TESTER_IMPLEMENTATION
#include <repetition_tester.h>

typedef struct haversine_setup haversine_setup;
typedef struct test_function test_function;
typedef F64 haversine_compute_func(haversine_setup setup);
typedef U64 haversine_verify_func(haversine_setup setup);

F64 ReferenceSumHaversine(haversine_setup setup);
U64 ReferenceVerifyHaversine(haversine_setup setup);

struct haversine_setup
{
    memory_arena JsonArena;
    memory_arena AnswersArena;
    memory_arena PairsArena;
    memory_arena TokenArena;

    U64 ParsedByteCount;

    U64 PairCount;
    haversine_pair *Pairs;
    F64 *Answers;

    F64 SumAnswer;
    B64 Valid;
};

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


global_function memory_arena ReadFileContents(char *filename)
{
    memory_arena result = {0};
    FILE *file = fopen(filename, "rb");

    if(!file)
    {
        fprintf(stderr, "[ERROR] Unable to open file \"%s\"\n", filename);
        Assert(FALSE);

        return result;
    }

#if _WIN32
    struct __stat64 stats;
    _stat64(filename, &stats);
#else
    struct stat stats;
    stat(filename, &stats);
#endif

    memory_index fileSize = stats.st_size;

    result = ArenaAllocate(fileSize, fileSize);
    if (!ArenaIsValid(&result))
    {
        printf("[ERROR] Unable to allocate memory for \"%s\"\n", filename);
        fclose(file);
        Assert(FALSE);

        return result;
    }

    B32 read_success = fread(result.BasePtr, fileSize, 1, file) == 1;
    if(!read_success)
    {
        fprintf(stderr, "[ERROR] Unable to read file \"%s\"\n", filename);
        free(result.BasePtr);
        fclose(file);
        Assert(FALSE);

        return result;
    }

    fclose(file);

    result.Used = fileSize;
    // Note (Aaron): We keep the arena's position pointer at the base pointer so we can use it as a read head for parsing the data.

    return result;
}


haversine_setup SetupHaversine(char *haversinePairsFilename, char *answersFilename)
{
    haversine_setup result = {0};

    fprintf(stdout, "[INFO] Initializing reference haversine tester");

    // read haversine pairs and answers files into buffers
    result.JsonArena = ReadFileContents(haversinePairsFilename);
    result.AnswersArena = ReadFileContents(answersFilename);

    // allocate memory for pairs values
    memory_index pairsArenaSize = GetMaxPairsSize(result.JsonArena.Size);
    result.PairsArena = ArenaAllocate(pairsArenaSize, pairsArenaSize);

    // allocate an arena for the token stack
    memory_index tokenArenaSize = Megabytes(1);
    result.TokenArena  = ArenaAllocate(tokenArenaSize, tokenArenaSize);

    if (ArenaIsValid(&result.JsonArena)
        && ArenaIsValid(&result.AnswersArena)
        && ArenaIsValid(&result.PairsArena)
        && ArenaIsValid(&result.TokenArena))
    {
        result.Pairs = (haversine_pair *)result.PairsArena.BasePtr;

        // retrieve answers counts
        answers_file_header answersHeader = *(answers_file_header *)result.AnswersArena.BasePtr;
        U64 answerCount = (result.AnswersArena.Size - sizeof(answers_file_header)) / sizeof(F64);

        // parse pairs from the json and assign the pairs count
        parsing_stats stats = ParseHaversinePairs(&result.JsonArena, &result.PairsArena, &result.TokenArena);
        U64 pairCount = stats.PairsParsed;

        // if the pairs count matches the answers count, then fill out the rest of the setup struct and mark as valid
        if (answersHeader.PairCount == answerCount && answersHeader.PairCount == pairCount)
        {
            result.PairCount = pairCount;
            result.Answers = (F64 *)(result.AnswersArena.BasePtr + sizeof(answers_file_header));
            result.SumAnswer = answersHeader.ExpectedSum;

            result.ParsedByteCount = (sizeof(haversine_pair) * result.PairCount);

            fprintf(stdout, "Source JSON: %llumb\n", result.JsonArena.Size / Megabytes(1));
            fprintf(stdout, "Parsed: %llumb (%lu pairs)\n", result.ParsedByteCount / Megabytes(1), result.PairCount);

            result.Valid = (result.PairCount != 0);
        }
        else
        {
            fprintf(stderr, "[ERROR]: JSON source data has %lu pairs, but answer file has %lu values (should have %lu).\n",
                    pairCount, answerCount, answersHeader.PairCount);
        }
    }

    return result;
}


B32 SetupIsValid(haversine_setup setup)
{
    B32 result =  setup.Valid;
    return result;
}


static void FreeHaversine(haversine_setup *setup)
{
    ArenaFree(&setup->JsonArena);
    ArenaFree(&setup->AnswersArena);
    ArenaFree(&setup->PairsArena);
    ArenaFree(&setup->TokenArena);
}


B32 ApproxAreEqual(F64 a, F64 b)
{
    /* NOTE(casey): Epsilon can be set to whatever tolerance we decide we will accept. If we make this value larger,
       we have more options for optimization. If we make it smaller, we must more closely follow the sequence
       of floating point operations that produced the original value. At zero, we would have to reproduce the
       sequence _exactly_. */
    F64 epsilon = 0.00000001f;

    F64 diff = (a - b);
    B32 result = (diff > -epsilon && diff < epsilon);
    return result;
}


F64 ReferenceSumHaversine(haversine_setup setup)
{
    U64 pairCount = setup.PairCount;
    haversine_pair *pairs = setup.Pairs;

    F64 sum = 0;

    F64 sumCoeficient = 1 / (F64)pairCount;
    for (U64 pairIndex = 0; pairIndex < pairCount; ++pairIndex)
    {
        haversine_pair pair = pairs[pairIndex];
        F64 earthRadius = EARTH_RADIUS;
        F64 dist = ReferenceHaversine(pair.point0.x, pair.point0.y, pair.point1.x, pair.point1.y, earthRadius);
        sum += sumCoeficient * dist;
    }

    return sum;
}


U64 ReferenceVerifyHaversine(haversine_setup setup)
{
    U64 pairCount = setup.PairCount;
    haversine_pair *pairs = setup.Pairs;
    F64 *answers = setup.Answers;

    U64 errorCount = 0;

    for (int pairIndex = 0; pairIndex < pairCount; ++pairIndex)
    {
        haversine_pair pair = pairs[pairIndex];
        F64 earthRadius = EARTH_RADIUS;
        F64 dist = ReferenceHaversine(pair.point0.x, pair.point0.y, pair.point1.x, pair.point1.y, earthRadius);
        if (!ApproxAreEqual(dist, answers[pairIndex]))
        {
            ++errorCount;
        }
    }

    return errorCount;
}


int main(int argCount, char const *args[])
{
    if (argCount != 3)
    {
        fprintf(stderr, "Usage: %s [haversine pairs file] [haversine answers file]\n", args[0]);
        return 1;
    }

    InitializeTester();

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
