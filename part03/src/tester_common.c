#include <stdio.h>

#include "tester_common.h"
#include "../../common/src/buffer.h"


static char const *DescribeAllocationType(allocation_type allocType)
{
    char const *result;
    switch(allocType)
    {
        case AllocType_none: { result = ""; break; }
        case AllocType_malloc: { result = "malloc"; break; }
        default: { result = "UNKNOWN"; break; }
    }

    return result;
}


static void HandleAllocation(read_parameters *params, buffer *buff)
{
    switch(params->AllocType)
    {
        case AllocType_none:
        {
            break;
        }
        case AllocType_malloc:
        {
            *buff = BufferAllocate(params->Buffer.SizeBytes);
            break;
        }

        default:
        {
            fprintf(stderr, "[ERROR] Unrecognized allocation type");
            break;
        }
    }
}


static void HandleDeallocation(read_parameters *params, buffer *buff)
{
    switch(params->AllocType)
    {
        case AllocType_none:
        {
            break;
        }
        case AllocType_malloc:
        {
            BufferFree(buff);
            break;
        }

        default:
        {
            fprintf(stderr, "[ERROR] Unrecognized allocation type");
            break;
        }
    }
}
