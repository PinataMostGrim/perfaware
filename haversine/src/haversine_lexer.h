/* TODO (Aaron):
    - Consider how to best pick size for MAX_TOKEN_LENGTH
    - Specify an int size for token_type enum?
*/

#ifndef HAVERSINE_LEXER_H
#define HAVERSINE_LEXER_H

#include <stdio.h>

#include "base_types.h"


#define MAX_TOKEN_LENGTH 32


typedef enum
{
    Token_invalid,          //
    Token_identifier,       // string
    Token_value,            // signed int
    Token_assignment,       // :
    Token_delimiter,        // ,
    Token_scope_open,       // {
    Token_scope_close,      // }
    Token_array_start,      // [
    Token_array_end,        // ]
    Token_EOF,              //

    Token_type_count,
} token_type;


typedef struct haversine_token haversine_token;
struct haversine_token
{
    token_type Type;
    char String[MAX_TOKEN_LENGTH];
    U32 Length;
};


global_function const char *GetTokenMenemonic(token_type tokenType);
global_function int EatNextCharacter(FILE *file);
global_function int PeekNextCharacter(FILE *file);
global_function B8 IsFloatingPointChar(char character);
global_function haversine_token GetNextToken(FILE *file);

#endif // HAVERSINE_LEXER_H
