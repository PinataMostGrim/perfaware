#include <float.h>
#include <math.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>

#include "base_inc.h"
#include "base_types.c"


// Note (Aaron): Reference values from different sources. Include one.
// #include "reference_values_wolfram.h"
#include "reference_values_libbf.h"

#define STATIC_ASSERT(condition, message) \
    typedef char static_assertion_##message[(condition) ? 1 : -1]

STATIC_ASSERT(sizeof(Reference_SinInput) == sizeof(Reference_SinOutput), array_lengths_must_be_equal);
STATIC_ASSERT(sizeof(Reference_CosInput) == sizeof(Reference_CosOutput), array_lengths_must_be_equal);
STATIC_ASSERT(sizeof(Reference_ArcSinInput) == sizeof(Reference_ArcSinOutput), array_lengths_must_be_equal);
STATIC_ASSERT(sizeof(Reference_SqrtInput) == sizeof(Reference_SqrtOutput), array_lengths_must_be_equal);


#define ARRAY_PARAM(type, array) ((type) \
{ \
    .Size = (sizeof(array) / sizeof((array)[0])), \
    .Data = (array) \
})


typedef struct f64_array f64_array;
struct f64_array
{
    size_t Size;
    F64 *Data;
};

typedef struct range range;
struct range
{
    F64 Min;
    F64 Max;
};

typedef struct validation_set validation_set;
struct validation_set
{
    F64 Input;
    F64 GroundTruth;

    F64 ReferenceOutput;
    F64 CustomOutput;

    F64 ReferenceError;
    F64 CustomError;
};

typedef struct error error;
struct error
{
    U64 SampleCount;

    range Reference;
    range Custom;

    F64 ReferenceAbs;
    F64 CustomAbs;
};


global_function void PrintValidationSet(validation_set set)
{
    fprintf(stdout, "input: %f\n", set.Input);
    fprintf(stdout, "ground truth: %f\n", set.GroundTruth);

    fprintf(stdout, "reference output: %f\n", set.ReferenceOutput);
    fprintf(stdout, "custom output: %f\n", set.CustomOutput);

    fprintf(stdout, "reference error: %.15f\n", set.ReferenceError);
    fprintf(stdout, "custom error: %.15f\n", set.CustomError);
}


global_function void PrintError(error error, char *label)
{
    fprintf(stdout, "--- %s ---\n", label);

    fprintf(stdout, "reference error: %.15f\n", error.ReferenceAbs);
    fprintf(stdout, "custom error: %.15f\n", error.CustomAbs);
    fprintf(stdout, "sample count: %" PRIu64"\n", error.SampleCount);

    // fprintf(stdout, "reference min: %.15f\n", error.Reference.Min);
    // fprintf(stdout, "reference max: %.15f\n", error.Reference.Max);
    // fprintf(stdout, "custom min: %.15f\n", error.Custom.Min);
    // fprintf(stdout, "custom max: %.15f\n", error.Custom.Max);
}


global_function F64 CustomSin(F64 input)
{
    return 0;
}


global_function F64 CustomCos(F64 input)
{
    return 0;
}


global_function F64 CustomArcSin(F64 input)
{
    return 0;
}


global_function F64 CustomSqrt(F64 input)
{
    return 0;
}


global_function void RangeInclude(range *range, F64 value)
{
    if (value < range->Min) { range->Min = value; }
    if (value > range->Max) { range->Max = value; }
}


global_function void ErrorCalculate(error *error)
{
    F64 refMinAbs = error->Reference.Min < 0 ? error->Reference.Min * -1 : error->Reference.Min;
    F64 refMaxAbs = error->Reference.Max < 0 ? error->Reference.Max * -1 : error->Reference.Max;

    F64 customMinAbs = error->Custom.Min < 0 ? error->Custom.Min * -1 : error->Custom.Min;
    F64 customMaxAbs = error->Custom.Max < 0 ? error->Custom.Max * -1 : error->Custom.Max;

    error->ReferenceAbs = refMaxAbs > refMinAbs ? refMaxAbs : refMinAbs;
    error->CustomAbs = customMaxAbs > customMinAbs ? customMaxAbs : customMinAbs;
}


global_function error MeasureFunctionError(f64_array inputValues, f64_array groundTruthValues, F64 (*referenceFunc)(F64), F64 (*customFunc)(F64))
{
    range invertedInfinite = {DBL_MAX, -DBL_MAX};

    error result = {0};
    result.Reference = invertedInfinite;
    result.Custom = invertedInfinite;

    if (inputValues.Size == groundTruthValues.Size)
    {
        if (referenceFunc && customFunc)
        {
            validation_set set = {0};
            for (int i = 0; i < inputValues.Size; ++i)
            {
                set.Input = inputValues.Data[i];
                set.GroundTruth = groundTruthValues.Data[i];

                set.ReferenceOutput = referenceFunc(set.Input);
                set.CustomOutput = customFunc(set.Input);

                set.ReferenceError = set.GroundTruth - set.ReferenceOutput;
                set.CustomError = set.GroundTruth - set.CustomOutput;

                // PrintValidationSet(set);
                // fprintf(stdout, "\n");

                result.SampleCount++;
                RangeInclude(&result.Reference, set.ReferenceError);
                RangeInclude(&result.Custom, set.CustomError);
            }
        }
        else
        {
            fprintf(stderr, "[ERROR] Invalid reference or custom function\n");
        }
    }
    else
    {
        fprintf(stderr, "[ERROR] Input value array and ground truth value array sizes do not match\n");
    }

    ErrorCalculate(&result);

    return result;
}


int main(int argc, char const *argv[])
{
    fprintf(stdout, "Function error from ground truth\n");

    f64_array sinInput = ARRAY_PARAM(f64_array, Reference_SinInput);
    f64_array sinOutput = ARRAY_PARAM(f64_array, Reference_SinOutput);
    error errorsSin = MeasureFunctionError(sinInput, sinOutput, sin, CustomSin);
    PrintError(errorsSin, "Sin");
    fprintf(stdout, "\n");

    f64_array cosInput = ARRAY_PARAM(f64_array, Reference_CosInput);
    f64_array cosOutput = ARRAY_PARAM(f64_array, Reference_CosOutput);
    error errorsCos = MeasureFunctionError(cosInput, cosOutput, cos, CustomCos);
    PrintError(errorsCos, "Cos");
    fprintf(stdout, "\n");

    f64_array arcSinInput = ARRAY_PARAM(f64_array, Reference_ArcSinInput);
    f64_array arcSinOutput = ARRAY_PARAM(f64_array, Reference_ArcSinOutput);
    error errorsArcSin = MeasureFunctionError(arcSinInput, arcSinOutput, asin, CustomArcSin);
    PrintError(errorsArcSin, "ArcSin");
    fprintf(stdout, "\n");

    f64_array sqrtInput = ARRAY_PARAM(f64_array, Reference_SqrtInput);
    f64_array sqrtOutput = ARRAY_PARAM(f64_array, Reference_SqrtOutput);
    error errorsSqrt = MeasureFunctionError(sqrtInput, sqrtOutput, sqrt, CustomSqrt);
    PrintError(errorsSqrt, "Sqrt");
    fprintf(stdout, "\n");

    return 0;
}
