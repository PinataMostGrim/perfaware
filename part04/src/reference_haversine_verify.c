#include <float.h>
#include <math.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>

#include "base_inc.h"
#include "base_types.c"
#include "math_tester.h"
#include "math_tester.c"
#include "reference_values_wolfram.h"

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


static U64 Factorial(U32 value)
{
    U64 result = value;
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
    // Note (Aaron): Enable to verify reference values
#if 0
    CheckHardcodedAnswer("sin", sin, Reference_Sin, ArrayCount(Reference_Sin));
    CheckHardcodedAnswer("cos", cos, Reference_Cos, ArrayCount(Reference_Cos));
    CheckHardcodedAnswer("asin", asin, Reference_ArcSin, ArrayCount(Reference_ArcSin));
    CheckHardcodedAnswer("sqrt", sqrt, Reference_Sqrt, ArrayCount(Reference_Sqrt));
#endif


#if 1
    U32 result = Factorial(5);
    F64 result = Exponent(5, 1);
    fprintf(stdout, "result: %f\n", result);

    F64 value = -Pi64;
    F64 reference = sin(value);
    F64 result = CustomSinTaylor(value, 19);

    fprintf(stdout, "reference: %f\n", reference);
    fprintf(stdout, "result: %f\n", result);
#endif

#if 0
    math_tester tester = {0};
    U32 stepCount = 100000000;
    printf("Calulating maximum function errors:\n");

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

#endif


    return 0;
}
