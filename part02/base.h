#ifndef BASE_H
#define BASE_H

// Note (Aaron):
// Based heavily on Allen Webster's base layer.
//  source: https://www.youtube.com/watch?v=8fJ4vWrkS4o&list=PLT6InxK-XQvNKTyLXk6H6KKy12UYS_KDL&index=1

// +------------------------------+
// Note (Aaron): Helper Macros

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#if !defined(ENABLE_ASSERT)
#   define ENABLE_ASSERT
#endif

#define Stmnt(S) do{ S }while(0)
#define AssertBreak() (*(int*)0 = 0)

#ifdef ENABLE_ASSERT
#   define Assert(c) Stmnt( if (!(c)){ AssertBreak(); } )
#else
#   define Assert(c)
#endif

#define global static
#define function static

#define c_linkage_begin extern "C"{
#define c_linkage_end }
#define c_linkage extern "C"

#define TRUE 1
#define FALSE 0


// +------------------------------+
// Note (Aaron): Typedefs

#include <stdint.h>
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


// +------------------------------+
// Note (Aaron): Compound Type Functions

function V2F32 v2f32(F32 x, F32 y)
{
    V2F32 r = { x = x, y = y };
    return r;
}

function V2F64 v2f64(F64 x, F64 y)
{
    V2F64 r = { x = x, y = y };
    return r;
}

function V3F32 v3f32(F32 x, F32 y, F32 z)
{
    V3F32 r = { x = x, y = y, z = z };
    return r;
}

function V3F64 v3f64(F64 x, F64 y, F64 z)
{
    V3F64 r = { x = x, y = y, z = z };
    return r;
}

function V4F32 v4f32(F32 x, F32 y, F32 z, F32 w)
{
    V4F32 r = { x = x, y = y, z = z, w = w };
    return r;
}

function V4F64 v4f64(F64 x, F64 y, F64 z, F64 w)
{
    V4F64 r = { x = x, y = y, z = z, w = w };
    return r;
}

#endif // BASE_H
