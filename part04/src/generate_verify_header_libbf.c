#include <stdio.h>
#include <time.h>
#include <stdlib.h>

#define malloc_is_forbidden malloc
#define free_is_forbidden free
#define realloc_is_forbidden realloc

#include "libbf/libbf.h"
#include "libbf/cutils.h"
#include "libbf/libbf.c"
#include "libbf/cutils.c"


#define ARRAY_SIZE 100

void *bf_realloc_func(void *opaque, void *ptr, size_t size)
{
    return realloc(ptr, size);
}

int main()
{
    FILE *fp = fopen("reference_values_libbf.h", "w");
    if (!fp)
    {
        printf("Failed to open output file\n");
        return 1;
    }

    bf_context_t ctx;
    bf_context_init(&ctx, bf_realloc_func, NULL);

    // Seed random number generator
    srand(time(NULL));

    // Allocate arrays dynamically
    bf_t *inputs = malloc(ARRAY_SIZE * sizeof(bf_t));
    bf_t *outputs = malloc(ARRAY_SIZE * sizeof(bf_t));

    if (!inputs || !outputs)
    {
        printf("Memory allocation failed\n");
        if (inputs) free(inputs);
        if (outputs) free(outputs);
        fclose(fp);
        return 1;
    }

    // Write size define
    fprintf(fp, "#define REFERENCE_ARRAY_SIZE %d\n\n", ARRAY_SIZE);

    // Write input array header
    fprintf(fp, "global_variable F64 Reference_SinInput[] = {");

    // Initialize and generate input numbers
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        bf_init(&ctx, &inputs[i]);

        // Generate random value between 0 and 1
        double rand_val = (double)rand() / RAND_MAX;
        bf_set_float64(&inputs[i], rand_val);

        // Convert to string
        size_t len;
        char *str = bf_ftoa(&len, &inputs[i], 10, 17, BF_RNDN);
        if (str)
        {
            fprintf(fp, "%s%s", str, (i < ARRAY_SIZE-1) ? ", " : "");
        }
    }

    // Close the input array
    fprintf(fp, "};\n\n");

    // Write output array header
    fprintf(fp, "global_variable F64 Reference_SinOutput[] = {");

    // Generate and write sine values
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        bf_init(&ctx, &outputs[i]);

        // Compute sine using bf_sin with specific precision (17 digits ≈ 57 bits)
        bf_sin(&outputs[i], &inputs[i], 57, BF_RNDN);

        // Convert sine result to string
        size_t len;
        char *str = bf_ftoa(&len, &outputs[i], 10, 17, BF_RNDN);
        if (str)
        {
            fprintf(fp, "%s%s", str, (i < ARRAY_SIZE-1) ? ", " : "");
        }
    }

    // Close the output array
    fprintf(fp, "};\n");

    // Cleanup
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        bf_delete(&inputs[i]);
        bf_delete(&outputs[i]);
    }
    free(inputs);
    free(outputs);
    bf_context_end(&ctx);
    fclose(fp);

    return 0;
}
