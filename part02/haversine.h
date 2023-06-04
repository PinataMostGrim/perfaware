#ifndef HAVERSINE_H
#define HAVERSINE_H

#include "base.h"

#include <math.h>

#define EARTH_RADIUS 6372.8
#define DATA_FILENAME "haversine-pairs.json"
#define ANSWER_FILENAME "haversine-answer.f64"

function F64 Square(F64 A);
function F64 RadiansFromDegrees(F64 Degrees);
function F64 ReferenceHaversine(F64 X0, F64 Y0, F64 X1, F64 Y1, F64 EarthRadius);

#endif // HAVERSINE_H
