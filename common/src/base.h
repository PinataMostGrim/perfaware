/* Note (Aaron):
Based heavily on Allen Webster's base layer.
    source: https://www.youtube.com/watch?v=8fJ4vWrkS4o&list=PLT6InxK-XQvNKTyLXk6H6KKy12UYS_KDL

Base layer conforms to the following principles:
- No OS dependencies
- No external dependencies
- Minimal testing
*/

#ifndef BASE_H
#define BASE_H

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


// +------------------------------+
// Note (Aaron): Linked list macros

#define SLLQueuePush_N(f,l,n,next)  ((f)==0?\
                                        (f)=(l)=(n):\
                                        ((l)->next=(n),(l)=(n)),\
                                        (n)->next=0)

#define SLLQueuePush(f, l, n) SLLQueuePush_N(f, l, n, Next)


// +------------------------------+
// Note (Aaron): Hacker's Delight

#define dozs(x, y) ((x) - (y)) & -((x) >= (y))
#define maxs(x, y) (y) + (dozs((x), (y)))
#define mins(x, y) (x) - (dozs((x), (y)))

#endif // BASE_H
