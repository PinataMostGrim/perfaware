#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>
#include <stdint.h>

typedef struct buffer buffer;
struct buffer
{
    size_t SizeBytes;
    uint8_t *Data;
};

static buffer BufferAllocate(size_t sizeBytes);
static void BufferFree(buffer *buff);

#endif // BUFFER_H
