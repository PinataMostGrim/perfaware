/* TODO (Aaron):
    - Figure out the correct EOF return value and apply it in _NextCharacter()
    - Consider eliminating quotes from identifier token strings
*/

#include "haversine_lexer.h"


global const char *TokenMnemonics[] =
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


function const char *GetTokenMenemonic(token_type tokenType)
{
    static_assert(ArrayCount(TokenMnemonics) == Token_type_count,
        "'TokenMnemonics' count does not match 'Token_type_count'");

    return TokenMnemonics[tokenType];
}


function int _NextCharacter(FILE *file, B8 peek)
{
    for (;;)
    {
        int nextChar = fgetc(file);
        if (feof(file))
        {
            return 0;
        }

        // skip white space characters
        if (nextChar == 0x20        // space
            || nextChar == 0x09     // horizontal tab
            || nextChar == 0xa)     // newline
        {
            continue;
        }

        if (peek)
        {
            ungetc(nextChar, file);
        }

        return nextChar;
    }
}


// Eat characters from a file stream until we get a non-whitespace character or reach EOF
function int EatNextCharacter(FILE *file)
{
    return _NextCharacter(file, FALSE);
}


// Peek at the next character in a file stream (white-space characters excluded)
function int PeekNextCharacter(FILE *file)
{
    return _NextCharacter(file, TRUE);
}

// Returns whether or not the character belong to the set of characters used by floating point values
function B8 IsFloatingPointChar(char character)
{
    return (isdigit(character) || character == '.' || character == '-');
}


// Extracts next JSON token from file stream
function haversine_token GetNextToken(FILE *file)
{
    haversine_token token;
    token.Type = Token_invalid;
    MemorySet((U8 *)token.String, 0, sizeof(token.String));
    token.Length = 0;

    for (int i = 0; i < MAX_TOKEN_LENGTH; ++i)
    {
        char nextChar = (char)PeekNextCharacter(file);
        token.String[i] = nextChar;
        token.Length++;

        if (nextChar == 0)
        {
            token.Type = Token_EOF;
            MemorySet((U8 *)token.String, 0, sizeof(token.String));
            return token;
        }

        if (token.Type == Token_invalid)
        {
            EatNextCharacter(file);

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
            EatNextCharacter(file);
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
                EatNextCharacter(file);
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
