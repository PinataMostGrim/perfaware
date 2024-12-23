#include <math.h>
#include <stdio.h>
#include <sys/stat.h>

#include "base_inc.h"

#include "buffer.h"

#include "reference_haversine.h"
#include "reference_haversine_lexer.h"
#include "reference_haversine_parser.h"

#include "base_types.c"
#include "base_memory.c"
#include "base_arena.c"
#include "base_string.c"

#include "buffer.c"

#include "reference_haversine.c"
#include "reference_haversine_lexer.c"
#include "reference_haversine_parser.c"


typedef struct haversine_ranges haversine_ranges;
struct haversine_ranges
{
    V2F64 sin_input_range;
    V2F64 sin_output_range;

    V2F64 cos_input_range;
    V2F64 cos_output_range;

    V2F64 square_input_range;
    V2F64 square_output_range;
};


global_function void UpdateRanges(V2F64 *range, F64 value)
{
    if (value < range->x)
    {
        range->x = value;
    }
    else if (value > range->y)
    {
        range->y = value;
    }
}


global_function F64 CalculateHaversineWithRanges(F64 X0, F64 Y0, F64 X1, F64 Y1, F64 EarthRadius, haversine_ranges *ranges)
{
    F64 lat1 = Y0;
    F64 lat2 = Y1;
    F64 lon1 = X0;
    F64 lon2 = X1;

    F64 dLat = RadiansFromDegrees(lat2 - lat1);
    F64 dLon = RadiansFromDegrees(lon2 - lon1);
    lat1 = RadiansFromDegrees(lat1);
    lat2 = RadiansFromDegrees(lat2);

    F64 dLatSinInput = dLat/2.0;
    UpdateRanges(&ranges->sin_input_range, dLatSinInput);

    F64 dLatSin = sin(dLatSinInput);
    UpdateRanges(&ranges->sin_output_range, dLatSin);

    F64 dLonSinInput = dLon/2;
    UpdateRanges(&ranges->sin_input_range, dLonSinInput);

    F64 dLonSin = sin(dLonSinInput);
    UpdateRanges(&ranges->sin_output_range, dLonSin);

    F64 lat1Cos = cos(lat1);
    UpdateRanges(&ranges->cos_input_range, lat1);
    UpdateRanges(&ranges->cos_output_range, lat1Cos);

    F64 lat2Cos = cos(lat2);
    UpdateRanges(&ranges->cos_input_range, lat2);
    UpdateRanges(&ranges->cos_output_range, lat2Cos);

    F64 dLatSinSquare = Square(dLatSin);
    F64 dLonSinSquare = Square(dLonSin);

    F64 a = dLatSinSquare + lat1Cos * lat2Cos * dLonSinSquare;

    // TODO (Aaron): Add ranges for asin and sqrt
    F64 c = 2.0*asin(sqrt(a));

    F64 Result = EarthRadius * c;

    return Result;
}


global_function F64 ReferenceSumHaversineRanges(haversine_setup setup, haversine_ranges *ranges)
{
    U64 pairCount = setup.PairCount;
    haversine_pair *pairs = setup.Pairs;

    F64 sum = 0;

    F64 sumCoeficient = 1 / (F64)pairCount;
    for (U64 pairIndex = 0; pairIndex < pairCount; ++pairIndex)
    {
        haversine_pair pair = pairs[pairIndex];
        F64 earthRadius = EARTH_RADIUS;
        F64 dist = CalculateHaversineWithRanges(pair.point0.x, pair.point0.y, pair.point1.x, pair.point1.y, earthRadius, ranges);
        sum += sumCoeficient * dist;
    }

    return sum;
}


global_function void PrintHaversineRanges(haversine_ranges ranges)
{
    fprintf(stdout, "Ranges:\n");

    fprintf(stdout, "Sine input, min: %f, max: %f\n", ranges.sin_input_range.x, ranges.sin_input_range.y);
    fprintf(stdout, "Sine output, min: %f, max: %f\n", ranges.sin_output_range.x, ranges.sin_output_range.y);

    fprintf(stdout, "Cos input, min: %f, max: %f\n", ranges.cos_input_range.x, ranges.cos_input_range.y);
    fprintf(stdout, "Cos output, min: %f, max: %f\n", ranges.cos_output_range.x, ranges.cos_output_range.y);
    fprintf(stdout, "\n");
}


int main(int argCount, char const *args[])
{
    if (argCount != 3)
    {
        fprintf(stderr, "Usage: %s [haversine pairs file] [haversine answers file]\n", args[0]);
        return 1;
    }

    char *haversinePairsFilename = (char *)args[1];
    char *answersFilename = (char *)args[2];

    haversine_setup setup = SetupHaversine(haversinePairsFilename, answersFilename);
    haversine_ranges ranges = {0};

    if (SetupIsValid(setup))
    {
        F64 referenceSum = setup.SumAnswer;

        U64 individualErrorCount = ReferenceVerifyHaversine(setup);
        F64 computedSum = ReferenceSumHaversineRanges(setup, &ranges);

        U64 sumErrorCount = 0;
        sumErrorCount += !ApproxAreEqual(computedSum, referenceSum);

        if (sumErrorCount || individualErrorCount)
        {
            fprintf(stderr, "[WARNING]: %lu haversines mismatched, %lu sum mismatches\n",
                    individualErrorCount, sumErrorCount);
        }

        fprintf(stdout, "\n");
        PrintHaversineRanges(ranges);
        fprintf(stdout, "\n");
    }
    else
    {
        fprintf(stderr, "[ERROR]: Test data size must be non-zero\n");
    }

    FreeHaversine(&setup);

    return 0;
}
