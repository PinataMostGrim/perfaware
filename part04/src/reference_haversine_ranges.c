#include <float.h>
#include <math.h>
#include <inttypes.h>
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


typedef struct range range;
struct range
{
    F64 Min;
    F64 Max;
};

typedef struct haversine_ranges haversine_ranges;
struct haversine_ranges
{
    range Sin;
    range Cos;
    range ArcSin;
    range Sqrt;
};


static haversine_ranges Ranges = {0};


static void RangeInclude(range *range, F64 value)
{
    if (value < range->Min) { range->Min = value; }
    if (value > range->Max) { range->Max = value; }
}


static F64 SinRange(F64 value)
{
    RangeInclude(&Ranges.Sin, value);
    F64 result = sin(value);

    return result;
}


static F64 CosRange(F64 value)
{
    RangeInclude(&Ranges.Cos, value);
    F64 result = cos(value);

    return result;
}


static F64 ArcSinRange(F64 value)
{
    RangeInclude(&Ranges.ArcSin, value);
    F64 result = asin(value);

    return result;
}


static F64 SqrtRange(F64 value)
{
    RangeInclude(&Ranges.Sqrt, value);
    F64 result = sqrt(value);

    return result;
}


static F64 ReferenceHaversineRanges(F64 X0, F64 Y0, F64 X1, F64 Y1, F64 EarthRadius)
{
    F64 lat1 = Y0;
    F64 lat2 = Y1;
    F64 lon1 = X0;
    F64 lon2 = X1;

    F64 dLat = RadiansFromDegrees(lat2 - lat1);
    F64 dLon = RadiansFromDegrees(lon2 - lon1);
    lat1 = RadiansFromDegrees(lat1);
    lat2 = RadiansFromDegrees(lat2);

    F64 a = Square(SinRange(dLat/2.0)) + CosRange(lat1)*CosRange(lat2)*Square(SinRange(dLon/2));
    F64 c = 2.0*ArcSinRange(SqrtRange(a));

    F64 Result = EarthRadius * c;

    return Result;
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
    if (SetupIsValid(setup))
    {
        range invertedInfinite = {DBL_MAX, -DBL_MAX};
        Ranges.Sin = invertedInfinite;
        Ranges.Cos = invertedInfinite;
        Ranges.ArcSin = invertedInfinite;
        Ranges.Sqrt = invertedInfinite;

        for (U64 pairIndex = 0; pairIndex < setup.PairCount; ++pairIndex)
        {
            haversine_pair pair = setup.Pairs[pairIndex];

            F64 distanceSum = ReferenceHaversineRanges(pair.point0.x, pair.point0.y, pair.point1.x, pair.point1.y, EARTH_RADIUS);
            F64 referenceSum = ReferenceHaversine(pair.point0.x, pair.point0.y, pair.point1.x, pair.point1.y, EARTH_RADIUS);
            if (distanceSum != referenceSum)
            {
                fprintf(stderr, "[ERROR] Range-check version doesn't match reference version.\n");
            }
        }

        fprintf(stdout, "\n");
        fprintf(stdout, "Ranges:\n");
        fprintf(stdout, "Sin: %f, %f\n", Ranges.Sin.Min, Ranges.Sin.Max);
        fprintf(stdout, "Cos: %f, %f\n", Ranges.Cos.Min, Ranges.Cos.Max);
        fprintf(stdout, "ArcSin: %f, %f\n", Ranges.ArcSin.Min, Ranges.ArcSin.Max);
        fprintf(stdout, "Sqrt: %f, %f\n", Ranges.Sqrt.Min, Ranges.Sqrt.Max);
        fprintf(stdout, "\n");
    }
    else
    {
        fprintf(stderr, "[ERROR]: Test data size must be non-zero\n");
    }

    FreeHaversine(&setup);

    return 0;
}
