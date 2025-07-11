#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "base_inc.h"
#include "haversine.h"
#include "reference_haversine_lexer.h"
#include "reference_haversine_parser.h"


// Note (Aaron): Returns the max size in bytes a memory_arena needs to be in order to hold
// haversine pairs parsed from JSON.
static memory_index GetMaxPairsSize(memory_index jsonSize)
{
    U32 minimumJSONPairSize = 6*4;      // 5 alpha chars + 1 numeric char (e.g. "x0":1)

    // 'jsonSize' will not always be cleanly divided by minimumJSONPairSize so we need to
    // account for fractions.
    U64 maxPairCount = jsonSize / minimumJSONPairSize;
    U64 remainder = jsonSize % minimumJSONPairSize;
    if (remainder)
    {
        maxPairCount++;
    }
    memory_index pairsArenaSize = maxPairCount * sizeof(haversine_pair);

    return pairsArenaSize;
}


static haversine_token *PushToken(token_stack *tokenStack, haversine_token value)
{
    haversine_token *tokenPtr = ArenaPushStruct(tokenStack->Arena, haversine_token);
    tokenStack->TokenCount++;
    MemoryCopy(tokenPtr, &value, sizeof(haversine_token));

    // TODO (Aaron): Error handling?

    return tokenPtr;
}


static haversine_token PopToken(token_stack *tokenStack)
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


static V2F64 GetVectorFromCoordinateTokens(haversine_token xValue, haversine_token yValue)
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


static parsing_stats ParseHaversinePairs(memory_arena *jsonContents, memory_arena *pairsArena, memory_arena *tokenArena)
{
    parsing_stats stats = {0};
    pairs_context context = {0};

    token_stack tokenStack = {0};
    tokenStack.Arena = tokenArena;

    for (;;)
    {
        haversine_token nextToken = GetNextToken(jsonContents);

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

        // skip token types we aren't (currently) interested in
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
                    // TODO (Aaron): Handle these errors differently
                    // Save the error into the stats struct?
                    // Print the error if we return early?
                    printf("[ERROR] Duplicate x0 identifier encountered in Haversine pair (%" PRIu64")\n", stats.PairsProcessed);
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
                    printf("[ERROR] Duplicate y0 identifier encountered in Haversine pair (%" PRIu64")\n", stats.PairsProcessed);
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
                    printf("[ERROR] Duplicate x1 identifier encountered in Haversine pair (%" PRIu64")\n", stats.PairsProcessed);
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
                    printf("[ERROR] Duplicate y1 identifier encountered in Haversine pair (%" PRIu64")\n", stats.PairsProcessed);
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

                haversine_pair *pair = ArenaPushStruct(pairsArena, haversine_pair);
                pair->point0 = GetVectorFromCoordinateTokens(x0Value, y0Value);
                pair->point1 = GetVectorFromCoordinateTokens(x1Value, y1Value);

                stats.PairsParsed++;
                continue;
            }
        }
    }

    return stats;
}
