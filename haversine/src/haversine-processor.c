/*  TODO (Aaron):
    - Reduce the number of timings created so the profiler doesn't skew the runtime too much
        - Original:
            - Total time with profiler disabled: ~7200ms
            - Total time with profiler enabled: ~8400ms
        - Read file contents at once:
            - Total time with profiler disabled: ~4100ms
            - Total time with profiler enabled: ~5600ms

    - Lex / Parse JSON all at once
        - Add timing for lexing / parsing
*/

#pragma warning(disable:4996)


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define PROFILER 1
#define PLATFORM_METRICS_IMPLEMENTATION
#include "platform_metrics.h"

#include "base_inc.h"
#include "haversine.h"
#include "haversine_lexer.h"
#include "base_types.c"
#include "base_memory.c"
#include "base_string.c"
#include "haversine.c"
#include "haversine_lexer.c"


#define EPSILON_FLOAT 0.001


typedef struct token_stack token_stack;
struct token_stack
{
    memory_arena *Arena;
    S64 TokenCount;
};


typedef struct processor_stats processor_stats;
struct processor_stats
{
    U64 TokenCount;
    U64 MaxTokenLength;
    S64 PairsProcessed;
    U64 CalculationErrors;
    F64 ExpectedSum;
    F64 CalcualtedSum;
};


typedef struct pairs_context pairs_context;
struct pairs_context
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
    struct __stat64 stat;
    _stat64(filename, &stat);
#else
    struct stat stat;
    stat(filename, &stat);
#endif

    memory_index fileSize = stat.st_size;
    U8 *basePtr = (U8 *)malloc(fileSize);

    if (!basePtr)
    {
        printf("[ERROR] Unable to allocate memory for \"%s\"\n", filename);
        free(basePtr);
        fclose(file);
        Assert(FALSE);

        return result;
    }

    if(fread(basePtr, fileSize, 1, file) != 1)
    {
        fprintf(stderr, "[ERROR] Unable to read file \"%s\"\n", filename);
        free(basePtr);
        fclose(file);
        Assert(FALSE);

        return result;
    }

    fclose(file);

    ArenaInitialize(&result, fileSize, basePtr);
    result.Used = fileSize;
    // Note (Aaron): We keep the arena's position pointer at the base pointer so we can use it as a read head for parsing the data.

    return result;
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
    MemorySet(tokenPtr, 0xff, sizeof(haversine_token));
#endif

    // TODO (Aaron): Error handling?

    return result;
}


global_function V2F64 GetVectorFromCoordinateTokens(haversine_token xValue, haversine_token yValue)
{
    char buffer[MAX_TOKEN_LENGTH] = {0};
    V2F64 result = { .x = 0, .y = 0};

    MemoryCopy(buffer, xValue.String.Str, xValue.String.Length);
    char *xEndPtr = buffer + xValue.String.Length;
    result.x = strtod(buffer, &xEndPtr);

    MemoryCopy(buffer, yValue.String.Str, yValue.String.Length);
    MemoryZero(buffer + yValue.String.Length + 1, 1);
    char *yEndPtr = buffer + yValue.String.Length;
    result.y = strtod(buffer, &yEndPtr);

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
    char *buffer[MAX_TOKEN_LENGTH] = {0};
    MemoryCopy(buffer, token->String.Str, token->String.Length);

    printf("%s:\t\t%s\n",
           GetTokenMenemonic(token->Type),
           (char *)buffer);
}


global_function void PrintHaversineDistance(V2F64 point0, V2F64 point1, F64 distance)
{
    printf("[INFO] Haversine distance: (%f, %f) -> (%f, %f) = %f\n", point0.x, point0.y, point1.x, point1.y, distance);
}


