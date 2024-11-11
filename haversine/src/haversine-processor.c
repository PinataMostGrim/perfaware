/*  TODO (Aaron):
    - Lex / Parse JSON all at once
        - Add timing for lexing / parsing
*/

#pragma warning(disable:4996)

#include <assert.h>
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <sys/stat.h>

#define PROFILER 1
#define PLATFORM_METRICS_IMPLEMENTATION
#include "platform_metrics.h"

#include "base_inc.h"
#include "haversine.h"
#include "haversine_lexer.h"
#include "haversine_parser.h"

#include "base_types.c"
#include "base_memory.c"
#include "base_arena.c"
#include "base_string.c"
#include "haversine.c"
#include "haversine_lexer.c"
#include "haversine_parser.c"


#define EPSILON_FLOAT 0.001

// Note (Aaron): Validate each distance calculation using values from the answers file
#define VALIDATE_ALL_PAIRS 0


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

    START_BANDWIDTH_TIMING(ReadFileContents, fileSize);
    B8 read_success = fread(result.BasePtr, fileSize, 1, file) == 1;
    END_TIMING(ReadFileContents)
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


global_function void PrintToken(haversine_token *token)
{
    printf("%s:\t\t%s\n",
           GetTokenMenemonic(token->Type),
           token->String);
}


global_function void PrintStats(parsing_stats *stats)
{
    printf("[INFO] Tokens processed:    %" PRIu64"\n", stats->TokenCount);
    printf("[INFO] Max token length:    %" PRIu64"\n", stats->MaxTokenLength);
    printf("[INFO] Pairs processed:     %" PRIu64"\n", stats->PairsProcessed);
#if VALIDATE_ALL_PAIRS
    printf("[INFO] Calculation errors:  %" PRIu64"\n", stats->CalculationErrors);
#endif
    printf("\n");
    printf("[INFO] Expected sum:        %.16f\n", stats->ExpectedSum);
    printf("[INFO] Calculated sum:      %.16f\n", stats->CalculatedSum);
    printf("[INFO] Sum divergence:      %.16f\n", stats->SumDivergence);

    if (stats->PairsProcessed != stats->ExpectedPairCount)
    {
        printf("[ERROR] Parsed pair count (%" PRIu64") does not match value in answers file (%" PRIu64")\n",
               stats->PairsProcessed,
               stats->ExpectedPairCount);
    }
}


global_function void PrintHaversineDistance(V2F64 point0, V2F64 point1, F64 distance)
{
    printf("[INFO] Haversine distance: (%f, %f) -> (%f, %f) = %f\n", point0.x, point0.y, point1.x, point1.y, distance);
}


int main()
{
    StartTimingsProfile();
    START_TIMING(Startup); ////////////////////////////////////////////////////

    // read data file
    char *dataFilename = DATA_FILENAME;
    printf("[INFO] Processing file '%s'\n", dataFilename);
    memory_arena jsonContents = ReadFileContents(dataFilename);
    if (!ArenaIsValid(&jsonContents))
    {
        perror("[ERROR] ");
        return 1;
    }

    // read answer file
    char *answerFilename = ANSWER_FILENAME;
    printf("[INFO] Processing file '%s'\n", answerFilename);
    memory_arena answerContents = ReadFileContents(answerFilename);
    if (!answerContents.BasePtr)
    {
        perror("[ERROR] ");
        return 1;
    }

    // read answers file header
    answers_file_header answerHeader = {0};
    MemoryCopy(&answerHeader, answerContents.PositionPtr, sizeof(answers_file_header));
    answerContents.PositionPtr += sizeof(answers_file_header);

    START_TIMING(MemoryAllocation) //////////////////////////////////
    // allocate memory arena for token stack
    // TODO (Aaron): Does this arena need to be growable?
    U64 tokenStackSize = Megabytes(1);
    memory_arena tokenArena = ArenaAllocate(tokenStackSize, tokenStackSize);
    if (!ArenaIsValid(&tokenArena))
    {
        printf("[ERROR] Unable to allocate memory for token stack\n");
        exit(1);
    }

    token_stack tokenStack = {0};
    tokenStack.Arena = &tokenArena;

    // allocate memory for pairs values
    memory_index pairsArenaSize = GetMaxPairsSize(jsonContents.Size);
    memory_arena pairsArena = ArenaAllocate(pairsArenaSize, pairsArenaSize);
    if (!ArenaIsValid(&pairsArena))
    {
        printf("[ERROR] Unable to allocate memory for haversine pairs values\n");
        exit(1);
    }
    END_TIMING(MemoryAllocation) ////////////////////////////////////
    END_TIMING(Startup); //////////////////////////////////////////////////////

    printf("[INFO] Processing haversine pairs\n");
    START_BANDWIDTH_TIMING(JSONParsing, jsonContents.Size)
    parsing_stats stats = ParseHaversinePairs(&jsonContents, &pairsArena, &tokenArena);
    stats.ExpectedSum = answerHeader.ExpectedSum;
    stats.ExpectedPairCount = answerHeader.PairCount;
    END_TIMING(JSONParsing)

    START_BANDWIDTH_TIMING(HaversineDistance, pairsArena.Used)
    haversine_pair *pairs = (haversine_pair *)pairsArena.BasePtr;

#if VALIDATE_ALL_PAIRS
    F64 *answerPtr = (F64 *)answerContents.PositionPtr;
#endif

    for (int i = 0; i < stats.PairsParsed; ++i)
    {
        haversine_pair pair = pairs[i];

        F64 distance = ReferenceHaversine(pair.point0.x, pair.point0.y, pair.point1.x, pair.point1.y, EARTH_RADIUS);

#if 0
        PrintHaversineDistance(point0, point1, distance);
#endif

        stats.CalculatedSum = ((stats.CalculatedSum * (F64)stats.PairsProcessed) + distance) / (F64)(stats.PairsProcessed + 1);
        stats.PairsProcessed++;

#if VALIDATE_ALL_PAIRS
        F64 answerDistance = answerPtr[i];

        Assert(((answerPtr + i) < (F64 *)(answerContents.BasePtr + answerContents.Used))
               && "Less pairs in the answers file than being parsed from the JSON");

        F64 difference = AbsF64(answerDistance - distance);
        difference = difference >= 0 ? difference : difference * -1;
        if (difference > EPSILON_FLOAT)
        {
            stats.CalculationErrors++;
            printf("[WARN] Calculated distance diverges from answer value significantly (calculated: %f vs. answer: %f)\n", distance, answerDistance);
        }
#endif
    }

    stats.SumDivergence = AbsF64(stats.CalculatedSum - stats.ExpectedSum);
    END_TIMING(HaversineDistance);

    printf("\n");
    PrintStats(&stats);
    printf("\n");

    EndTimingsProfile();
    PrintProfileTimings();

    return 0;
}

ProfilerEndOfCompilationUnit
