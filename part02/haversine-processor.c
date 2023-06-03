/*  TODO (Aaron):
    - Parse JSON
*/

#pragma warning(disable:4996)

#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "base.h"

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


// Eat characters from a file stream until we get a non-whitespace character or reach EOF
int EatNextCharacter(FILE *file)
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

        return nextChar;
    }
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

    // walk through entire file one char at a time
    // for (int i = 0; i < 100; ++i)
    for (;;)
    {
        int nextChar = EatNextCharacter(dataFile);
        if (nextChar == 0)
        {
            printf("\n");
            printf("[INFO] Reached end of file\n");
            break;
        }

        printf("%c", nextChar);
        // printf("%i ", nextChar);

        // TODO (Aaron): Do processing on the char

        // TODO (Aaron): parsing "pairs"
    }

    if (ferror(dataFile))
    {
        fclose(dataFile);
        perror("[ERROR] ");
        return 1;
    }

    fclose(dataFile);

    return 0;
}
