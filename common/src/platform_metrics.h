/* TODO (Aaron):
*/

#ifndef PLATFORM_METRICS_H
#define PLATFORM_METRICS_H

#include "base.h"
#include "base_types.h"


// Note (Aaron): Increase this count as needed
#define MAX_NAMED_TIMINGS 64

// Note (Aaron): Defines number of milliseconds used to determine the CPU frequency.
// A higher number is more accurate.
#define CPU_FREQUENCY_MS 200

// Note (Aaron): Enable this to check for timings that have been started but not ended or vice versa
#define DETECT_ORPHAN_TIMINGS 0


typedef struct timings_profile timings_profile;
typedef struct zone_timing zone_timing;
typedef struct zone_block zone_block;

global_function void StartTimingsProfile();
global_function void EndTimingsProfile();
global_function void PrintTimingsProfile();
global_function void _StartTiming(zone_block *block, U32 timingIndex, char const *label);
global_function void _EndTiming(zone_block *block);

global_function U64 GetOSTimerFrequency();
global_function U64 ReadOSTimer();
global_function U64 ReadCPUTimer();
global_function U64 GetCPUFrequency(U64 millisecondsToWait);


struct zone_timing
{
    U64 TSCElapsed;
    U64 TSCElapsedChildren;
    U64 TSCElapsedOriginal;
    U64 HitCount;
    char const *Label;

#if DETECT_ORPHAN_TIMINGS
    U64 EndCount;
#endif // DETECT_ORPHAN_TIMINGS
};


struct zone_block
{
    U32 ParentIndex;
    U32 Index;
    U64 TSCElapsedOriginal;
    U64 Start;
    char const *Label;

#if __cplusplus
    B8 AutoExecute;

    zone_block(U32 index, char const *label, B8 autoExecute)
    {
        AutoExecute = autoExecute;
        if (AutoExecute) _StartTiming(this, index, label);
    }

    ~zone_block(void)
    {
        if (AutoExecute) _EndTiming(this);
    }
#endif // __cplusplus
};


struct timings_profile
{
    U64 Start;
    U64 TSCElapsed;
    U64 CPUFrequency;
    zone_timing Timings[MAX_NAMED_TIMINGS];

#if DETECT_ORPHAN_TIMINGS
    B8 Started;
    B8 Ended;
#endif // DETECT_ORPHAN_TIMINGS
};


#ifndef __cplusplus //////////////////////////////////////////////////////////////////////////
// Note (Aaron): Use the following macros to start and end named timings within the same scope.
#define START_TIMING(label)     zone_block label##Block = {0};                       \
                                _StartTiming(&label##Block, __COUNTER__ + 1, #label);

#define END_TIMING(label)       _EndTiming(&label##Block);


// Note (Aaron): The following macros can be used to control the scope a timing is created in
// so it can be re-used later in the same scope.
#define PREWARM_TIMING(label)   zone_block label##Block = {0}; \
                                _PreWarmTiming(&label##Block, __COUNTER__ + 1, #label)

#define RESTART_TIMING(label)   _RestartTiming(&label##Block);

#else ////////////////////////////////////////////////////////////////////////////////////////
#define FUNCTION_TIMING         zone_block __func__##Block (__COUNTER__ + 1, __func__, TRUE);
#define ZONE_TIMING(label)      zone_block label##Block (__COUNTER__ + 1, #label, TRUE);

#define START_TIMING(label)     zone_block label##Block (__COUNTER__ + 1, #label, FALSE); \
                                _StartTiming(&label##Block, __COUNTER__ + 1, #label);

#define END_TIMING(label)       _EndTiming(&label##Block);

#define PREWARM_TIMING(label)   zone_block label##Block (__COUNTER__ + 1, #label, FALSE); \
                                _PreWarmTiming(&label##Block, __COUNTER__ + 1, #label);
#define RESTART_TIMING(label)   _RestartTiming(&label##Block);
#endif // __cplusplus ////////////////////////////////////////////////////////////////////////

#endif // PLATFORM_METRICS_H
