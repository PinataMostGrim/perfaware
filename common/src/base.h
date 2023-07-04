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

#define Stringify_(S) #S
#define Stringify(S) Stringify_(S)
#define Glue_(A, B) A##B
#define Glue(A, B) Glue_(A, B)

// Note (Aaron): These may not be compatible with all compilers, but implementation can be switched based on context
#define IntFromPtr(p) (unsigned long long)((char *)p - (char *)0)
#define PtrFromInt(n) (void *)((char *)0 + (n))

// Note (Aaron): Can't read or write from a member using this macro, but can abstractly represent the member.
#define Member(T, m) (((T*)0)->m)
#define OffsetOfMember(T, m) IntFromPtr(&Member(T,m))


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