int main()
{
    StartTimingsProfile();
    START_TIMING(Startup);

    // read data file
    START_TIMING(ReadJSONFile)
    char *dataFilename = DATA_FILENAME;
    printf("[INFO] Processing file '%s'\n", dataFilename);
    memory_arena jsonContents = ReadFileContents(dataFilename);
    if (!jsonContents.BasePtr)
    {
        perror("[ERROR] ");
        return 1;
    }
    END_TIMING(ReadJSONFile)

    // read answer file
    START_TIMING(ReadAnswerFile)
    char *answerFilename = ANSWER_FILENAME;
    printf("[INFO] Processing file '%s'\n", answerFilename);
    memory_arena answerContents = ReadFileContents(answerFilename);
    if (!answerContents.BasePtr)
    {
        perror("[ERROR] ");
        return 1;
    }
    END_TIMING(ReadAnswerFile)

    answers_file_header answerHeader = { .Seed = 0, .ExpectedSum = 0 };
    MemoryCopy(&answerHeader, answerContents.PositionPtr, sizeof(answers_file_header));
    answerContents.PositionPtr += sizeof(answers_file_header);

    // allocate memory for the token bucket
    // TODO (Aaron): Ideally would use a growable arena for this. This current heuristic doesn't work.
    // memory_index tokenBucketSize = jsonContents.Size * 2;
    // U8 *tokenBucketPtr = (U8 *)malloc(tokenBucketSize);
    // if (!tokenBucketPtr)
    // {
    //     printf("[ERROR] Unable to allocate memory for token bucket");
    //     exit(1);
    // }
    // memory_arena tokenBucket;
    // ArenaInitialize(&tokenBucket, tokenBucketSize, tokenBucketPtr);

    // allocate memory arena for token stack
    U8 *memoryPtr = calloc(1, Megabytes(1));
    if (!memoryPtr)
    {
        printf("[ERROR] Unable to allocate memory for token stack");
        exit(1);
    }

    memory_arena tokenArena;
    ArenaInitialize(&tokenArena, Megabytes(1), memoryPtr);
    token_stack tokenStack = {0};
    tokenStack.Arena = &tokenArena;

    pairs_context context = {0};
    processor_stats stats = {0};
    stats.ExpectedSum = answerHeader.ExpectedSum;

    END_TIMING(Startup);

    for (;;)
    {
        START_TIMING(JSONLexing);
        haversine_token nextToken = GetNextToken(&jsonContents);

        // TODO (Aaron): Can't push tokens into their own bucket as the size of the struct is just too large and there's no good heuristic I can' think of that maps from the raw JSON to number of tokens
        //  - Str8 still takes up 72bytes of space, largely because of the Length member
        //  - This means our "intermediate" representation is quite large compared to the input
        //  - And we use more cycles copying CStrings into buffers so they can have a null terminator
        //  - Hmmm...
        //  - Without growable arenas, I don't think there's a way to know how large our token bucket needs to be
        //  - And there's no real benefit to parsing the JSON in a separate pass from the lexing
        //  - Some options:
        //      - Stick with arenas and implement growable arenas (don't want to do this yet)
        //      - Stick with arenas and use VirtAlloc (and whatever the linux equivalent is) and allocate a massive ammount of memory so we can just use all the memory (not opposed to this, really. It'll just page memory in and out)
        //      - Leave things the way they are with CStrings and do the lexing and parsing all in one step
        //      - Re-do the JSON parsing so we end up with an intermediate representation so we can crank through the haversine pairs separately

        // ArenaPushData(&tokenBucket, sizeof(haversine_token), (U8 *)&nextToken);

        END_TIMING(JSONLexing);

        START_TIMING(UpdateTokenStats);
        stats.TokenCount++;
        stats.MaxTokenLength = nextToken.String.Length > stats.MaxTokenLength
            ? nextToken.String.Length
            : stats.MaxTokenLength;
        END_TIMING(UpdateTokenStats);

#if 0
        printf("[INFO] %lli: ", stats.TokenCount);
        PrintToken(&nextToken);
#endif

        START_TIMING(JSONParsing);
        if (nextToken.Type == Token_EOF)
        {
#if 0
            printf("\n");
            printf("[INFO] EOF reached\n");
#endif
            END_TIMING(JSONParsing);
            break;
        }

        // skip token types we aren't (currently) interested in
        if (nextToken.Type == Token_assignment
            || nextToken.Type == Token_delimiter
            || nextToken.Type == Token_scope_open
            || nextToken.Type == Token_scope_close)
        {
            END_TIMING(JSONParsing);
            continue;
        }

        haversine_token *tokenPtr = PushToken(&tokenStack, nextToken);

        if (!context.PairsToken
            && nextToken.Type == Token_identifier
            && CompareStr8(nextToken.String, Str8CString((U8 *)"\"pairs\""), TRUE))
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
                END_TIMING(JSONParsing);
                continue;
            }

            if (!context.ArrayStartToken)
            {
                END_TIMING(JSONParsing);
                continue;
            }

            // clean up context and token stack when we reach the end of the pairs array
            if (tokenPtr->Type == Token_array_end)
            {
                PopToken(&tokenStack);
                PopToken(&tokenStack);

                context.ArrayStartToken = 0;
                context.PairsToken = 0;
                END_TIMING(JSONParsing);
                continue;
            }

            // load up token pointers in the context until we fill all required tokens
            if (tokenPtr->Type == Token_identifier
                && CompareStr8(tokenPtr->String, Str8CString((U8 *)"\"x0\""), TRUE))
            {
                if (context.X0Token)
                {
                    printf("[ERROR] Duplicate x0 identifier encountered in Haversine pair (%lli)\n", stats.PairsProcessed);
                    exit(1);
                }
                context.X0Token = tokenPtr;
                END_TIMING(JSONParsing);
                continue;
            }

            if (tokenPtr->Type == Token_identifier
                && CompareStr8(tokenPtr->String, Str8CString((U8 *)"\"y0\""), TRUE))
            {
                if (context.Y0Token)
                {
                    printf("[ERROR] Duplicate y0 identifier encountered in Haversine pair (%lli)\n", stats.PairsProcessed);
                    exit(1);
                }
                context.Y0Token = tokenPtr;
                END_TIMING(JSONParsing);
                continue;
            }

            if (tokenPtr->Type == Token_identifier
                && CompareStr8(tokenPtr->String, Str8CString((U8 *)"\"x1\""), TRUE))
            {
                if (context.X1Token)
                {
                    printf("[ERROR] Duplicate x1 identifier encountered in Haversine pair (%lli)\n", stats.PairsProcessed);
                    exit(1);
                }
                context.X1Token = tokenPtr;
                END_TIMING(JSONParsing);
                continue;
            }

            if (tokenPtr->Type == Token_identifier
                && CompareStr8(tokenPtr->String, Str8CString((U8 *)"\"y1\""), TRUE))
            {
                if (context.Y1Token)
                {
                    printf("[ERROR] Duplicate y1 identifier encountered in Haversine pair (%lli)\n", stats.PairsProcessed);
                    exit(1);
                }
                context.Y1Token = tokenPtr;
                END_TIMING(JSONParsing);
                continue;
            }

            END_TIMING(JSONParsing);

            // process a Haversine point pair once we have parsed its values
            if (context.X0Token && context.Y0Token && context.X1Token && context.Y1Token)
            {
                START_TIMING(StackOperations);
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
                END_TIMING(StackOperations);

                START_TIMING(HaversineDistance);
                F64 distance = ReferenceHaversine(point0.x, point0.y, point1.x, point1.y, EARTH_RADIUS);
                END_TIMING(HaversineDistance);

#if 0
                PrintHaversineDistance(point0, point1, distance);
#endif

                START_TIMING(Validation);
                F64 answerDistance;
                MemoryCopy(&answerDistance, answerContents.PositionPtr, sizeof(F64));
                answerContents.PositionPtr += sizeof(F64);

                F64 difference = answerDistance - distance;
                difference = difference >= 0 ? difference : difference * -1;
                if (difference > EPSILON_FLOAT)
                {
                    stats.CalculationErrors++;
                    printf("[WARN] Calculated distance diverges from answer value significantly (calculated: %f vs. answer: %f)\n", distance, answerDistance);
                }
                END_TIMING(Validation);

                START_TIMING(SumCalculation);
                stats.CalcualtedSum = ((stats.CalcualtedSum * (F64)stats.PairsProcessed) + distance) / (F64)(stats.PairsProcessed + 1);
                stats.PairsProcessed++;
                END_TIMING(SumCalculation);

                continue;
            }
        }
        else
        {
            END_TIMING(JSONParsing);
        }
    }


    printf("\n");
    PrintStats(&stats);
    printf("\n");

    EndTimingsProfile();
    PrintProfileTimings();

    return 0;
}

ProfilerEndOfCompilationUnit
