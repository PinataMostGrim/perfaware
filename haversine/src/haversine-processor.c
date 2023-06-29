/*  TODO (Aaron):
*/

#pragma warning(disable:4996)


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base.h"
#include "memory_arena.h"
#include "haversine.h"
#include "haversine_lexer.h"
#include "memory_arena.h"
#define PLATFORM_METRICS_IMPLEMENTATION
#include "platform_metrics.h"

#include "haversine.c"
#include "haversine_lexer.c"
#include "memory_arena.c"


#define ENABLE_TIMINGS 1
#define EPSILON_FLOAT 0.001


typedef struct
{
    union
    {
        metric_timing Subtimings[8];
        struct
        {
            metric_timing Startup;
            metric_timing JSONLexing;
            metric_timing JSONParsing;
            metric_timing StackOperations;
            metric_timing HaversineDistance;
            metric_timing Validation;
            metric_timing SumCalculation;
            metric_timing MiscOperations;
        } _sub_timings;
    };

    metric_timing Total;
    U64 CPUFrequency;
} haversine_processor_metrics;


typedef struct
{
    memory_arena *Arena;
    S64 TokenCount;
} token_stack;


typedef struct
{
    S64 TokenCount;
    S64 MaxTokenLength;
    S64 PairsProcessed;
    U64 CalculationErrors;
    F64 ExpectedSum;
    F64 CalcualtedSum;
} processor_stats;


typedef struct
{
    haversine_token *PairsToken;

    haversine_token *ArrayStartToken;
    haversine_token *ArrayEndToken;

    haversine_token *ScopeEnterToken;
    haversine_token *ScopeExitToken;

    haversine_token *X0Token;
    haversine_token *Y0Token;
    haversine_token *X1Token;
    haversine_token *Y1Token;

} pairs_context;


global_function void InitializeProcessorMetrics(haversine_processor_metrics *metrics)
{
    for (int i = 0; i < ArrayCount(metrics->Subtimings); ++i)
    {
        InitializeMetricTiming(&metrics->Subtimings[i]);
    }

    InitializeMetricTiming(&metrics->Total);
    metrics->CPUFrequency = 0;
}


global_function void InitializeProcessorStats(processor_stats *stats)
{
    stats->TokenCount = 0;
    stats->MaxTokenLength = 0;
    stats->PairsProcessed = 0;
    stats->CalculationErrors = 0;
    stats->ExpectedSum = 0;
    stats->CalcualtedSum = 0;
}


global_function void InitializePairsContext(pairs_context *context)
{
    context->PairsToken = 0;
    context->ArrayStartToken = 0;
    context->ArrayEndToken = 0;
    context->ScopeEnterToken = 0;
    context->ScopeExitToken = 0;
    context->X0Token = 0;
    context->Y0Token = 0;
    context->X1Token = 0;
    context->Y1Token = 0;
}


global_function void InitializeTokenStack(token_stack *stack, memory_arena *arena)
{
    stack->Arena = arena;
    stack->TokenCount = 0;
}


global_function haversine_token *PushToken(token_stack *tokenStack, haversine_token value)
{
    haversine_token *tokenPtr = ArenaPushStruct(tokenStack->Arena, haversine_token);
    tokenStack->TokenCount++;
    MemoryCopy(tokenPtr, &value, sizeof(haversine_token));

    // TODO (Aaron): Error handling?

    return tokenPtr;
}


global_function haversine_token PopToken(token_stack *tokenStack)
{
    haversine_token result;
    haversine_token *tokenPtr = ArenaPopStruct(tokenStack->Arena, haversine_token);
    tokenStack->TokenCount--;

    MemoryCopy(&result, tokenPtr, sizeof(haversine_token));

#if HAVERSINE_SLOW
    MemorySet((U8 *)tokenPtr, 0xff, sizeof(haversine_token));
#endif

    // TODO (Aaron): Error handling?

    return result;
}


global_function V2F64 GetVectorFromCoordinateTokens(haversine_token xValue, haversine_token yValue)
{
    V2F64 result = { .x = 0, .y = 0};

    char *xStartPtr = xValue.String;
    char *xEndPtr = xStartPtr + strlen(xValue.String);
    result.x = strtod(xValue.String, &xEndPtr);

    char *yStartPtr = yValue.String;
    char *yEndPtr = yStartPtr + strlen(yValue.String);
    result.y = strtod(yValue.String, &yEndPtr);

    // TODO (Aaron): Error handling?

    return result;
}


