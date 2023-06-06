/*  TODO (Aaron):
    - Optionally load and consume the binary "answers" file for validation
        - Compare expected sum with value read out of the answers file
*/

#pragma warning(disable:4996)

#include "base.h"
#include "haversine.h"
#include "haversine_lexer.h"
#include "memory_arena.h"

#include "haversine.c"
#include "haversine_lexer.c"
#include "memory_arena.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define EPSILON_FLOAT 0.01


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


function void InitializeTokenStack(token_stack *stack, memory_arena *arena)
{
    stack->Arena = arena;
    stack->TokenCount = 0;
}


function void InitializeProcessorStats(processor_stats *stats)
{
    stats->TokenCount = 0;
    stats->MaxTokenLength = 0;
    stats->PairsProcessed = 0;
}


function void InitializePairsContext(pairs_context *context)
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


function haversine_token *PushToken(token_stack *tokenStack, haversine_token value)
{
    haversine_token *tokenPtr = PushSize(tokenStack->Arena, haversine_token);
    tokenStack->TokenCount++;
    MemoryCopy(tokenPtr, &value, sizeof(haversine_token));

    // TODO (Aaron): Error handling?

    return tokenPtr;
}


function haversine_token PopToken(token_stack *tokenStack)
{
    haversine_token result;
    haversine_token *tokenPtr = PopSize(tokenStack->Arena, haversine_token);
    tokenStack->TokenCount--;

    MemoryCopy(&result, tokenPtr, sizeof(haversine_token));

#if HAVERSINE_SLOW
    MemorySet((U8 *)tokenPtr, 0xff, sizeof(haversine_token));
#endif

    // TODO (Aaron): Error handling?

    return result;
}


function V2F64 GetVectorFromCoordinateTokens(haversine_token xValue, haversine_token yValue)
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


function void PrintStats(processor_stats *stats)
{
    printf("[INFO] Tokens processed:    %lli\n", stats->TokenCount);
    printf("[INFO] Max token length:    %lli\n", stats->MaxTokenLength);
    printf("[INFO] Pairs processed:     %lli\n", stats->PairsProcessed);
    printf("[INFO] Calculation errors:  %lli\n", stats->CalculationErrors);
}


function void PrintToken(haversine_token *token)
{
    printf("%s:\t\t%s\n",
           GetTokenMenemonic(token->Type),
           token->String);
}


function void PrintHaversineDistance(V2F64 point0, V2F64 point1, F64 distance)
{
    printf("[INFO] Haversine distance: (%f, %f) -> (%f, %f) = %f\n", point0.x, point0.y, point1.x, point1.y, distance);
}


int main()
{
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

    // allocate memory arena for token stack
    memory_arena tokenArena;
    U8 *memoryPtr = calloc(1, Megabytes(1));
    if (!memoryPtr)
    {
        printf("[ERROR] Unable to allocate memory for token stack");
        exit(1);
    }
    InitializeArena(&tokenArena, Megabytes(1), memoryPtr);

#if HAVERSINE_SLOW
    // Note (Aaron): Fill memory with 1s for debug purposes
    MemorySet(memoryPtr, 0xff, Megabytes(1));
#endif

    token_stack tokenStack;
    InitializeTokenStack(&tokenStack, &tokenArena);

    processor_stats stats;
    InitializeProcessorStats(&stats);

    pairs_context context;
    InitializePairsContext(&context);

    for (;;)
    {
        haversine_token nextToken = GetNextToken(dataFile);

        stats.TokenCount++;
        stats.MaxTokenLength = nextToken.Length > stats.MaxTokenLength
            ? nextToken.Length
            : stats.MaxTokenLength;

#if 0
        printf("[INFO] %lli: ", stats.TokenCount);
        PrintToken(&nextToken);
#endif

        if (nextToken.Type == Token_EOF)
        {
#if 0
            printf("\n");
            printf("[INFO] EOF reached\n");
#endif
            break;
        }

        // skip tokens we aren't (currently) interested in
        if (nextToken.Type == Token_assignment
            || nextToken.Type == Token_delimiter
            || nextToken.Type == Token_scope_open
            || nextToken.Type == Token_scope_close)
        {
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
                continue;
            }

            if (!context.ArrayStartToken)
            {
                continue;
            }

            // clean up context and token stack when we reach the end of the pairs array
            if (tokenPtr->Type == Token_array_end)
            {
                PopToken(&tokenStack);
                PopToken(&tokenStack);

                context.ArrayStartToken = 0;
                context.PairsToken = 0;
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
                continue;
            }

            // process a Haversine point pair once we have parsed its values
            if (context.X0Token && context.Y0Token && context.X1Token && context.Y1Token)
            {
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

                F64 distance = ReferenceHaversine(point0.x, point0.y, point1.x, point1.y, EARTH_RADIUS);
                PrintHaversineDistance(point0, point1, distance);

                F64 answerDistance;
                fread(&answerDistance, sizeof(F64), 1, answerFile);

                F64 difference = answerDistance - distance;
                difference = difference > 0 ? difference : difference * -1;
                if (difference > EPSILON_FLOAT)
                {
                    stats.CalculationErrors++;
                    printf("[WARN] Calculated distance diverges from answer value significantly (calculated: %f vs. answer: %f)\n", distance, answerDistance);
                }

                stats.ExpectedSum = ((stats.ExpectedSum * (F64)stats.PairsProcessed) + distance) / (F64)(stats.PairsProcessed + 1);
                stats.PairsProcessed++;

                continue;
            }
        }
    }

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

    return 0;
}
