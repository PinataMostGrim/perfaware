/*  TODO (Aaron):
    - Use a token stack to parse through the "pairs" array
    - Optionally load and consume the binary "answers" file for validation
*/

#pragma warning(disable:4996)

#include "base.h"
#include "haversine.c"
#include "haversine_lexer.c"

#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct
{
    S64 TokenCount;
    S64 MaxTokenLength;
} processor_stats;


function void PrintStats(processor_stats stats)
{
    printf("[INFO] Stats:\n");
    printf("[INFO]  Tokens processed: '%lli'\n", stats.TokenCount);
    printf("[INFO]  Max token length: '%lli'\n", stats.MaxTokenLength);
    printf("\n");
}


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

    processor_stats stats =
    {
        .TokenCount = 0,
        .MaxTokenLength = 0,
    };

    for (;;)
    {
        token nextToken = GetNextToken(dataFile);
        stats.TokenCount++;

        printf("Token type: %s \t|  Token value: %s \t\t|  Token length: %i\n",
               GetTokenMenemonic(nextToken.Type),
               nextToken.String,
               nextToken.Length);

        stats.MaxTokenLength = nextToken.Length > stats.MaxTokenLength
            ? nextToken.Length
            : stats.MaxTokenLength;

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

    PrintStats(stats);

    return 0;
}
