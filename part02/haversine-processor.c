/*  TODO (Aaron):
    - Use a token stack to parse through the "pairs" array
    - Optionally load and consume the binary "answers" file for validation
*/

#pragma warning(disable:4996)

#include "base.h"
#include "haversine.c"
#include "haversine_lexer.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct
{
    S64 TokenCount;
    S64 MaxTokenLength;
} processor_stats;


typedef size_t memory_index;

typedef struct
{
    U8 *BasePtr;
    U8 *PosPtr;
    memory_index Size;
    memory_index Used;
} memory_arena;


function void InitializeArena(memory_arena *arena, memory_index size, U8 *basePtr)
{
    arena->BasePtr = basePtr;
    arena->PosPtr = basePtr;
    arena->Size = size;
    arena->Used = 0;
}


function void *PushSize_(memory_arena *arena, memory_index size)
{
    Assert((arena->Used + size) <= arena->Size);

    void *result = arena->BasePtr + arena->Used;
    arena->Used += size;
    arena->PosPtr = arena->BasePtr + arena->Used;

    return result;
}


function void *PopSize_(memory_arena *arena, memory_index size)
{
    Assert(arena->Used >= size);

    arena->Used -= size;
    arena->PosPtr = arena->BasePtr + arena->Used;

    void *result = arena->BasePtr + arena->Used;
    return result;
}


#define PushSize(arena, type) (type *)PushSize_(arena, sizeof(type))
#define PopSize(arena, type) (type *)PopSize_(arena, sizeof(type))


function token *PushToken(memory_arena *tokenStack, token value)
{
    token *tokenPtr = PushSize(tokenStack, token);
    memcpy(tokenPtr, &value, sizeof(token));

    // TODO (Aaron): Error handling?

    return tokenPtr;
}


function token PopToken(memory_arena *arena)
{
    token result;
    token *tokenPtr = PopSize(arena, token);

    memcpy(&result, tokenPtr, sizeof(token));

#if HAVERSINE_SLOW
    MemorySet((U8 *)tokenPtr, 0xff, sizeof(token));
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
        token nextToken = GetNextToken(dataFile);
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

        token *tokenPtr = PushToken(&tokenStack, nextToken);
        token poppedToken = PopToken(&tokenStack);
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
