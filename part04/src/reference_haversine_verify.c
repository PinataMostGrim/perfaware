#include <float.h>
#include <math.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>

#include "base_inc.h"
#include "base_types.c"

typedef F64 math_func(F64);

typedef struct reference_answer reference_answer;
struct reference_answer
{
    F64 Input;
    F64 Output;
};



// Note (Aaron): Reference values from different sources. Include one.
#include "reference_values_wolfram.h"
// #include "reference_values_libbf.h"


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


global_function void CheckHardcodedAnswer(char *label, math_func referenceFunc, reference_answer *answers, size_t answerCount)
{
    printf("%s:\n", label);
    for (int i = 0; i < answerCount; ++i)
    {
        reference_answer answer = answers[i];
        printf("  f(%+.16f) = %+.16f [reference]\n", answer.Input, answer.Output);
        F64 output = referenceFunc(answer.Input);
        printf("            = %+.16f (%+.16f) [%s]\n", output, answer.Output - output, label);
    }
    printf("\n");
}




int main(int argc, char const *argv[])
{
    CheckHardcodedAnswer("sin", sin, Reference_Sin, ArrayCount(Reference_Sin));
    CheckHardcodedAnswer("cos", cos, Reference_Cos, ArrayCount(Reference_Cos));
    CheckHardcodedAnswer("asin", asin, Reference_ArcSin, ArrayCount(Reference_ArcSin));
    CheckHardcodedAnswer("sqrt", sqrt, Reference_Sqrt, ArrayCount(Reference_Sqrt));

    return 0;
}
