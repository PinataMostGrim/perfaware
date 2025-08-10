#include <math.h>
#include <stdio.h>

#include "math_tester.h"


void CheckHardcodedAnswer(char *label, math_function referenceFunc, math_answer *answers, size_t answersCount)
{
    printf("%s:\n", label);
    for (int i = 0; i < answersCount; ++i)
    {
        math_answer answer = answers[i];
        printf("  f(%+.16f) = %+.16f [reference]\n", answer.Input, answer.Output);
        F64 output = referenceFunc(answer.Input);
        printf("            = %+.16f (%+.16f) [%s]\n", output, answer.Output - output, label);
    }
    printf("\n");
}


static F64 GetAvgDiff(math_test_result from)
{
    F64 result = from.SampleCount ? (from.TotalDiff / (F64)from.SampleCount) : 0;
    return result;

}


static void PrintResult(math_test_result result)
{
    printf("%+.16f at %+.16f (%+.16f) [%s]\n", result.MaxDiff, result.InputValueAtMaxDiff, GetAvgDiff(result), result.Label);
}


static B32 PrecisionTest(math_tester *tester, F64 minInputValue, F64 maxInputValue, U32 stepCount)
{
    if (!tester->Testing)
    {
        // Note (Aaron): This is a new test
        tester->Testing = TRUE;
        tester->StepIndex = 0;
    }
    else
    {
        tester->StepIndex++;
    }

    if (tester->StepIndex < stepCount)
    {
        // Note (Aaron): Continue testing
        tester->ResultOffset = 0;

        F64 tStep = (F64)tester->StepIndex / (F64)(stepCount - 1);
        tester->InputValue = ((1.0 - tStep) * minInputValue) + (tStep * maxInputValue);
    }
    else
    {
        // Note (Aaron): End the test
        tester->ResultCount += tester->ResultOffset;
        if (tester->ResultCount > ArrayCount(tester->Results))
        {
            tester->ResultCount = ArrayCount(tester->Results);
            fprintf(stderr, "Out of room to store math test results.\n");
        }

        if (tester->ProgressResultCount < tester->ResultCount)
        {
            while (tester->ProgressResultCount < tester->ResultCount)
            {
                PrintResult(tester->Results[tester->ProgressResultCount++]);
            }
        }

        tester->Testing = FALSE;
    }

    B32 result = tester->Testing;
    return result;
}


static void TestResult(math_tester *tester, F64 expected, F64 output, char const *format, ...)
{
    U32 resultIndex = tester->ResultCount + tester->ResultOffset;
    math_test_result *result = &tester->ErrorResult;

    if (resultIndex < ArrayCount(tester->Results))
    {
        result = tester->Results + resultIndex;
    }

    // Note (Aaron): Clear the result and set the label
    if (tester->StepIndex == 0)
    {
        // Note (Aaron): Zero out the result struct
        uint64_t count = sizeof(*result);
        void *destPtr = result;
        unsigned char *dest = (unsigned char *)destPtr;
        while(count--) *dest++ = (unsigned char)0;

        va_list argList;
        va_start(argList, format);
        vsnprintf(result->Label, sizeof(result->Label), format, argList);
        va_end(argList);
    }

    F64 diff = fabs(expected - output);
    result->TotalDiff += diff;
    ++result->SampleCount;

    if (result->MaxDiff < diff)
    {
        result->MaxDiff = diff;
        result->InputValueAtMaxDiff = tester->InputValue;
        result->OutputValueAtMaxDiff = output;
        result->ExpectedValueAtMaxDiff = expected;
    }

    ++tester->ResultOffset;
}
