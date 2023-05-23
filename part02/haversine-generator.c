#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma warning(disable:4996)


typedef struct
{
    double X0;
    double Y0;
    double X1;
    double Y1;
    double Distance;
} haversine_pair;


void PrintUsage()
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


double GetRandomFloatInRange(double minValue, double maxValue)
{
    double scale = (double)rand() / (double)RAND_MAX;
    return minValue + scale * (maxValue - minValue);
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

    printf("[INFO] Generating %llu Haversine coordinate pairs...\n", pairCount);

    char *filename = "haversine_pairs.json";
    FILE *file;
    file = fopen(filename, "w");

    if (!file)
    {
        printf("[ERROR] Unable to open '%s' for writing\n", filename);
        return 1;
    }

    fputs("{\n\t\"pairs\": [\n", file);

    srand(seed);
    char *line[256];
    haversine_pair pair;
    for (int i = 0; i < pairCount; ++i)
    {
        pair.X0 = GetRandomFloatInRange(-180, 180);
        pair.Y0 = GetRandomFloatInRange(-180, 180);
        pair.X1 = GetRandomFloatInRange(-180, 180);
        pair.Y1 = GetRandomFloatInRange(-180, 180);

        sprintf((char *)line, "\t\t{ \"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f }", pair.X0, pair.Y0, pair.X1, pair.Y1);
        fputs((const char *)line, file);
        fputs((i == (pairCount - 1) ? "\n" : ",\n"),
              file);
    }

    fputs("\t]\n}\n", file);
    fclose(file);

    printf("[INFO] Haversine coordinate pairs saved to '%s'\n", filename);

    // TODO (Aaron): Implement the haversine formula

    // TODO (Aaron): Use clustering for pairs

    // TODO (Aaron): Keep track of the mean value of the distance between haversine pairs

    // TODO (Aaron): Output a binary file containing the haversine solution for all pairs output in the json file

    // TODO (Aaron): Try to figure out how to time execution time in C so I can time and compare these operations

    return 0;
}