global_function void PrintStats(processor_stats *stats)
{
    printf("[INFO] Tokens processed:    %lli\n", stats->TokenCount);
    printf("[INFO] Max token length:    %lli\n", stats->MaxTokenLength);
    printf("[INFO] Pairs processed:     %lli\n", stats->PairsProcessed);
    printf("[INFO] Calculation errors:  %lli\n", stats->CalculationErrors);
    printf("\n");
    printf("[INFO] Expected sum:        %.16f\n", stats->ExpectedSum);
    printf("[INFO] Calculated sum:      %.16f\n", stats->CalcualtedSum);
}


global_function void PrintToken(haversine_token *token)
{
    printf("%s:\t\t%s\n",
           GetTokenMenemonic(token->Type),
           token->String);
}


global_function void PrintHaversineDistance(V2F64 point0, V2F64 point1, F64 distance)
{
    printf("[INFO] Haversine distance: (%f, %f) -> (%f, %f) = %f\n", point0.x, point0.y, point1.x, point1.y, distance);
}


inline void StartCPUTiming(metric_timing *timing)
{
#if ENABLE_TIMINGS
    timing->Start = ReadCPUTimer();
#endif
}


inline void EndCPUTimingAndIncrementMetric(metric_timing *timing)
{
#if ENABLE_TIMINGS
    timing->End = ReadCPUTimer();
    U64 duration = timing->End - timing->Start;
    timing->Duration += duration;
#endif
}


inline void PrintTiming(char *timingName, U64 timing, U64 totalTime, U8 tabCount)
{
    printf("  %s:", timingName);
    for (int i = 0; i < tabCount; ++i)
    {
        printf("\t");
    }
    printf("%llu (%.2f%s)\n", timing, ((F64)timing / (F64)totalTime) * 100.0f, "%");
}


