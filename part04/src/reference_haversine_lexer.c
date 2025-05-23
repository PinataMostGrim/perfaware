#include <assert.h>
#include <ctype.h>
#include <stdio.h>

#include "base.h"
#include "base_memory.h"
#include "base_arena.h"
#include "base_string.h"
#include "reference_haversine_lexer.h"


global_variable const char *TokenMnemonics[] =
{
    "Token_invalid",
    "Token_identifier",
    "Token_value",
    "Token_assignment",
    "Token_delimiter",
    "Token_scope_open",
    "Token_scope_close",
    "Token_array_start",
    "Token_array_end",
    "Token_EOF",
};


global_function const char *GetTokenMenemonic(token_type tokenType)
{
    static_assert(ArrayCount(TokenMnemonics) == Token_type_count,
        "'TokenMnemonics' count does not match 'Token_type_count'");

    return TokenMnemonics[tokenType];
}


global_function int _NextCharacter(memory_arena *arena, B8 peek)
{
    for (;;)
    {
        if (arena->PositionPtr >= arena->BasePtr + arena->Used)
        {
            return 0;
        }
        int nextChar = *arena->PositionPtr++;

        // skip white space characters
        if (IsWhitespaceChar((char)nextChar))
        {
            continue;
        }

        // TODO (Aaron): If peek is true, this will reset the pointer back to the last whitespace character
        // Need to resolve this.
        if (peek)
        {
            arena->PositionPtr--;
        }

        return nextChar;
    }
}


// Eat characters from a file stream until we get a non-whitespace character or reach EOF
global_function int EatNextCharacter(memory_arena *arena)
{
    return _NextCharacter(arena, FALSE);
}


// Peek at the next character in a file stream (white-space characters excluded)
global_function int PeekNextCharacter(memory_arena *arena)
{
    return _NextCharacter(arena, TRUE);
}

// Returns whether or not the character belong to the set of characters used by floating point values
global_function B8 IsFloatingPointChar(char character)
{
    return (isdigit(character) || character == '.' || character == '-');
}


// Extracts next JSON token from file stream
global_function haversine_token GetNextToken(memory_arena *arena)
{
    haversine_token token;
    token.Type = Token_invalid;
    MemorySet(token.String, 0, sizeof(token.String));
    token.Length = 0;

    for (int i = 0; i < MAX_TOKEN_LENGTH; ++i)
    {
        char nextChar = (char)PeekNextCharacter(arena);
        token.String[i] = nextChar;
        token.Length++;

        if (nextChar == 0)
        {
            token.Type = Token_EOF;
            MemorySet(token.String, 0, sizeof(token.String));
            return token;
        }

        if (token.Type == Token_invalid)
        {
            EatNextCharacter(arena);

            if (nextChar == '{')
            {
                token.Type = Token_scope_open;
                return token;
            }

            if (nextChar == '}')
            {
                token.Type = Token_scope_close;
                return token;
            }

            if (nextChar == ':')
            {
                token.Type = Token_assignment;
                return token;
            }

            if (nextChar == '[')
            {
                token.Type = Token_array_start;
                return token;
            }

            if (nextChar == ']')
            {
                token.Type = Token_array_end;
                return token;
            }

            if (nextChar == ',')
            {
                token.Type = Token_delimiter;
                return token;
            }

            if (nextChar == '"')
            {
                token.Type = Token_identifier;
                continue;
            }

            if (IsFloatingPointChar(nextChar))
            {
                token.Type = Token_value;
                continue;
            }
        }

        if (token.Type == Token_identifier)
        {
            EatNextCharacter(arena);

            // Note (Aaron): If we were going to do a more serious job, we'd check for escape characters here
            if (nextChar != '"')
            {
                continue;
            }

            return token;
        }

        if (token.Type == Token_value)
        {
            if (IsFloatingPointChar(nextChar))
            {
                EatNextCharacter(arena);
                continue;
            }

            token.String[i] = 0;
            token.Length--;
            return token;
        }

        // Note (Aaron): If we reach this code path, we've failed to identify the token
        Assert(FALSE);
    }

    // Note (Aaron): If we reach this code path, we've exceeded the maximum token size and the token is invalid
    Assert(FALSE);
    return token;
}
