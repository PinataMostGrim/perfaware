#include <float.h>
#include <math.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>

#include "base_arena.h"
#include "base_string.h"

#include "base_types.c"
#include "base_memory.c"
#include "base_arena.c"
#include "base_string.c"

#include "math_tester.h"
#include "math_tester.c"
#include "reference_values_wolfram.h"

#define PLATFORM_METRICS_IMPLEMENTATION
#define PROFILER 1
#include "platform_metrics.h"

#if _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

// source: https://www.wolframalpha.com/input?i=N%5BPi%2C+17%5D
// N[Pi, 17]
#define Pi64 3.1415926535897932
#define DegToRad(degrees) (0.01745329251994329577 * degrees)
#define RadToDeg(radians) (57.29577951308232 * radians)


static F64 Square(F64 x)
{
    F64 result = x*x;
    return result;
}


static F64 CustomSin(F64 x)
{
    // sin approximation
    // ax^2 * bx where:
    //  a = -4/Pi^2
    //  b = 4/Pi
    F64 x2 = Square(x);
    F64 a = -4.0 / Square(Pi64);
    F64 b = 4.0 / Pi64;

    F64 result = (a * x2) + (b * x);
    return result;
}


static F64 CustomSinHalf(F64 x)
{
    // Range reduction

    // If input is 0 or above, do nothing
    // If input is below 0, invert it so that it is above zero, and then invert the answer before returning later.
    double multiplier = (x >= 0) ? 1 : -1;
    x *= multiplier;

    // sin approximation
    // ax^2 * bx where:
    //  a = -4/Pi^2
    //  b = 4/Pi
    F64 x2 = Square(x);
    F64 a = -4.0 / Square(Pi64);
    F64 b = 4.0 / Pi64;

    F64 result = (a * x2) + (b * x);
    result *= multiplier;

    return result;
}


static F64 Factorial(U32 value)
{
    F64 result = (F64)value;
    for (int i = value - 1; i > 0; --i)
    {
        result *= i;
    }

    return result;
}


static F64 Exponent(F64 base, U32 power)
{
    F64 result = base;
    if (power == 0)
    {
        result = 1;
    }
    else
    {
        for (U32 i = 1; i < power; ++i)
        {
            result *= base;
        }
    }

    return result;
}


static F64 CustomSinTaylor(F64 x, U32 power)
{
    // Taylor series estimation for sin:
    // x - (X^3)/(3!) + (x^5)/(5!) - (x^7)/(7!) + ...

    F64 result = x;
    B32 add = FALSE;
    for (U32 i = 3; i <= power; i+=2)
    {
        F64 dividend = Exponent(x, i);
        F64 divisor = (F64)Factorial(i);
        F64 term = dividend / divisor;
        result = add ? (result + term) : (result - term);

        add = !add;
    }

    return result;
}


static F64 CustomTaylorSeriesCoefficient(U32 power)
{
    F64 sign = ((power - 1) / 2) % 2 ? -1.0 : 1.0;
    F64 result = (sign / Factorial(power));

    return result;
}


static F64 CustomSinTaylorHorner(F64 x, U32 maxPower)
{
    F64 result = 0;

    F64 x2 = x*x;
    for (U32 inversePower = 1; inversePower <= maxPower; inversePower += 2)
    {
        U32 power = maxPower - (inversePower - 1);
        result = result * x2 + CustomTaylorSeriesCoefficient(power);
    }

    result *= x;

    return result;
}


static F64 CustomSinTaylorHornerFusedMultiply(F64 x, U32 maxPower)
{
    F64 result = 0;

    F64 x2 = x*x;
    for (U32 inversePower = 1; inversePower <= maxPower; inversePower += 2)
    {
        U32 power = maxPower - (inversePower - 1);
        result = fma(result, x2, CustomTaylorSeriesCoefficient(power));
    }

    result *= x;

    return result;
}


static F64 CustomSinTaylorHornerFusedMultiplyIntrinsic(F64 x, U32 maxPower)
{
    F64 result = 0;

    F64 x2 = x*x;
    for (U32 inversePower = 1; inversePower <= maxPower; inversePower += 2)
    {
        U32 power = maxPower - (inversePower - 1);

        __m128d xmmA = _mm_set_sd(result);
        __m128d xmmB = _mm_set_sd(x2);
        __m128d xmmC = _mm_set_sd(CustomTaylorSeriesCoefficient(power));
        __m128d fmaResult = _mm_fmadd_sd(xmmA, xmmB, xmmC);
        result = _mm_cvtsd_f64(fmaResult);
    }

    result *= x;

    return result;
}


