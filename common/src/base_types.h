/* TODO (Aaron):
    - Resolve the nonstandard extension use for nameless structs / unions in C
*/

// Note (Aaron): Typedefs

#ifndef BASE_TYPES_H
#define BASE_TYPES_H

#include <stdint.h>

#include "base.h"

typedef uint8_t  U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;

typedef int8_t  S8;
typedef int16_t S16;
typedef int32_t S32;
typedef int64_t S64;

typedef S8  B8;
typedef S16 B16;
typedef S32 B32;
typedef S64 B64;

typedef float  F32;
typedef double F64;


// +------------------------------+
// Note (Aaron): Floats

global_variable U32 Sign32 = 0x80000000;
global_variable U64 Sign64 = 0x8000000000000000;

global_function F32 AbsF32(F32 f);
global_function F64 AbsF64(F64 f);


// +------------------------------+
// Note (Aaron): Compound Types

typedef struct
{
    union
    {
        struct
        {
            F32 x;
            F32 y;
        };
        F32 v[2];
    };
} V2F32;

typedef struct
{
    union
    {
        struct
        {
            F32 x;
            F32 y;
            F32 z;
        };
        F32 v[3];
    };
} V3F32;

typedef struct
{
    union
    {
        struct
        {
            F32 x;
            F32 y;
            F32 z;
            F32 w;
        };
        F32 v[4];
    };
} V4F32;

typedef struct
{
    union
    {
        struct
        {
            F64 x;
            F64 y;
        };
        F64 v[2];
    };
}  V2F64;

typedef struct
{
    union
    {
        struct
        {
            F64 x;
            F64 y;
            F64 z;
        };
        F64 v[3];
    };
} V3F64;

typedef struct
{
    union
    {
        struct
        {
            F64 x;
            F64 y;
            F64 z;
            F64 w;
        };
        F64 v[4];
    };
}  V4F64;


// TODO (Aaron): I don't like the naming convention of these methods, but I can't think of a better way right now
// Maybe constructors have a trailing '_' character?
global_function V2F32 v2f32(F32 x, F32 y);
global_function V2F64 v2f64(F64 x, F64 y);
global_function V3F32 v3f32(F32 x, F32 y, F32 z);
global_function V3F64 v3f64(F64 x, F64 y, F64 z);
global_function V4F32 v4f32(F32 x, F32 y, F32 z, F32 w);
global_function V4F64 v4f64(F64 x, F64 y, F64 z, F64 w);

#endif // BASE_TYPES_H
