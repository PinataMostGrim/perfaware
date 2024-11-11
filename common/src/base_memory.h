#ifndef BASE_MEMORY_H
#define BASE_MEMORY_H

#include "base.h"
#include "base_types.h"


// +------------------------------+
// Note (Aaron): Memory modification

global_function void *MemorySet(void *destPtr, int c, size_t count);
global_function void *MemoryCopy(void *destPtr, void const *sourcePtr, size_t size);

#define MemoryZero(ptr, count)  MemorySet((ptr), 0, (count))
#define MemoryZeroStruct(ptr)   MemoryZero((ptr), sizeof(*(ptr)))
#define MemoryZeroArray(array)  MemoryZero((array), sizeof(array))

#define MemoryCopyStruct(destPtr, sourcePtr)    MemoryCopy((destPtr), (sourcePtr), Min(sizeof(*(destPtr)), sizeof(*(sourcePtr))))
#define MemoryCopyArray(destArray, sourceArray) MemoryCopy((destArray), (sourceArray), Min(sizeof(sourceArray), sizeof(destArray)))

#endif // BASE_MEMORY_H
