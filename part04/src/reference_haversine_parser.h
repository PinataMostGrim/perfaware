#ifndef HAVERSINE_PARSER_H
#define HAVERSINE_PARSER_H

#include "base_inc.h"
#include "reference_haversine_lexer.h"


typedef struct parsing_stats parsing_stats;
struct parsing_stats
{
    U64 TokenCount;
    U64 MaxTokenLength;
    U64 ExpectedPairCount;
    F64 ExpectedSum;
    U64 PairsParsed;
    U64 PairsProcessed;
    F64 CalculatedSum;
    U64 CalculationErrors;
    F64 SumDivergence;
};

typedef struct token_stack token_stack;
struct token_stack
{
    memory_arena *Arena;
    S64 TokenCount;
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


global_function memory_index GetMaxPairsSize(memory_index jsonSize);
global_function haversine_token *PushToken(token_stack *tokenStack, haversine_token value);
global_function haversine_token PopToken(token_stack *tokenStack);
global_function V2F64 GetVectorFromCoordinateTokens(haversine_token xValue, haversine_token yValue);
global_function parsing_stats ParseHaversinePairs(memory_arena *jsonContents, memory_arena *pairsArena, memory_arena *tokenArena);

#endif // HAVERSINE_PARSER_H
