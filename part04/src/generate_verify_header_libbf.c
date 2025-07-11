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

#define ARRAY_SIZE 1000000

#define SIN_INPUT_NAME "Reference_SinInput"
#define SIN_OUTPUT_NAME "Reference_SinOutput"
#define COS_INPUT_NAME "Reference_CosInput"
#define COS_OUTPUT_NAME "Reference_CosOutput"
#define ARCSIN_INPUT_NAME "Reference_ArcSinInput"
#define ARCSIN_OUTPUT_NAME "Reference_ArcSinOutput"
#define SQRT_INPUT_NAME "Reference_SqrtInput"
#define SQRT_OUTPUT_NAME "Reference_SqrtOutput"


void *bf_realloc_func(void *opaque, void *ptr, size_t size)
{
    return realloc(ptr, size);
}


void process_sine_values(FILE *fp, bf_context_t *ctx, bf_t *inputs, bf_t *outputs)
{
    // Create and initialize pi value
    bf_t pi;
    bf_init(ctx, &pi);
    bf_const_pi(&pi, 57, BF_RNDN);

    // Write sine input array header
    fprintf(fp, "static F64 %s[] = {", SIN_INPUT_NAME);

    // Initialize and generate sine input numbers
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        // Generate random value between -pi and pi
        double rand_val = ((double)rand() / RAND_MAX) * 2.0 - 1.0; // [-1, 1]
        bf_set_float64(&inputs[i], rand_val);
        bf_mul(&inputs[i], &inputs[i], &pi, 57, BF_RNDN);  // multiply by pi to get [-pi, pi]

        // Convert to string
        size_t len;
        char *str = bf_ftoa(&len, &inputs[i], 10, 17, BF_RNDN);
        if (str)
        {
            fprintf(fp, "%s%s", str, (i < ARRAY_SIZE-1) ? ", " : "");
        }
    }

    // Close the sine input array
    fprintf(fp, "};\n\n");

    // Write sine output array header
    fprintf(fp, "static F64 %s[] = {", SIN_OUTPUT_NAME);

    // Generate and write sine values
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
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

    // Close the sine output array
    fprintf(fp, "};\n\n");

    // Cleanup pi
    bf_delete(&pi);
}


void process_cosine_values(FILE *fp, bf_context_t *ctx, bf_t *inputs, bf_t *outputs)
{
    // Create and initialize pi value
    bf_t pi;
    bf_init(ctx, &pi);
    bf_const_pi(&pi, 57, BF_RNDN);

    // Create pi/2
    bf_t pi_half, two;
    bf_init(ctx, &pi_half);
    bf_init(ctx, &two);
    bf_set_si(&two, 2);  // Set value to 2
    bf_div(&pi_half, &pi, &two, 57, BF_RNDN);  // Divide pi by 2

    // Write cosine input array header
    fprintf(fp, "static F64 %s[] = {", COS_INPUT_NAME);

    // Generate cosine input numbers
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        // Generate random value between -1 and 1
        double rand_val = ((double)rand() / RAND_MAX) * 2.0 - 1.0;
        bf_set_float64(&inputs[i], rand_val);

        // Multiply by pi/2 to get range [-pi/2, pi/2]
        bf_mul(&inputs[i], &inputs[i], &pi_half, 57, BF_RNDN);

        // Convert to string
        size_t len;
        char *str = bf_ftoa(&len, &inputs[i], 10, 17, BF_RNDN);
        if (str)
        {
            fprintf(fp, "%s%s", str, (i < ARRAY_SIZE-1) ? ", " : "");
        }
    }

    // Close the cosine input array
    fprintf(fp, "};\n\n");

    // Write cosine output array header
    fprintf(fp, "static F64 %s[] = {", COS_OUTPUT_NAME);

    // Generate and write cosine values
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        // Compute cosine using bf_cos with specific precision (17 digits ≈ 57 bits)
        bf_cos(&outputs[i], &inputs[i], 57, BF_RNDN);

        // Convert cosine result to string
        size_t len;
        char *str = bf_ftoa(&len, &outputs[i], 10, 17, BF_RNDN);
        if (str)
        {
            fprintf(fp, "%s%s", str, (i < ARRAY_SIZE-1) ? ", " : "");
        }
    }

    // Close the cosine output array
    fprintf(fp, "};\n\n");

    // Cleanup pi values
    bf_delete(&pi);
    bf_delete(&pi_half);
    bf_delete(&two);
}


