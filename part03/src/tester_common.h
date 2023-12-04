#ifndef TESTER_COMMON_H
#define TESTER_COMMON_H

#include "../../common/src/repetition_tester.h"
#include "../../common/src/buffer.h"


typedef struct read_parameters read_parameters;
typedef struct test_function test_function;
typedef void read_overhead_test_func(repetition_tester *tester, read_parameters *params);

typedef enum
{
    AllocType_none,
    AllocType_malloc,

    AllocType_Count,
} allocation_type;


struct read_parameters
{
    char const *FileName;
    buffer Buffer;
    allocation_type AllocType;
};

#endif // TESTER_COMMON_H
