#include <math.h>
#include <stdio.h>

#define malloc_is_forbidden malloc
#define free_is_forbidden free
#define realloc_is_forbidden realloc

#include "libbf/libbf.h"
#include "libbf/cutils.h"
#include "libbf/libbf.c"
#include "libbf/cutils.c"

#include "base_inc.h"
#include "base_types.c"

// Note (Aaron): Must be defined before including the reference values header
typedef struct reference_answer reference_answer;
struct reference_answer
{
    F64 Input;
    F64 Output;
};

#include "reference_values_wolfram.h"


void *bf_realloc_func(void *opaque, void *ptr, size_t size)
{
    return realloc(ptr, size);
}


F64 calculate_bf_sin(bf_context_t *ctx, F64 input)
{
    F64 result;

    bf_t bf_input;
    bf_t bf_output;

    bf_init(ctx, &bf_input);
    bf_init(ctx, &bf_output);

    bf_set_float64(&bf_input, input);

    // BF_RNDN means "round to nearest" mode
    bf_sin(&bf_output, &bf_input, 57, BF_RNDN);

    // Convert back to F64 (double)
    bf_get_float64(&bf_output, &result, BF_RNDN);

    // Clean up
    bf_delete(&bf_input);
    bf_delete(&bf_output);

    return result;
}


// Function to calculate error between two values
F64 calculate_error(F64 reference, F64 calculated)
{
    return fabs(reference - calculated);
}


int main(int argc, char const *argv[])
{
    // Setup LibBF
    bf_context_t ctx;
    bf_context_init(&ctx, bf_realloc_func, NULL);

    // Process each input value
    printf("Comparing sin() calculations:\n");
    printf("-----------------------------\n");

    size_t answerCount = ArrayCount(Reference_Sin);
    for (size_t i = 0; i < answerCount; i++)
    {
        reference_answer answer = Reference_Sin[i];

        // Standard lib calculation
        F64 std_lib_result = sin(answer.Input);
        F64 std_lib_error = calculate_error(answer.Output, std_lib_result);

        // Compute sine using bf_sin (LibBF) with specific precision (17 digits ≈ 57 bits)
        F64 bf_result = calculate_bf_sin(&ctx, answer.Input);
        F64 bf_error = calculate_error(answer.Output, bf_result);

        printf("Input:      %.17g\n", answer.Input);
        printf("Reference:  %.17g\n", answer.Output);
        printf("Std Lib:    %.17g (Error: %.17g)\n", std_lib_result, std_lib_error);
        printf("LibBF:      %.17g (Error: %.17g)\n\n", bf_result, bf_error);
    }

    return 0;
}
