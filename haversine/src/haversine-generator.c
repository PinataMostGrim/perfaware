/*  TODO (Aaron):
    - Make seed an optional argument
    - Add "pairs_count" to JSON and answers file for easy validation
    - Move "seed" value to bottom of JSON output. I think I like it better at the bottom.
*/

#pragma warning(disable:4996)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "base_inc.h"
#include "haversine.h"

#include "base_types.c"
#include "haversine.c"

#define CLUSTER_COUNT 16
#define CLUSTER_PROXIMITY 20


global_function void PrintUsage()
{
    printf("usage: haversine-generator seed pair-count \n\n");
    printf("produces a JSON formatted file containing a variable number of coordinate pairs\nused for calculating Haversine distances.\n\n");

    printf("positional arguments:\n");
    printf("  seed\t\t\tvalue to use for random seed\n");
    printf("  pair-count\t\tthe number of coordinate pairs to generate\n");
    printf("\n");

    printf("options:\n");
    printf("  --help, -h\t\tshow this message\n");
}


global_function S64 GetRandomS64InRange(S64 minValue, S64 maxValue)
{
    F64 scale = (F64)rand() / (F64)RAND_MAX;
    return minValue + (S64)(scale * (maxValue - minValue));
}


global_function F64 GetRandomF64InRange(F64 minValue, F64 maxValue)
{
    F64 scale = (F64)rand() / (F64)RAND_MAX;
    return minValue + scale * (maxValue - minValue);
}


// perform modulo operation on values so they fall within the range (-180, 180)
global_function F64 CanonicalizeCoordinate(F64 value)
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
    const char *seedEndPtr = seedPtr + seedLength;
    unsigned int seed = (unsigned int)strtoll(seedPtr, (char **)&seedEndPtr, 10);

    const char *pairCountPtr = argv[2];
    size_t pairCountLength = strlen(pairCountPtr);
    const char *pairCountEndPtr = pairCountPtr + pairCountLength;
    int64_t pairCount = strtoll(pairCountPtr, (char **)&pairCountEndPtr, 10);

    if (pairCount <= 0)
    {
        printf("[ERROR] Argument 'pair-count' must be larger than 0!\n");
        exit(1);
    }

    // open data file
    char *dataFilename = DATA_FILENAME;
    FILE *dataFile;
    dataFile = fopen(dataFilename, "w");

    if (!dataFile)
    {
        printf("[ERROR] Unable to open '%s' for writing\n", dataFilename);
        return 1;
    }

    // open answer file
    char *answerFilename = ANSWER_FILENAME;
    FILE *answerFile;
    answerFile = fopen(answerFilename, "wb");

    if (!answerFile)
    {
        printf("[ERROR] Unable to open '%s' for writing\n", answerFilename);
        return 1;
    }

    // write placeholder header to answers file
    // (we won't know the expected sum until generation has finished)
    answers_file_header answersHeader = { .Seed = 0, .ExpectedSum = 0 };
    fwrite(&answersHeader, sizeof(answers_file_header), 1, answerFile);

    printf("[INFO] Generating Haversine distance coordinate pairs...\n");
    printf("[INFO] Seed:\t\t%u\n", seed);
    printf("[INFO] Pair count:\t%llu\n", pairCount);

    srand(seed);

    char *line[256];
    F64 expectedSum = 0;

    sprintf((char *)line, "{\n\t\"seed\":%u,\n", seed);
    fputs((char *)line, dataFile);

    // Generate cluster points
    V2F64 clusters[CLUSTER_COUNT];
    for (int i = 0; i < ArrayCount(clusters); ++i)
    {
        V2F64 cluster = v2f64(
            GetRandomF64InRange(-180, 180),
            GetRandomF64InRange(-180, 180));

        clusters[i] = cluster;
    }

    fputs("\t\"pairs\": [\n", dataFile);

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

        sprintf((char *)line, "\t\t{ \"x0\":%.16f, \"y0\":%.16f, \"x1\":%.16f, \"y1\":%.16f }", point0.x, point0.y, point1.x, point1.y);
        fputs((char *)line, dataFile);
        fputs((i == (pairCount - 1) ? "\n" : ",\n"),
              dataFile);

        F64 distance = ReferenceHaversine(point0.x, point0.y, point1.x, point1.y, EARTH_RADIUS);
        fwrite(&distance, sizeof(F64), 1, answerFile);

        // source: https://math.stackexchange.com/a/1153800
        expectedSum = ((expectedSum * (F64)i) + distance) / (F64)(i + 1);
    }

    fputs("\t],\n", dataFile);

    sprintf((char *)line, "\t\"expected_sum\":%.16f\n", expectedSum);
    fputs((char *)line, dataFile);

    fputs("}\n", dataFile);

    if (ferror(dataFile))
    {
        fclose(dataFile);
        Assert(FALSE);

        printf("[ERROR] Error writing file %s\n", dataFilename);
        exit(1);
    }
    fclose(dataFile);

    // set correct values in answer file header
    answersHeader.Seed = seed;
    answersHeader.ExpectedSum = expectedSum;
    fseek(answerFile, 0, SEEK_SET);
    fwrite(&answersHeader, sizeof(answersHeader), 1, answerFile);

    if (ferror(answerFile))
    {
        fclose(answerFile);
        Assert(FALSE);

        printf("[ERROR] Error writing file %s\n", answerFilename);
        exit(1);
    }
    fclose(answerFile);

    printf("[INFO] Expected sum:\t%f\n", expectedSum);

    printf("\n");
    printf("[INFO] Data file: \t%s\n", dataFilename);
    printf("[INFO] Answer file: \t%s\n", answerFilename);

    return 0;
}
