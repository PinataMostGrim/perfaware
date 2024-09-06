#ifndef TESTER_COMMON_H
#define TESTER_COMMON_H

#include <stdio.h>
#include <stdbool.h>

#include "../../common/src/repetition_tester.h"
#include "../../common/src/buffer.h"


typedef enum
{
    AllocType_none,
    AllocType_malloc,

    AllocType_Count,
} allocation_type;


typedef struct read_parameters read_parameters;
struct read_parameters
{
    char const *FileName;
    buffer Buffer;
    allocation_type AllocType;
};

typedef struct test_series_label test_series_label;
struct test_series_label
{
    char Chars[64];
};

typedef struct test_series test_series;
struct test_series
{
    buffer Memory;

    uint32_t MaxRowCount;
    uint32_t ColumnCount;

    uint32_t RowIndex;
    uint32_t ColumnIndex;

    test_results *TestResults; // [RowCount][ColumnCount]
    test_series_label *RowLabels; // [RowCount]
    test_series_label *ColumnLabels; // [ColumnCount]

    test_series_label RowLabelLabel;
};

static test_series TestSeriesAllocate(uint32_t columnCount, uint32_t maxRowCount);
static uint32_t TestSeriesIsValid(test_series series);
static void TestSeriesFree(test_series *series);
static bool TestSeriesIsInBounds(test_series *series);

static void SetColumnLabel(test_series *series, char const *Format, ...);
static void SetRowLabel(test_series *series, char const *Format, ...);
static void SetRowLabelLabel(test_series *series, char const *format, ...);

static void TestSeriesNewTestWave(test_series *series, repetition_tester *tester, uint64_t targetProcessedByteCount, uint64_t cpuTimerFrequency, uint32_t secondsToTry);
static uint32_t TestSeriesIsTesting(test_series *series, repetition_tester *tester);
static test_results *GetTestResults(test_series *series, uint32_t columnIndex, uint32_t rowIndex);
static void PrintCSVForValue(test_series *series, measurement_types measurementType, FILE *dest, double coefficient);

static char const *DescribeAllocationType(allocation_type allocType);
static void HandleAllocation(read_parameters *params, buffer *buff);
static void HandleDeallocation(read_parameters *params, buffer *buff);

static uint64_t GetMaxOSRandomCount();
static bool ReadOSRandomBytes(uint64_t Count, void *Dest);
static void FillWithRandomBytes(buffer dest);

static uint64_t TC__GetFileSize(char *fileName);
static buffer ReadEntireFile(char *fileName);

#endif // TESTER_COMMON_H