static F64 CustomCos(F64 input)
{
    F64 result = CustomSinHalf(input + (Pi64 / 2));
    return result;
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


int main(int argc, char const *argv[])
{
    StartTimingsProfile();

    math_tester tester = {0};
    U32 stepCount = 100000000;

#if 0
    // Note (Aaron): Verify reference values
    printf("Verifying reference values:\n");

    START_TIMING(VerifyReferenceValues);
    CheckHardcodedAnswer("sin", sin, Reference_Sin, ArrayCount(Reference_Sin));
    CheckHardcodedAnswer("cos", cos, Reference_Cos, ArrayCount(Reference_Cos));
    CheckHardcodedAnswer("asin", asin, Reference_ArcSin, ArrayCount(Reference_ArcSin));
    CheckHardcodedAnswer("sqrt", sqrt, Reference_Sqrt, ArrayCount(Reference_Sqrt));
    END_TIMING(VerifyReferenceValues);

    printf("\n");
#endif

#if 0
    // Note (Aaron): Precision test custom haversine math functions
    printf("Calulating maximum function errors:\n");

    START_TIMING(CustomHaversineFunctions);
    while(PrecisionTest(&tester, 0, Pi64, stepCount))
    {
        TestResult(&tester, sin(tester.InputValue), CustomSin(tester.InputValue), "CustomSin: 0 to Pi");
    }

    while(PrecisionTest(&tester, -Pi64, Pi64, stepCount))
    {
        TestResult(&tester, sin(tester.InputValue), CustomSin(tester.InputValue), "CustomSin: -Pi to Pi");
    }

    while(PrecisionTest(&tester, -Pi64, Pi64, stepCount))
    {
        TestResult(&tester, sin(tester.InputValue), CustomSinHalf(tester.InputValue), "CustomSinHalf: -Pi to Pi");
    }

    while(PrecisionTest(&tester, -Pi64/2, Pi64/2, stepCount))
    {
        TestResult(&tester, cos(tester.InputValue), CustomCos(tester.InputValue), "CustomCos: -Pi/2 to Pi/2");
    }

    while(PrecisionTest(&tester, 0, 1, stepCount))
    {
        TestResult(&tester, asin(tester.InputValue), CustomArcSin(tester.InputValue), "CustomArcSin: 0 to 1");
    }

    while(PrecisionTest(&tester, 0, 1, stepCount))
    {
        TestResult(&tester, sqrt(tester.InputValue), CustomSqrt(tester.InputValue), "CustomSqrt: 0 to 1");
    }
    END_TIMING(CustomHaversineFunctions);

    printf("\n");
#endif

#if 1
    // Note (Aaron): Precision test multiple Taylor series higher power approximations
    printf("\nCalulating maximum function errors for Taylor series approxmination:\n");

    // Allocate an arena for timing labels
    memory_arena labelsArena = ArenaAllocate(Megabytes(1), Megabytes(1));

    START_TIMING(TaylorSeriesExpansion_Total);

    int taylorSeriesMaxPower = 31;

    for (int i = 1; i <= taylorSeriesMaxPower; i+=2)
    {
        // Construct a dynamic label for the taylor series
        char *taylorLabel = ArenaPushCStringf(&labelsArena, TRUE, "CustomSinTaylor(%i)                              ", i);
        zone_block zoneBlockTaylor = {0};
        _StartTiming(&zoneBlockTaylor, taylorLabel, (i * 4) + (__COUNTER__), 0);

        // Perform the precision test
        while(PrecisionTest(&tester, 0, Pi64/2, stepCount))
        {
            TestResult(&tester, sin(tester.InputValue), CustomSinTaylor(tester.InputValue, i), "CustomSinTaylor(%i): 0 to Pi/2", i);
        }

        _EndTiming(&zoneBlockTaylor);

        // Construct a dynamic label for the Horner implementation
        char *hornerLabel = ArenaPushCStringf(&labelsArena, TRUE, "CustomSinHorner(%i)                              ", i);
        zone_block zoneBlockHorner = {0};
        _StartTiming(&zoneBlockHorner, hornerLabel, (i * 4) + __COUNTER__, 0);

        // Perform the precision test
        while(PrecisionTest(&tester, 0, Pi64/2, stepCount))
        {
            TestResult(&tester, sin(tester.InputValue), CustomSinTaylorHorner(tester.InputValue, i), "CustomSinTaylorHorner(%i): 0 to Pi/2", i);
        }

        _EndTiming(&zoneBlockHorner);

        // Construct a dynamic label for the fused multiply add Horner implementation
        char *fusedMultiplyLabel = ArenaPushCStringf(&labelsArena, TRUE, "CustomSinTaylorHornerFusedMultiply(%i)           ", i);
        zone_block zoneBlockFma = {0};
        _StartTiming(&zoneBlockFma, fusedMultiplyLabel, (i * 4) + __COUNTER__, 0);

        // Perform the precision test
        while(PrecisionTest(&tester, 0, Pi64/2, stepCount))
        {
            TestResult(&tester, sin(tester.InputValue), CustomSinTaylorHornerFusedMultiply(tester.InputValue, i), "CustomSinTaylorHornerFusedMultiply(%i): 0 to Pi/2", i);
        }

        _EndTiming(&zoneBlockFma);

        // Construct a dynamic label for the fused multiply add Horner implementation using intrinsics
        char *fusedMultiplyIntrinsicsLabel = ArenaPushCStringf(&labelsArena, TRUE, "CustomSinTaylorHornerFusedMultiplyIntrinsics(%i) ", i);
        zone_block zoneBlockFmaIntrinsics = {0};
        _StartTiming(&zoneBlockFmaIntrinsics, fusedMultiplyIntrinsicsLabel, (i * 4) + __COUNTER__, 0);

        // Perform the precision test
        while(PrecisionTest(&tester, 0, Pi64/2, stepCount))
        {
            TestResult(&tester, sin(tester.InputValue), CustomSinTaylorHornerFusedMultiplyIntrinsic(tester.InputValue, i), "CustomSinTaylorHornerFusedMultiplyIntrinsic(%i): 0 to Pi/2", i);
        }

        _EndTiming(&zoneBlockFmaIntrinsics);
    }

    END_TIMING(TaylorSeriesExpansion_Total);

    printf("\n");
#endif

    EndTimingsProfile();
    PrintProfileTimings();

    return 0;
}

ProfilerEndOfCompilationUnit
