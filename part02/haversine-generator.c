#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma warning(disable:4996)

typedef double f64;

#define EARTH_RADIUS 6372.8


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


static f64 GetRandomFloatInRange(f64 minValue, f64 maxValue)
{
    f64 scale = (f64)rand() / (f64)RAND_MAX;
    return minValue + scale * (maxValue - minValue);
}


static f64 Square(f64 A)
{
    f64 Result = (A*A);
    return Result;
}


static f64 RadiansFromDegrees(f64 Degrees)
{
    f64 Result = 0.01745329251994329577f * Degrees;
    return Result;
}


// NOTE(casey): EarthRadius is generally expected to be 6372.8
static f64 ReferenceHaversine(f64 X0, f64 Y0, f64 X1, f64 Y1, f64 EarthRadius)
{
    /* NOTE(casey): This is not meant to be a "good" way to calculate the Haversine distance.
       Instead, it attempts to follow, as closely as possible, the formula used in the real-world
       question on which these homework exercises are loosely based.
    */

    f64 lat1 = Y0;
    f64 lat2 = Y1;
    f64 lon1 = X0;
    f64 lon2 = X1;

    f64 dLat = RadiansFromDegrees(lat2 - lat1);
    f64 dLon = RadiansFromDegrees(lon2 - lon1);
    lat1 = RadiansFromDegrees(lat1);
    lat2 = RadiansFromDegrees(lat2);

    f64 a = Square(sin(dLat/2.0)) + cos(lat1)*cos(lat2)*Square(sin(dLon/2));
    f64 c = 2.0*asin(sqrt(a));

    f64 Result = EarthRadius * c;

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

    printf("Generating Haversine distance coordinate pairs\n");
    printf("Seed:\t\t%u\n", seed);
    printf("Pair count:\t%llu\n\n", pairCount);

    char *filename = "haversine_pairs.json";
    FILE *file;
    file = fopen(filename, "w");

    if (!file)
    {
        printf("[ERROR] Unable to open '%s' for writing\n", filename);
        return 1;
    }

    f64 *distances = calloc(pairCount, sizeof(f64));
    if (!distances)
    {
        printf("[ERROR] Unable to allocate memory to hold Haversine distances (%llu bytes)\n", pairCount * sizeof(f64));
        return 1;
    }

    fputs("{\n\t\"pairs\": [\n", file);

    srand(seed);
    char *line[256];
    for (int i = 0; i < pairCount; ++i)
    {
        f64 x0 = GetRandomFloatInRange(-180, 180);
        f64 y0 = GetRandomFloatInRange(-180, 180);
        f64 x1 = GetRandomFloatInRange(-180, 180);
        f64 y1 = GetRandomFloatInRange(-180, 180);

        distances[i] = ReferenceHaversine(x0, y0, x1, y1, EARTH_RADIUS);

        sprintf((char *)line, "\t\t{ \"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f }", x0, y0, x1, y1);
        fputs((const char *)line, file);
        fputs((i == (pairCount - 1) ? "\n" : ",\n"),
              file);
    }

    fputs("\t]\n}\n", file);
    fclose(file);

    printf("Haversine coordinate pairs saved to '%s'\n", filename);

    // TODO (Aaron): Use clustering for pairs

    // TODO (Aaron): Keep track of the mean value of the distance between haversine pairs

    // TODO (Aaron): Output a binary file containing the haversine solution for all pairs output in the json file

    // TODO (Aaron): Try to figure out how to time execution time in C so I can time and compare these operations

    return 0;
}
