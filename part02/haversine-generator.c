/* // TODO (Aaron):
    - Figure out how to time execution time in C so I can time and compare these operations
*/

#pragma warning(disable:4996)

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base.h"

#define EARTH_RADIUS 6372.8
#define CLUSTER_PROXIMITY 20
#define DATA_FILENAME "haversine-pairs.json"
#define ANSWER_FILENAME "haversine-answer.f64"


static void PrintUsage()
{
    printf("usage: haversine-generator seed pair-count \n\n");
    printf("disassembles 8086/88 assembly and optionally simulates it. note: supports \na limited number of instructions.\n\n");

    printf("positional arguments:\n");
    printf("  seed\t\t\tvalue to use for random seed\n");
    printf("  pair-count\t\tthe number of coordinate pairs to generate\n");
    printf("\n");

    printf("options:\n");
    printf("  --help, -h\t\tshow this message\n");
}


function S64 GetRandomS64InRange(S64 minValue, S64 maxValue)
{
    F64 scale = (F64)rand() / (F64)RAND_MAX;
    return minValue + (S64)(scale * (maxValue - minValue));
}


function F64 GetRandomF64InRange(F64 minValue, F64 maxValue)
{
    F64 scale = (F64)rand() / (F64)RAND_MAX;
    return minValue + scale * (maxValue - minValue);
}


// Perform modulo operation on values so they fall within the range (-180, 180)
function F64 CanonicalizeCoordinate(F64 value)
{
    value += 180;
    if (value > 360)
    {
        while (value > 360)
        {
            value -= 360;
        }
    }
    else
    {
        while (value < 0)
        {
            value += 360;
        }
    }
    value -= 180;

    return value;
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


int main(int argc, char const *argv[])
{
    if (argc != 3)
    {
        PrintUsage();
        exit(1);
    }

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp("--help", argv[i]) == 0)
        {
            PrintUsage();
            exit(0);
        }
    }

    const char *seedPtr = argv[1];
    size_t seedLength = strlen(seedPtr);
    const char **seedEndPtr = (&seedPtr + seedLength);
    unsigned seed = (unsigned int)strtoll(seedPtr, (char **)seedEndPtr, 10);

    const char *pairCountPtr = argv[2];
    size_t pairCountLength = strlen(pairCountPtr);
    const char **pairCountEndPtr = (&pairCountPtr + pairCountLength);
    int64_t pairCount = strtoll(pairCountPtr, (char **)pairCountEndPtr, 10);

    if (pairCount <= 0)
    {
        printf("[ERROR] Argument 'pair-count' must be larger than 0!\n");
        exit(1);
    }

    // Open data file
    char *dataFilename = DATA_FILENAME;
    FILE *dataFile;
    dataFile = fopen(dataFilename, "w");

    if (!dataFile)
    {
        printf("[ERROR] Unable to open '%s' for writing\n", dataFilename);
        return 1;
    }

    // Open answer file
    char *answerFilename = ANSWER_FILENAME;
    FILE *answerFile;
    answerFile = fopen(answerFilename, "wb");

    if (!answerFile)
    {
        printf("[ERROR] Unable to open '%s' for writing\n", answerFilename);
        return 1;
    }

    printf("[INFO] Seed:\t\t%u\n", seed);
    printf("[INFO] Pair count:\t%llu\n", pairCount);
    printf("[INFO] Generating Haversine distance coordinate pairs...\n");

    fputs("{\n\t\"pairs\": [\n", dataFile);

    srand(seed);

    // Generate cluster points
    V2F64 clusters[64];
    for (int i = 0; i < ArrayCount(clusters); ++i)
    {
        V2F64 cluster = v2f64(
            GetRandomF64InRange(-180, 180),
            GetRandomF64InRange(-180, 180));

        clusters[i] = cluster;
    }

    char *line[256];
    F64 expectedSum = 0;

    // Generate Haversine distance pairs
    for (int i = 0; i < pairCount; ++i)
    {
        S64 clusterIndex0 = GetRandomS64InRange(0, ArrayCount(clusters));
        V2F64 clusterPoint0 = clusters[clusterIndex0];
        V2F64 point0 = v2f64(
            GetRandomF64InRange(clusterPoint0.x - CLUSTER_PROXIMITY, clusterPoint0.x + CLUSTER_PROXIMITY),
            GetRandomF64InRange(clusterPoint0.y - CLUSTER_PROXIMITY, clusterPoint0.y + CLUSTER_PROXIMITY));

        S64 clusterIndex1 = GetRandomS64InRange(0, ArrayCount(clusters));
        V2F64 clusterPoint1 = clusters[clusterIndex1];
        V2F64 point1 = v2f64(
            GetRandomF64InRange(clusterPoint1.x - CLUSTER_PROXIMITY, clusterPoint1.x + CLUSTER_PROXIMITY),
            GetRandomF64InRange(clusterPoint1.y - CLUSTER_PROXIMITY, clusterPoint1.y + CLUSTER_PROXIMITY));

        point0.x = CanonicalizeCoordinate(point0.x);
        point0.y = CanonicalizeCoordinate(point0.y);
        point1.x = CanonicalizeCoordinate(point1.x);
        point1.y = CanonicalizeCoordinate(point1.y);

        sprintf((char *)line, "\t\t{ \"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f }", point0.x, point0.y, point1.x, point1.y);
        fputs((char *)line, dataFile);
        fputs((i == (pairCount - 1) ? "\n" : ",\n"),
              dataFile);

        F64 distance = ReferenceHaversine(point0.x, point0.y, point1.x, point1.y, EARTH_RADIUS);
        fwrite(&distance, sizeof(F64), 1, answerFile);

        // source: https://math.stackexchange.com/a/1153800
        expectedSum = ((expectedSum * i) + distance) / (i + 1);
    }

    fputs("\t],\n", dataFile);

    sprintf((char *)line, "\t\"expected_sum\":%f,\n", expectedSum);
    fputs((char *)line, dataFile);

    sprintf((char *)line, "\t\"seed\":%u\n", seed);
    fputs((char *)line, dataFile);
    fputs("}\n", dataFile);

    fclose(dataFile);
    fclose(answerFile);

    printf("[INFO] Expected sum:\t%f\n", expectedSum);

    return 0;
}
