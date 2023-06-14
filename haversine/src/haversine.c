#include "base.h"
#include "haversine.h"

#include <math.h>


global_function F64 Square(F64 A)
{
    F64 Result = (A*A);
    return Result;
}


global_function F64 RadiansFromDegrees(F64 Degrees)
{
    F64 Result = 0.01745329251994329577f * Degrees;
    return Result;
}


// NOTE(casey): EarthRadius is generally expected to be 6372.8
global_function F64 ReferenceHaversine(F64 X0, F64 Y0, F64 X1, F64 Y1, F64 EarthRadius)
{
    /* NOTE(casey): This is not meant to be a "good" way to calculate the Haversine distance.
       Instead, it attempts to follow, as closely as possible, the formula used in the real-world
       question on which these homework exercises are loosely based.
    */

    F64 lat1 = Y0;
    F64 lat2 = Y1;
    F64 lon1 = X0;
    F64 lon2 = X1;

    F64 dLat = RadiansFromDegrees(lat2 - lat1);
    F64 dLon = RadiansFromDegrees(lon2 - lon1);
    lat1 = RadiansFromDegrees(lat1);
    lat2 = RadiansFromDegrees(lat2);

    F64 a = Square(sin(dLat/2.0)) + cos(lat1)*cos(lat2)*Square(sin(dLon/2));
    F64 c = 2.0*asin(sqrt(a));

    F64 Result = EarthRadius * c;

    return Result;
}
