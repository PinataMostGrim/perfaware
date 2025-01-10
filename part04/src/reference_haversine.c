#include <math.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "base_inc.h"

#include "buffer.h"
#include "tester_common.h"

#include "reference_haversine.h"
#include "reference_haversine_lexer.h"
#include "reference_haversine_parser.h"


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


global_function haversine_setup SetupHaversine(char *haversinePairsFilename, char *answersFilename)
{
    haversine_setup result = {0};

    fprintf(stdout, "[INFO] Initializing reference haversine tester\n");

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

            fprintf(stdout, "Source JSON: %" PRIu64 "mb\n", result.JsonArena.Size / Megabytes(1));
            fprintf(stdout, "Parsed: %" PRIu64 "mb (%" PRIu64 " pairs)\n", result.ParsedByteCount / Megabytes(1), result.PairCount);

            result.Valid = (result.PairCount != 0);
        }
        else
        {
            fprintf(stderr, "[ERROR]: JSON source data has %" PRIu64 " pairs, but answer file has %" PRIu64 " values (should have %" PRIu64 ").\n",
                    pairCount, answerCount, answersHeader.PairCount);
        }
    }

    return result;
}


global_function B32 SetupIsValid(haversine_setup setup)
{
    B32 result = setup.Valid;
    return result;
}


global_function void FreeHaversine(haversine_setup *setup)
{
    ArenaFree(&setup->JsonArena);
    ArenaFree(&setup->AnswersArena);
    ArenaFree(&setup->PairsArena);
    ArenaFree(&setup->TokenArena);
}


global_function B32 ApproxAreEqual(F64 a, F64 b)
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


global_function F64 Square(F64 A)
{
    F64 Result = (A*A);
    return Result;
}


global_function F64 RadiansFromDegrees(F64 Degrees)
{
    F64 Result = 0.01745329251994329577f * Degrees;
    return Result;
}


// NOTE(casey): EarthRadius is generally expected to be 6372.8
global_function F64 ReferenceHaversine(F64 X0, F64 Y0, F64 X1, F64 Y1, F64 EarthRadius)
{
    /* NOTE(casey): This is not meant to be a "good" way to calculate the Haversine distance.
       Instead, it attempts to follow, as closely as possible, the formula used in the real-world
       question on which these homework exercises are loosely based.
    */

    F64 lat1 = Y0;
    F64 lat2 = Y1;
    F64 lon1 = X0;
    F64 lon2 = X1;

    F64 dLat = RadiansFromDegrees(lat2 - lat1);
    F64 dLon = RadiansFromDegrees(lon2 - lon1);
    lat1 = RadiansFromDegrees(lat1);
    lat2 = RadiansFromDegrees(lat2);

    F64 a = Square(sin(dLat/2.0)) + cos(lat1)*cos(lat2)*Square(sin(dLon/2));
    F64 c = 2.0*asin(sqrt(a));

    F64 Result = EarthRadius * c;

    return Result;
}


global_function F64 ReferenceSumHaversine(haversine_setup setup)
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


global_function U64 ReferenceVerifyHaversine(haversine_setup setup)
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
