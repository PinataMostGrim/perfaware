#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>

#include "tester_common.h"
#include "../../common/src/repetition_tester.h"
#include "../../common/src/buffer.h"


static test_series TestSeriesAllocate(uint32_t columnCount, uint32_t maxRowCount)
{
    test_series series = {0};

    uint64_t testResultsSize = (columnCount * maxRowCount) * sizeof(test_results);
    uint64_t rowLabelsSize = (maxRowCount) * sizeof(test_series_label);
    uint64_t columnLabelsSize = (columnCount) * sizeof(test_series_label);

    series.Memory = BufferAllocate(testResultsSize + rowLabelsSize + columnLabelsSize);
    if (BufferIsValid(series.Memory))
    {
        series.MaxRowCount = maxRowCount;
        series.ColumnCount = columnCount;

        series.TestResults = (test_results *)series.Memory.Data;
        series.RowLabels = (test_series_label *)((uint8_t *)series.TestResults + testResultsSize);
        series.ColumnLabels = (test_series_label *)((uint8_t *)series.RowLabels + rowLabelsSize);
    }

    return series;
}


static uint32_t TestSeriesIsValid(test_series series)
{
    uint32_t result = BufferIsValid(series.Memory);
    return result;
}


static void TestSeriesFree(test_series *series)
{
    if (series)
    {
        BufferFree(&series->Memory);

        size_t count = sizeof(test_series);
        unsigned char *dest = (unsigned char *)series;
        while(count--) *dest++ = (unsigned char)0;
    }
}


static bool TestSeriesIsInBounds(test_series *series)
{
    bool result = ((series->ColumnIndex < series->ColumnCount)
                   && (series->RowIndex < series->MaxRowCount));
    return result;
}


static void SetColumnLabel(test_series *series, char const *format, ...)
{
    if(TestSeriesIsInBounds(series))
    {
        test_series_label *label = series->ColumnLabels + series->ColumnIndex;
        va_list args;
        va_start(args, format);
        vsnprintf(label->Chars, sizeof(label->Chars), format, args);
        va_end(args);
    }
}


static void SetRowLabel(test_series *series, char const *format, ...)
{
    if(TestSeriesIsInBounds(series))
    {
        test_series_label *label = series->RowLabels + series->RowIndex;
        va_list args;
        va_start(args, format);
        vsnprintf(label->Chars, sizeof(label->Chars), format, args);
        va_end(args);
    }
}


static void SetRowLabelLabel(test_series *series, char const *format, ...)
{
    test_series_label *label = &series->RowLabelLabel;
    va_list args;
    va_start(args, format);
    vsnprintf(label->Chars, sizeof(label->Chars), format, args);
    va_end(args);
}


static void TestSeriesNewTestWave(
    test_series *series,
    repetition_tester *tester, uint64_t targetProcessedByteCount, uint64_t cpuTimerFrequency, uint32_t secondsToTry)
{
    if(TestSeriesIsInBounds(series))
    {
        printf("\n--- %s %s ---\n",
            series->ColumnLabels[series->ColumnIndex].Chars,
            series->RowLabels[series->RowIndex].Chars);
    }

    NewTestWave(tester, targetProcessedByteCount, cpuTimerFrequency, secondsToTry);
}


static uint32_t TestSeriesIsTesting(test_series *series, repetition_tester *tester)
{
    uint32_t result = IsTesting(tester);

    if (!result)
    {
        if (TestSeriesIsInBounds(series))
        {
            *GetTestResults(series, series->ColumnIndex, series->RowIndex) = tester->Results;

            if(++series->ColumnIndex >= series->ColumnCount)
            {
                series->ColumnIndex = 0;
                ++series->RowIndex;
            }
        }
    }

    return result;
}


static test_results *GetTestResults(test_series *series, uint32_t columnIndex, uint32_t rowIndex)
{
    test_results *result = 0;
    if ((columnIndex < series->ColumnCount) && (rowIndex < series->MaxRowCount))
    {
        result = series->TestResults + (rowIndex * series->ColumnCount + columnIndex);
    }

    return result;
}


static void PrintCSVForValue(test_series *series, measurement_types measurementType, FILE *dest, double coefficient)
{
    fprintf(dest, "%s", series->RowLabelLabel.Chars);
    for(uint32_t columnIndex = 0; columnIndex < series->ColumnCount; ++columnIndex)
    {
        fprintf(dest, ",%s", series->ColumnLabels[columnIndex].Chars);
    }
    fprintf(dest, "\n");

    for(uint32_t rowIndex = 0; rowIndex < series->RowIndex; ++rowIndex)
    {
        fprintf(dest, "%s", series->RowLabels[rowIndex].Chars);
        for(uint32_t columnIndex = 0; columnIndex < series->ColumnCount; ++columnIndex)
        {
            test_results *testResults = GetTestResults(series, columnIndex, rowIndex);
            fprintf(dest, ",%f", coefficient * testResults->Min.DerivedValues[measurementType]);
        }
        fprintf(dest, "\n");
    }
}


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
static bool ReadOSRandomBytes(uint64_t Count, void *Dest) { assert(0 && "Not implemented"); }
static uint64_t GetFileSize(char *fileName) { assert(0 && "Not implemented"); }
static buffer ReadEntireFile(char *fileName) { assert(0 && "Not implemented"); }

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

static uint64_t GetFileSize(char *fileName)
{
    struct stat stats;
    stat(fileName, &stats);

    return stats.st_size;
}

static buffer ReadEntireFile(char *fileName)
{
    buffer result = {0};

    FILE *file = fopen(fileName, "rb");
    if (!file)
    {
        fprintf(stderr, "[ERROR]: Unable to open \"%s\".\\n", fileName);
        return result;
    }

    uint64_t fileSize = GetFileSize(fileName);
    result = BufferAllocate(fileSize);
    if (result.Data)
    {
        if (fread(result.Data, result.SizeBytes, 1, file) != 1)
        {
            fprintf(stderr, "[ERROR]: Unable to read \"%s\".\n", fileName);
            BufferFree(&result);
        }
    }
    fclose(file);

    return result;
}

#endif
