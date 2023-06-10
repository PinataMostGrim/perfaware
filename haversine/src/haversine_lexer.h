/* TODO (Aaron):
    - Consider how to best pick size for MAX_TOKEN_LENGTH
    - Specify an int size for token_type enum?
*/

#ifndef HAVERSINE_LEXER_H
#define HAVERSINE_LEXER_H

#include "base.h"

#include <stdio.h>


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


typedef struct
{
    token_type Type;
    char String[MAX_TOKEN_LENGTH];
    U32 Length;
} haversine_token;


function const char *GetTokenMenemonic(token_type tokenType);
function int EatNextCharacter(FILE *file);
function int PeekNextCharacter(FILE *file);
function B8 IsFloatingPointChar(char character);
function haversine_token GetNextToken(FILE *file);

#endif // HAVERSINE_LEXER_H
