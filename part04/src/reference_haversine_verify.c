#include <float.h>
#include <math.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>

#include "base_inc.h"
#include "base_types.c"

#if _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

// source: https://www.wolframalpha.com/input?i=N%5BPi%2C+17%5D
// N[Pi, 17]
#define Pi64 3.1415926535897932

// Note (Aaron H): Must be defined before including the reference values header
typedef struct reference_answer reference_answer;
struct reference_answer
{
    F64 Input;
    F64 Output;
};

#include "reference_values_wolfram.h"


typedef F64 math_func(F64);

typedef struct math_test math_test;
struct math_test
{
    F64 MaxDiff;
};


typedef struct function_error function_error;
struct function_error
{
    F64 SampleCount;
    F64 MaxDiff;
    F64 TotalDiff;

    F64 InputValueAtMaxDiff;
    F64 OutputValueAtMaxDiff;
    F64 ExpectedValueAtMaxDiff;

    char Label[64];
};


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


global_function F64 CustomSqrt(F64 value)
{
    __m128d xmmValue = _mm_set_sd(value);
    __m128d xmmZero = _mm_set_sd(0);
    __m128d xmmResult = _mm_sqrt_sd(xmmZero, xmmValue);
    F64 result = _mm_cvtsd_f64(xmmResult);

    return result;
}


global_function void CheckHardcodedAnswer(char *label, math_func referenceFunc, reference_answer *answers, size_t answersCount)
{
    printf("%s:\n", label);
    for (int i = 0; i < answersCount; ++i)
    {
        reference_answer answer = answers[i];
        printf("  f(%+.16f) = %+.16f [reference]\n", answer.Input, answer.Output);
        F64 output = referenceFunc(answer.Input);
        printf("            = %+.16f (%+.16f) [%s]\n", output, answer.Output - output, label);
    }
    printf("\n");
}


global_function function_error MeasureMaximumFunctionError(math_func mathFunc, math_func referenceFunc, F64 minInputValue, F64 maxInputValue, U32 stepCount, char const *format, ...)
{
    function_error result = {0};

    if (mathFunc && referenceFunc)
    {
        // Set Label on error
        va_list argList;
        va_start(argList, format);
        vsnprintf(result.Label, sizeof(result.Label), format, argList);
        va_end(argList);

        // Determine largest diff
        for (U32 i = 0; i < stepCount; ++i)
        {
            F64 tStep = (F64)i / (F64)(stepCount - 1);
            F64 inputValue = ((1.0 - tStep) * minInputValue) + (tStep * maxInputValue);

            F64 outputValue = mathFunc(inputValue);
            F64 referenceOutputValue = referenceFunc(inputValue);
            F64 diff = fabs(outputValue - referenceOutputValue);

            if (diff > result.MaxDiff)
            {
                result.MaxDiff = diff;
                result.InputValueAtMaxDiff = inputValue;
                result.OutputValueAtMaxDiff = outputValue;
                result.ExpectedValueAtMaxDiff = referenceOutputValue;
            }

            result.TotalDiff += diff;
            result.SampleCount++;
        }
    }
    else
    {
        fprintf(stderr, "[ERROR] Invalid function or reference function\n");
    }

    return result;
}


global_function void PrintError(function_error error)
{
    F64 averageDiff = error.SampleCount ? (error.TotalDiff / (F64)error.SampleCount) : 0;
    printf("%+.16f (%+.16f) at %+.16f [%s]\n", error.MaxDiff, averageDiff, error.InputValueAtMaxDiff, error.Label);
}


int main(int argc, char const *argv[])
{
    CheckHardcodedAnswer("sin", sin, Reference_Sin, ArrayCount(Reference_Sin));
    CheckHardcodedAnswer("cos", cos, Reference_Cos, ArrayCount(Reference_Cos));
    CheckHardcodedAnswer("asin", asin, Reference_ArcSin, ArrayCount(Reference_ArcSin));
    CheckHardcodedAnswer("sqrt", sqrt, Reference_Sqrt, ArrayCount(Reference_Sqrt));

    U32 stepCount = 100000000;

    printf("Maximum function error:\n");
    function_error errorSin = MeasureMaximumFunctionError(CustomSin, sin, -Pi64, Pi64, stepCount, "CustomSin");
    PrintError(errorSin);

    function_error errorCos = MeasureMaximumFunctionError(CustomCos, cos, -Pi64/2, Pi64/2, stepCount, "CustomCos");
    PrintError(errorCos);

    function_error errorASin = MeasureMaximumFunctionError(CustomArcSin, asin, 0, 1, stepCount, "CustomArcSin");
    PrintError(errorASin);

    function_error errorSqrt = MeasureMaximumFunctionError(CustomSqrt, sqrt, 0, 1, stepCount, "CustomSqrt");
    PrintError(errorSqrt);

    return 0;
}
