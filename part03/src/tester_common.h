#ifndef TESTER_COMMON_H
#define TESTER_COMMON_H

#include <stdbool.h>

#include "../../common/src/repetition_tester.h"
#include "../../common/src/buffer.h"


typedef struct read_parameters read_parameters;

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


static char const *DescribeAllocationType(allocation_type allocType);
static void HandleAllocation(read_parameters *params, buffer *buff);
static void HandleDeallocation(read_parameters *params, buffer *buff);

static uint64_t GetMaxOSRandomCount();
static bool ReadOSRandomBytes(uint64_t Count, void *Dest);
static void FillWithRandomBytes(buffer dest);

#endif // TESTER_COMMON_H
