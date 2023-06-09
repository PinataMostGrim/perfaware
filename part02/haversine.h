#ifndef HAVERSINE_H
#define HAVERSINE_H

#include "base.h"

#define EARTH_RADIUS 6372.8
#define DATA_FILENAME "haversine-pairs.json"

/* Note (Aaron): The answer file should be a binary file structured as follows:
    - answers_file_header
    - A series of F64s containing the distance calculated from the matching coordinate pairs on the pairs JSON file
*/
#define ANSWER_FILENAME "haversine-answer.f64"


typedef struct
{
    unsigned int Seed;
    F64 ExpectedSum;
}answers_file_header;


function F64 Square(F64 A);
function F64 RadiansFromDegrees(F64 Degrees);
function F64 ReferenceHaversine(F64 X0, F64 Y0, F64 X1, F64 Y1, F64 EarthRadius);

#endif // HAVERSINE_H