void process_arcsin_values(FILE *fp, bf_context_t *ctx, bf_t *inputs, bf_t *outputs)
{
    // Write arc sine input array header
    fprintf(fp, "static F64 %s[] = {", ARCSIN_INPUT_NAME);

    // Generate arc sine input numbers
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        // Generate random value between 0 and 1
        double rand_val = (double)rand() / RAND_MAX; // [0, 1]
        bf_set_float64(&inputs[i], rand_val);

        // Convert to string
        size_t len;
        char *str = bf_ftoa(&len, &inputs[i], 10, 17, BF_RNDN);
        if (str)
        {
            fprintf(fp, "%s%s", str, (i < ARRAY_SIZE-1) ? ", " : "");
        }
    }

    // Close the arc sine input array
    fprintf(fp, "};\n\n");

    // Write arc sine output array header
    fprintf(fp, "static F64 %s[] = {", ARCSIN_OUTPUT_NAME);

    // Generate and write arc sine values
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        // Compute arc sine using bf_asin with specific precision (17 digits ≈ 57 bits)
        bf_asin(&outputs[i], &inputs[i], 57, BF_RNDN);

        // Convert arc sine result to string
        size_t len;
        char *str = bf_ftoa(&len, &outputs[i], 10, 17, BF_RNDN);
        if (str)
        {
            fprintf(fp, "%s%s", str, (i < ARRAY_SIZE-1) ? ", " : "");
        }
    }

    // Close the arc sine output array
    fprintf(fp, "};\n\n");
}


void process_sqrt_values(FILE *fp, bf_context_t *ctx, bf_t *inputs, bf_t *outputs)
{
    // Write square root input array header
    fprintf(fp, "static F64 %s[] = {", SQRT_INPUT_NAME);

    // Generate square root input numbers
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        // Generate random value between 0 and 1
        double rand_val = (double)rand() / RAND_MAX; // [0, 1]
        bf_set_float64(&inputs[i], rand_val);

        // Convert to string
        size_t len;
        char *str = bf_ftoa(&len, &inputs[i], 10, 17, BF_RNDN);
        if (str)
        {
            fprintf(fp, "%s%s", str, (i < ARRAY_SIZE-1) ? ", " : "");
        }
    }

    // Close the square root input array
    fprintf(fp, "};\n\n");

    // Write square root output array header
    fprintf(fp, "static F64 %s[] = {", SQRT_OUTPUT_NAME);

    // Generate and write square root values
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        // Compute square root using bf_sqrt with specific precision (17 digits ≈ 57 bits)
        bf_sqrt(&outputs[i], &inputs[i], 57, BF_RNDN);

        // Convert square root result to string
        size_t len;
        char *str = bf_ftoa(&len, &outputs[i], 10, 17, BF_RNDN);
        if (str)
        {
            fprintf(fp, "%s%s", str, (i < ARRAY_SIZE-1) ? ", " : "");
        }
    }

    // Close the square root output array
    fprintf(fp, "};\n");
}


int main(int argc, char *argv[])
{
    int result = 0;

    // Check if output path is provided
    if (argc != 2)
    {
        printf("Usage: %s <output_path>\n", argv[0]);
        return 1;
    }

    // Use the provided output path
    FILE *fp = fopen(argv[1], "w");
    if (!fp)
    {
        printf("Failed to open output file: %s\n", argv[1]);
        return 1;
    }

    bf_context_t ctx;
    bf_context_init(&ctx, bf_realloc_func, NULL);

    // Seed random number generator
    srand(time(NULL));

    // Allocate arrays dynamically
    bf_t *inputs = malloc(ARRAY_SIZE * sizeof(bf_t));
    bf_t *outputs = malloc(ARRAY_SIZE * sizeof(bf_t));

    if (inputs && outputs)
    {
        // Initialize arrays
        for (int i = 0; i < ARRAY_SIZE; i++)
        {
            bf_init(&ctx, &inputs[i]);
            bf_init(&ctx, &outputs[i]);
        }

        // Write the beginning of the include guard
        fprintf(fp, "#ifndef REFERENCE_VALUES_H\n#define REFERENCE_VALUES_H\n\n");

        // Write size define
        fprintf(fp, "#define REFERENCE_ARRAY_SIZE %d\n\n", ARRAY_SIZE);

        // Process sine values
        process_sine_values(fp, &ctx, inputs, outputs);

        // Process cosine values
        process_cosine_values(fp, &ctx, inputs, outputs);

        // Process arc sine values
        process_arcsin_values(fp, &ctx, inputs, outputs);

        // Process square root values
        process_sqrt_values(fp, &ctx, inputs, outputs);

        // Write the end of the include guard
        fprintf(fp, "\n#endif // REFERENCE_VALUES_H\n");

        // Cleanup
        for (int i = 0; i < ARRAY_SIZE; i++)
        {
            bf_delete(&inputs[i]);
            bf_delete(&outputs[i]);
        }
    }
    else
    {
        printf("Memory allocation failed\n");
        result = 1;
    }

    if (inputs) free(inputs);
    if (outputs) free(outputs);

    bf_context_end(&ctx);
    fclose(fp);

    return result;
}
