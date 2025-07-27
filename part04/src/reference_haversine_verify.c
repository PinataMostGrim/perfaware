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


static F64 Square(F64 x)
{
    F64 result = x*x;
    return result;
}


static F64 CustomSin(F64 x)
{
    F64 x2 = Square(x);
    F64 a = -4.0 / Square(Pi64);
    F64 b = 4.0 / Pi64;

    F64 result = (a * x2) + (b * x);
    return result;
}


static F64 CustomSinRR(F64 x)
{
    // Range reduction

    // If input is 0 or above, do nothing
    // If input is below 0, invert it so that it is above zero, and then invert the answer before returning later.
    double multiplier = (x >= 0) ? 1 : -1;
    x *= multiplier;

    F64 x2 = Square(x);
    F64 a = -4.0 / Square(Pi64);
    F64 b = 4.0 / Pi64;

    F64 result = (a * x2) + (b * x);
    result *= multiplier;

    return result;
}


static F64 CustomCos(F64 input)
{
    return 0;
}


static F64 CustomArcSin(F64 input)
{
    return 0;
}


static F64 CustomSqrt(F64 x)
{
    __m128d xmmValue = _mm_set_sd(x);
    __m128d xmmZero = _mm_set_sd(0);
    __m128d xmmResult = _mm_sqrt_sd(xmmZero, xmmValue);
    F64 result = _mm_cvtsd_f64(xmmResult);

    return result;
}


static void CheckHardcodedAnswer(char *label, math_func referenceFunc, reference_answer *answers, size_t answersCount)
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


static function_error MeasureMaximumFunctionError(math_func mathFunc, math_func referenceFunc, F64 minInputValue, F64 maxInputValue, U32 stepCount, char const *format, ...)
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
            F64 diff = fabs(referenceOutputValue - outputValue);

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


static void PrintError(function_error error)
{
    F64 averageDiff = error.SampleCount ? (error.TotalDiff / (F64)error.SampleCount) : 0;
    printf("%+.16f at %+.16f (%+.16f) [%s]\n", error.MaxDiff, error.InputValueAtMaxDiff, averageDiff, error.Label);
}


int main(int argc, char const *argv[])
{
    // Note (Aaron): Enable to verify reference values
#if 0
    CheckHardcodedAnswer("sin", sin, Reference_Sin, ArrayCount(Reference_Sin));
    CheckHardcodedAnswer("cos", cos, Reference_Cos, ArrayCount(Reference_Cos));
    CheckHardcodedAnswer("asin", asin, Reference_ArcSin, ArrayCount(Reference_ArcSin));
    CheckHardcodedAnswer("sqrt", sqrt, Reference_Sqrt, ArrayCount(Reference_Sqrt));
#endif

    U32 stepCount = 100000000;

    printf("Calulating maximum function error:\n");

    function_error error = MeasureMaximumFunctionError(CustomSin, sin, 0, Pi64, stepCount, "CustomSin: 0 to Pi");
    PrintError(error);

    error = MeasureMaximumFunctionError(CustomSin, sin, -Pi64, Pi64, stepCount, "CustomSin: -Pi to Pi");
    PrintError(error);

    error = MeasureMaximumFunctionError(CustomSinRR, sin, -Pi64, Pi64, stepCount, "CustomSinRR: -Pi to Pi");
    PrintError(error);

    function_error errorCos = MeasureMaximumFunctionError(CustomCos, cos, -Pi64/2, Pi64/2, stepCount, "CustomCos");
    PrintError(errorCos);

    function_error errorASin = MeasureMaximumFunctionError(CustomArcSin, asin, 0, 1, stepCount, "CustomArcSin");
    PrintError(errorASin);

    function_error errorSqrt = MeasureMaximumFunctionError(CustomSqrt, sqrt, 0, 1, stepCount, "CustomSqrt");
    PrintError(errorSqrt);

    return 0;
}
