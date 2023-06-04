/*  TODO (Aaron):
    - Use a token stack to parse through the "pairs" array
    - Optionally load and consume the binary "answers" file for validation
    - Replace memcpy with hand-rolled version in base.h
    - Consider making a stack typedef instead of using memory_arena directly
*/

#pragma warning(disable:4996)

#include "base.h"
#include "haversine.c"
#include "haversine_lexer.c"
#include "memory_arena.c"

#include <stdio.h>
#include <stdlib.h>


typedef struct
{
    S64 TokenCount;
    S64 MaxTokenLength;
} processor_stats;


function haversine_token *PushToken(memory_arena *tokenStack, haversine_token value)
{
    haversine_token *tokenPtr = PushSize(tokenStack, haversine_token);
    MemoryCopy(tokenPtr, &value, sizeof(haversine_token));

    // TODO (Aaron): Error handling?

    return tokenPtr;
}


function haversine_token PopToken(memory_arena *arena)
{
    haversine_token result;
    haversine_token *tokenPtr = PopSize(arena, haversine_token);

    MemoryCopy(&result, tokenPtr, sizeof(haversine_token));

#if HAVERSINE_SLOW
    MemorySet((U8 *)tokenPtr, 0xff, sizeof(haversine_token));
#endif

    // TODO (Aaron): Error handling?

    return result;
}


function void PrintStats(processor_stats stats)
{
    printf("[INFO] Stats:\n");
    printf("[INFO]  Tokens processed: '%lli'\n", stats.TokenCount);
    printf("[INFO]  Max token length: '%lli'\n", stats.MaxTokenLength);
    printf("\n");
}


int main()
{
    char *dataFilename = DATA_FILENAME;
    FILE *dataFile;
    dataFile = fopen(dataFilename, "r");

    printf("[INFO] Processing file '%s'\n", dataFilename);

    if (!dataFile)
    {
        perror("[ERROR] ");
        return 1;
    }

    // allocate memory arena for token stack
    memory_arena tokenStack;
    U8 *memoryPtr = calloc(1, Megabytes(1));
    if (!memoryPtr)
    {
        printf("[ERROR] Unable to allocate memory for token stack");
        exit(1);
    }

    InitializeArena(&tokenStack, Megabytes(1), memoryPtr);
#if HAVERSINE_SLOW
    // Note (Aaron): Fill memory with 1s for debug purposes
    MemorySet(memoryPtr, 0xff, Megabytes(1));
#endif

    processor_stats stats =
    {
        .TokenCount = 0,
        .MaxTokenLength = 0,
    };

    for (;;)
    {
        haversine_token nextToken = GetNextToken(dataFile);
        stats.TokenCount++;

        printf("Token type: %s \t|  Token value: %s \t\t|  Token length: %i\n",
               GetTokenMenemonic(nextToken.Type),
               nextToken.String,
               nextToken.Length);

        stats.MaxTokenLength = nextToken.Length > stats.MaxTokenLength
            ? nextToken.Length
            : stats.MaxTokenLength;

        if (nextToken.Type == Token_EOF)
        {
            printf("\n");
            printf("[INFO] Reached end of file\n");
            break;
        }

        // TODO (Aaron): Parse tokens

        haversine_token *tokenPtr = PushToken(&tokenStack, nextToken);
        haversine_token poppedToken = PopToken(&tokenStack);
    }

    if (ferror(dataFile))
    {
        fclose(dataFile);
        perror("[ERROR] ");
        return 1;
    }

    fclose(dataFile);

    PrintStats(stats);

    return 0;
}
