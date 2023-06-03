/*  TODO (Aaron):
    - Parse JSON
*/

#pragma warning(disable:4996)

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base.h"

#define EARTH_RADIUS 6372.8
#define DATA_FILENAME "haversine-pairs.json"
#define ANSWER_FILENAME "haversine-answer.f64"

// TODO (Aaron): Consider how to best pick this size
#define MAX_TOKEN_LENGTH 32


// TODO (Aaron): Specify an int size for enum?
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
} token;


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


function F64 Square(F64 A)
{
    F64 Result = (A*A);
    return Result;
}


function F64 RadiansFromDegrees(F64 Degrees)
{
    F64 Result = 0.01745329251994329577f * Degrees;
    return Result;
}


// NOTE(casey): EarthRadius is generally expected to be 6372.8
function F64 ReferenceHaversine(F64 X0, F64 Y0, F64 X1, F64 Y1, F64 EarthRadius)
{
    /* NOTE(casey): This is not meant to be a "good" way to calculate the Haversine distance.
       Instead, it attempts to follow, as closely as possible, the formula used in the real-world
       question on which these homework exercises are loosely based.
    */

    F64 lat1 = Y0;
    F64 lat2 = Y1;
    F64 lon1 = X0;
    F64 lon2 = X1;

    F64 dLat = RadiansFromDegrees(lat2 - lat1);
    F64 dLon = RadiansFromDegrees(lon2 - lon1);
    lat1 = RadiansFromDegrees(lat1);
    lat2 = RadiansFromDegrees(lat2);

    F64 a = Square(sin(dLat/2.0)) + cos(lat1)*cos(lat2)*Square(sin(dLon/2));
    F64 c = 2.0*asin(sqrt(a));

    F64 Result = EarthRadius * c;

    return Result;
}


static void *MemorySet(uint8_t *destPtr, int c, size_t count)
{
    Assert(count > 0);

    unsigned char *dest = (unsigned char *)destPtr;
    while(count--) *dest++ = (unsigned char)c;

    return destPtr;
}


int _NextCharacter(FILE *file, B8 peek)
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
int EatNextCharacter(FILE *file)
{
    return _NextCharacter(file, FALSE);
}


// Peek at the next character in a file stream (white-space characters excluded)
int PeekNextCharacter(FILE *file)
{
    return _NextCharacter(file, TRUE);
}


// Returns whether or not the character belong to the set of characters used by floating point values
bool IsFloatingPointChar(char character)
{
    return (isdigit(character) || character == '.' || character == '-');
}


// Extracts next JSON token from file stream
token GetNextToken(FILE *file)
{
    token token;
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
        Assert(false);
    }

    // Note (Aaron): If we reach this code path, we've exceeded the maximum token size and the token is invalid
    Assert(false);
    return token;
}


// int main(int argc, char const *argv[])
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

    S64 maxTokenLength = 0;

    for (;;)
    {
        token nextToken = GetNextToken(dataFile);

        printf("Token type: %s \t|  Token value: %s \t\t|  Token length: %i\n",
               GetTokenMenemonic(nextToken.Type),
               nextToken.String,
               nextToken.Length);

        maxTokenLength = nextToken.Length > maxTokenLength
            ? nextToken.Length
            : maxTokenLength;

        if (nextToken.Type == Token_EOF)
        {
            printf("\n");
            printf("[INFO] Reached end of file\n");
            break;
        }

        // TODO (Aaron): Parse tokens
    }

    if (ferror(dataFile))
    {
        fclose(dataFile);
        perror("[ERROR] ");
        return 1;
    }

    fclose(dataFile);

    printf("[INFO] Max token length: '%lli'\n", maxTokenLength);

    return 0;
}
