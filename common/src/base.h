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

#define global_variable static
#define global_function static
#define local_persist static

#define TRUE 1
#define FALSE 0

#define C_LINKAGE_BEGIN extern "C"{
#define C_LINKAGE_END }
#define C_LINKAGE extern "C"

#define Max(x, y) ((x) > (y)) ? (x) : (y)
#define Min(x, y) ((x) < (y)) ? (x) : (y)
#define Clamp(a, x, b) (((x) < (a)) ? (a) : ((b) < (x)) ? (b) : (x))

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

global_function V2F32 v2f32(F32 x, F32 y)
{
    V2F32 r = { r.x = x, r.y = y };
    return r;
}

global_function V2F64 v2f64(F64 x, F64 y)
{
    V2F64 r = { r.x = x, r.y = y };
    return r;
}

global_function V3F32 v3f32(F32 x, F32 y, F32 z)
{
    V3F32 r = { r.x = x, r.y = y, r.z = z };
    return r;
}

global_function V3F64 v3f64(F64 x, F64 y, F64 z)
{
    V3F64 r = { r.x = x, r.y = y, r.z = z };
    return r;
}

global_function V4F32 v4f32(F32 x, F32 y, F32 z, F32 w)
{
    V4F32 r = { r.x = x, r.y = y, r.z = z, r.w = w };
    return r;
}

global_function V4F64 v4f64(F64 x, F64 y, F64 z, F64 w)
{
    V4F64 r = { r.x = x, r.y = y, r.z = z, r.w = w };
    return r;
}


// +------------------------------+
// Note (Aaron): Helper Functions

global_function void *MemorySet(uint8_t *destPtr, int c, size_t count)
{
    Assert(count > 0 && "Attempted to set 0 bytes");

    unsigned char *dest = (unsigned char *)destPtr;
    while(count--) *dest++ = (unsigned char)c;

    return destPtr;
}


global_function void *MemoryCopy(void *destPtr, void const *sourcePtr, size_t size)
{
    // TODO (Aaron): Return instead? Or does this assert catch cases we want to know about?
    Assert(size > 0 && "Attempted to copy 0 bytes");

    unsigned char *source = (unsigned char *)sourcePtr;
    unsigned char *dest = (unsigned char *)destPtr;
    while(size--) *dest++ = *source++;

    return destPtr;
}


global_function U64 GetStringLength(char *str)
{
    U64 count = 0;
    while(*str++) count++;

    return count;
}


#endif // BASE_H
