#include "base.h"
#include "base_types.h"

// +------------------------------+
// Note (Aaron): Compound Types

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
