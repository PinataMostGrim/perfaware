#include <stdio.h>
#include <stdbool.h>

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


static void FillWithRandomBytes(buffer dest)
{
    uint64_t maxRandCount = GetMaxOSRandomCount();
    uint64_t atOffset = 0;
    while (atOffset < dest.SizeBytes)
    {
        uint64_t readCount = dest.SizeBytes - atOffset;
        if (readCount > maxRandCount)
        {
            readCount = maxRandCount;
        }

        ReadOSRandomBytes(readCount, dest.Data + atOffset);
        atOffset += readCount;
    }
}


#if _WIN32

#include <assert.h>

static uint64_t GetMaxOSRandomCount() { assert(0 && "Not implemented"); }
static b32 ReadOSRandomBytes(uint64_t Count, void *Dest) { assert(0 && "Not implemented"); }

#else

#include <sys/random.h>
#include <sys/time.h>

static uint64_t GetMaxOSRandomCount()
{
    return SIZE_MAX;
}


static bool ReadOSRandomBytes(uint64_t count, void *dest)
{
    uint64_t bytesRead = 0;
    while (bytesRead < count)
    {
        bytesRead += getrandom(dest, count - bytesRead, 0);
        if (bytesRead < 0)
        {
            return false;
        }
        // TODO (Aaron): Print the error using errno
    }

    return true;
}

#endif