int main()
{
    // calculate CPU frequency
    haversine_processor_metrics metrics;
    InitializeProcessorMetrics(&metrics);
    metrics.CPUFrequency = GetCPUFrequency(200);

    static_assert(sizeof(metrics.Subtimings) == sizeof(metrics._sub_timings),
                  "Union mismatch - Timings array does not match the number of timings in _haversine_timings struct");

    // Note (Aaron): Avoid using StartCPUTiming() for the Total timing so we can
    // disable the timings methods and get a sense of how much of an impact they
    // have on execution times.
    metrics.Total.Start = ReadCPUTimer();
    StartCPUTiming(&metrics._sub_timings.Startup);

    // open data file
    char *dataFilename = DATA_FILENAME;
    FILE *dataFile;
    dataFile = fopen(dataFilename, "r");

    printf("[INFO] Processing file '%s'\n", dataFilename);

    if (!dataFile)
    {
        perror("[ERROR] ");
        return 1;
    }

    // open answer file
    char *answerFilename = ANSWER_FILENAME;
    FILE *answerFile;
    answerFile = fopen(answerFilename, "rb");

    if (!answerFile)
    {
        perror("[ERROR] ");
        return 1;
    }

    answers_file_header answerHeader = { .Seed = 0, .ExpectedSum = 0 };
    fread(&answerHeader, sizeof(answers_file_header), 1, answerFile);

    // allocate memory arena for token stack
    memory_arena tokenArena;
    U8 *memoryPtr = calloc(1, Megabytes(1));
    if (!memoryPtr)
    {
        printf("[ERROR] Unable to allocate memory for token stack");
        exit(1);
    }
    ArenaInitialize(&tokenArena, Megabytes(1), memoryPtr);

#if HAVERSINE_SLOW
    // Note (Aaron): Fill memory with 1s for debug purposes
    MemorySet(memoryPtr, 0xff, Megabytes(1));
#endif

    token_stack tokenStack;
    InitializeTokenStack(&tokenStack, &tokenArena);

    processor_stats stats;
    InitializeProcessorStats(&stats);
    stats.ExpectedSum = answerHeader.ExpectedSum;

    pairs_context context;
    InitializePairsContext(&context);

    EndCPUTimingAndIncrementMetric(&metrics._sub_timings.Startup);

    for (;;)
    {
        StartCPUTiming(&metrics._sub_timings.JSONLexing);
        haversine_token nextToken = GetNextToken(dataFile);
        EndCPUTimingAndIncrementMetric(&metrics._sub_timings.JSONLexing);

        StartCPUTiming(&metrics._sub_timings.MiscOperations);
        stats.TokenCount++;
        stats.MaxTokenLength = nextToken.Length > stats.MaxTokenLength
            ? nextToken.Length
            : stats.MaxTokenLength;
        EndCPUTimingAndIncrementMetric(&metrics._sub_timings.MiscOperations);

#if 0
        printf("[INFO] %lli: ", stats.TokenCount);
        PrintToken(&nextToken);
#endif

        StartCPUTiming(&metrics._sub_timings.JSONParsing);
        if (nextToken.Type == Token_EOF)
        {
#if 0
            printf("\n");
            printf("[INFO] EOF reached\n");
#endif
            EndCPUTimingAndIncrementMetric(&metrics._sub_timings.JSONParsing);
            break;
        }

        // skip tokens we aren't (currently) interested in
        if (nextToken.Type == Token_assignment
            || nextToken.Type == Token_delimiter
            || nextToken.Type == Token_scope_open
            || nextToken.Type == Token_scope_close)
        {
            EndCPUTimingAndIncrementMetric(&metrics._sub_timings.JSONParsing);
            continue;
        }

        haversine_token *tokenPtr = PushToken(&tokenStack, nextToken);

        if (!context.PairsToken
            && nextToken.Type == Token_identifier
            && strcmp(nextToken.String, "\"pairs\"") == 0)
        {
            context.PairsToken = tokenPtr;
        }

        // parse the pairs token
        if (context.PairsToken)
        {
            // parse until we reach the pairs array start token
            if (!context.ArrayStartToken && nextToken.Type == Token_array_start)
            {
                context.ArrayStartToken = tokenPtr;
                EndCPUTimingAndIncrementMetric(&metrics._sub_timings.JSONParsing);
                continue;
            }

            if (!context.ArrayStartToken)
            {
                EndCPUTimingAndIncrementMetric(&metrics._sub_timings.JSONParsing);
                continue;
            }

            // clean up context and token stack when we reach the end of the pairs array
            if (tokenPtr->Type == Token_array_end)
            {
                PopToken(&tokenStack);
                PopToken(&tokenStack);

                context.ArrayStartToken = 0;
                context.PairsToken = 0;
                EndCPUTimingAndIncrementMetric(&metrics._sub_timings.JSONParsing);
                continue;
            }

            // load up token pointers in the context until we fill all required tokens
            if (tokenPtr->Type == Token_identifier
                && strcmp(tokenPtr->String, "\"x0\"") == 0)
            {
                if (context.X0Token)
                {
                    printf("[ERROR] Duplicate x0 identifier encountered in Haversine pair (%lli)\n", stats.PairsProcessed);
                    exit(1);
                }
                context.X0Token = tokenPtr;
                EndCPUTimingAndIncrementMetric(&metrics._sub_timings.JSONParsing);
                continue;
            }

            if (tokenPtr->Type == Token_identifier
                && strcmp(tokenPtr->String, "\"y0\"") == 0)
            {
                if (context.Y0Token)
                {
                    printf("[ERROR] Duplicate y0 identifier encountered in Haversine pair (%lli)\n", stats.PairsProcessed);
                    exit(1);
                }
                context.Y0Token = tokenPtr;
                EndCPUTimingAndIncrementMetric(&metrics._sub_timings.JSONParsing);
                continue;
            }

            if (tokenPtr->Type == Token_identifier
                && strcmp(tokenPtr->String, "\"x1\"") == 0)
            {
                if (context.X1Token)
                {
                    printf("[ERROR] Duplicate x1 identifier encountered in Haversine pair (%lli)\n", stats.PairsProcessed);
                    exit(1);
                }
                context.X1Token = tokenPtr;
                EndCPUTimingAndIncrementMetric(&metrics._sub_timings.JSONParsing);
                continue;
            }

            if (tokenPtr->Type == Token_identifier
                && strcmp(tokenPtr->String, "\"y1\"") == 0)
            {
                if (context.Y1Token)
                {
                    printf("[ERROR] Duplicate y1 identifier encountered in Haversine pair (%lli)\n", stats.PairsProcessed);
                    exit(1);
                }
                context.Y1Token = tokenPtr;
                EndCPUTimingAndIncrementMetric(&metrics._sub_timings.JSONParsing);
                continue;
            }

            EndCPUTimingAndIncrementMetric(&metrics._sub_timings.JSONParsing);

            // process a Haversine point pair once we have parsed its values
            if (context.X0Token && context.Y0Token && context.X1Token && context.Y1Token)
            {
                StartCPUTiming(&metrics._sub_timings.StackOperations);
                haversine_token y1Value = PopToken(&tokenStack);
                PopToken(&tokenStack);
                haversine_token x1Value = PopToken(&tokenStack);
                PopToken(&tokenStack);
                haversine_token y0Value = PopToken(&tokenStack);
                PopToken(&tokenStack);
                haversine_token x0Value = PopToken(&tokenStack);
                PopToken(&tokenStack);

                Assert(x0Value.Type == Token_value
                       && y0Value.Type == Token_value
                       && x1Value.Type == Token_value
                       && y1Value.Type == Token_value);

                context.X0Token = 0;
                context.Y0Token = 0;
                context.X1Token = 0;
                context.Y1Token = 0;

                V2F64 point0 = GetVectorFromCoordinateTokens(x0Value, y0Value);
                V2F64 point1 = GetVectorFromCoordinateTokens(x1Value, y1Value);
                EndCPUTimingAndIncrementMetric(&metrics._sub_timings.StackOperations);

                StartCPUTiming(&metrics._sub_timings.HaversineDistance);
                F64 distance = ReferenceHaversine(point0.x, point0.y, point1.x, point1.y, EARTH_RADIUS);
                EndCPUTimingAndIncrementMetric(&metrics._sub_timings.HaversineDistance);

#if 0
                PrintHaversineDistance(point0, point1, distance);
#endif

                StartCPUTiming(&metrics._sub_timings.Validation);
                F64 answerDistance;
                fread(&answerDistance, sizeof(F64), 1, answerFile);

                F64 difference = answerDistance - distance;
                difference = difference >= 0 ? difference : difference * -1;
                if (difference > EPSILON_FLOAT)
                {
                    stats.CalculationErrors++;
                    printf("[WARN] Calculated distance diverges from answer value significantly (calculated: %f vs. answer: %f)\n", distance, answerDistance);
                }
                EndCPUTimingAndIncrementMetric(&metrics._sub_timings.Validation);

                StartCPUTiming(&metrics._sub_timings.SumCalculation);
                stats.CalcualtedSum = ((stats.CalcualtedSum * (F64)stats.PairsProcessed) + distance) / (F64)(stats.PairsProcessed + 1);
                stats.PairsProcessed++;
                EndCPUTimingAndIncrementMetric(&metrics._sub_timings.SumCalculation);

                continue;
            }
        }
    }

    StartCPUTiming(&metrics._sub_timings.MiscOperations);
    if (ferror(dataFile))
    {
        fclose(dataFile);
        perror("[ERROR] ");
        Assert(FALSE);

        return 1;
    }
    fclose(dataFile);

    if (ferror(answerFile))
    {
        fclose(answerFile);
        perror("[ERROR] ");
        Assert(FALSE);

        exit(1);
    }
    fclose(answerFile);

    printf("\n");
    PrintStats(&stats);
    printf("\n");
    EndCPUTimingAndIncrementMetric(&metrics._sub_timings.MiscOperations);

    // Note (Aaron): As above, avoid using EndCPUTimingAndIncrementMetric() on Total timing
    // and calculate the duration below so we can disable the timings methods and get a sense
    // for how much taking the timings affects the execution time.
    metrics.Total.End = ReadCPUTimer();
    metrics.Total.Duration = metrics.Total.End - metrics.Total.Start;
    F64 totalTimeMs = ((F64)metrics.Total.Duration / (F64)metrics.CPUFrequency) * 1000.0f;

#if ENABLE_TIMINGS
    S64 unaccounted = metrics.Total.Duration;
    for (int i = 0; i < ArrayCount(metrics.Subtimings); ++i)
    {
        unaccounted -= metrics.Subtimings[i].Duration;
    }

    Assert((unaccounted > 0) && "Subtracted more than the total time");

    printf("Timings:\n");
    PrintTiming("Startup", metrics._sub_timings.Startup.Duration, metrics.Total.Duration, 2);
    PrintTiming("JSON Lexing", metrics._sub_timings.JSONLexing.Duration, metrics.Total.Duration, 2);
    PrintTiming("JSON Parsing", metrics._sub_timings.JSONParsing.Duration, metrics.Total.Duration, 2);
    PrintTiming("Stack Operations", metrics._sub_timings.StackOperations.Duration, metrics.Total.Duration, 1);
    PrintTiming("Haversine Distance", metrics._sub_timings.HaversineDistance.Duration, metrics.Total.Duration, 1);
    PrintTiming("Validation", metrics._sub_timings.Validation.Duration, metrics.Total.Duration, 2);
    PrintTiming("Sum Calculation", metrics._sub_timings.SumCalculation.Duration, metrics.Total.Duration, 1);
    PrintTiming("Misc Operations", metrics._sub_timings.MiscOperations.Duration, metrics.Total.Duration, 1);
    PrintTiming("Unaccounted", unaccounted, metrics.Total.Duration, 2);
    printf("\n");
#endif

    printf("  Total time: %.4fms (CPU freq %llu)\n", totalTimeMs, metrics.CPUFrequency);
    printf("\n");

    return 0;
}
