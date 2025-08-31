#ifndef MATH_TESTER_H
#define MATH_TESTER_H


typedef struct math_answer math_answer;
struct math_answer
{
    F64 Input;
    F64 Output;
};


typedef F64 math_function(F64);

typedef struct math_test_result math_test_result;
struct math_test_result
{
    U32 SampleCount;
    F64 MaxDiff;
    F64 TotalDiff;

    F64 InputValueAtMaxDiff;
    F64 OutputValueAtMaxDiff;
    F64 ExpectedValueAtMaxDiff;

    char Label[64];
};


typedef struct math_tester math_tester;
struct math_tester
{
    math_test_result Results[256];
    math_test_result ErrorResult;

    U32 ResultCount;
    U32 ProgressResultCount;

    B32 Testing;
    U32 StepIndex;
    U32 ResultOffset;

    F64 InputValue;
};


void CheckHardcodedAnswer(char *label, math_function referenceFunc, math_answer *answers, size_t answersCount);

static void PrintResult(math_test_result result);
static B32 PrecisionTest(math_tester *tester, F64 minInputValue, F64 maxInputValue, U32 stepCount);
static void TestResult(math_tester *tester, F64 expected, F64 output, char const *format, ...);

#endif // MATH_TESTER_H
