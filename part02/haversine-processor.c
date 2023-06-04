/*  TODO (Aaron):
    - Use a token stack to parse through the "pairs" array
    - Load and consume the binary "answers" file for validation
    - Create a struct for stats (max token length, number of tokens, etc)
    - Extract file names into 'haversine.h'
*/

#pragma warning(disable:4996)

#include "base.h"
#include "haversine_lexer.c"

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EARTH_RADIUS 6372.8
#define DATA_FILENAME "haversine-pairs.json"
#define ANSWER_FILENAME "haversine-answer.f64"


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

        // TODO (Aaron): Parse!
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
