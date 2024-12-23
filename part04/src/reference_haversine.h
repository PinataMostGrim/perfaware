#ifndef HAVERSINE_H
#define HAVERSINE_H

#include "base_inc.h"

#define EARTH_RADIUS 6372.8
#define DATA_FILENAME "haversine-pairs.json"

/* Note (Aaron): The answer file should be a binary file structured as follows:
    - answers_file_header
    - A series of F64s containing the distance calculated from the matching coordinate pairs on the pairs JSON file
*/
#define ANSWER_FILENAME "haversine-answer.f64"


typedef struct answers_file_header answers_file_header;
struct answers_file_header
{
    unsigned int Seed;
    U64 PairCount;
    F64 ExpectedSum;
};


typedef struct haversine_pair haversine_pair;
struct haversine_pair
{
    V2F64 point0;
    V2F64 point1;
};


typedef struct haversine_setup haversine_setup;
struct haversine_setup
{
    memory_arena JsonArena;
    memory_arena AnswersArena;
    memory_arena PairsArena;
    memory_arena TokenArena;

    U64 ParsedByteCount;

    U64 PairCount;
    haversine_pair *Pairs;
    F64 *Answers;

    F64 SumAnswer;
    B64 Valid;
};


global_function memory_arena ReadFileContents(char *filename);
global_function haversine_setup SetupHaversine(char *haversinePairsFilename, char *answersFilename);
global_function B32 SetupIsValid(haversine_setup setup);
global_function void FreeHaversine(haversine_setup *setup);

global_function F64 Square(F64 A);
global_function F64 RadiansFromDegrees(F64 Degrees);

global_function F64 ReferenceHaversine(F64 X0, F64 Y0, F64 X1, F64 Y1, F64 EarthRadius);
global_function F64 ReferenceSumHaversine(haversine_setup setup);
global_function U64 ReferenceVerifyHaversine(haversine_setup setup);

#endif // HAVERSINE_H
