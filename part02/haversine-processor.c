/*  TODO (Aaron):
    - Use a token stack to parse through the "pairs" array
    - Load and consume the binary "answers" file for validation
    - Create a struct for stats (max token length, number of tokens, etc)
    - Extract file names into 'haversine.h'
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
