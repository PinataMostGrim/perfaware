#include <float.h>
#include <math.h>
#include <inttypes.h>
#include <stdio.h>
#include <sys/stat.h>

#include "base_inc.h"
#include "base_types.c"


// Note (Aaron): Include for reference values from libbf
// #include "reference_values_libbf.h"

#define STATIC_ASSERT(condition, message) \
    typedef char static_assertion_##message[(condition) ? 1 : -1]

#define ARRAY_PARAM(type, array) ((type) \
{ \
    .Size = (sizeof(array) / sizeof((array)[0])), \
    .Data = (array) \
})


typedef struct f64_array f64_array;
struct f64_array
{
    size_t Size;
    F64 *Data;
};

typedef struct range range;
struct range
{
    F64 Min;
    F64 Max;
};

typedef struct validation_set validation_set;
struct validation_set
{
    F64 Input;
    F64 GroundTruth;

    F64 ReferenceOutput;
    F64 CustomOutput;

    F64 ReferenceError;
    F64 CustomError;
};

typedef struct error error;
struct error
{
    U64 SampleCount;

    range Reference;
    range Custom;

    F64 ReferenceAbs;
    F64 CustomAbs;
};


global_variable F64 Reference_SinInput[] = {-3.14159, -3.09251, -3.04342, -2.99433, -2.94524, -2.89616, -2.84707, -2.79798, -2.74889, -2.69981, -2.65072, -2.60163, -2.55254, -2.50346, -2.45437, -2.40528, -2.35619, -2.30711, -2.25802, -2.20893, -2.15984, -2.11076, -2.06167, -2.01258, -1.9635, -1.91441, -1.86532, -1.81623, -1.76715, -1.71806, -1.66897, -1.61988, -1.5708, -1.52171, -1.47262, -1.42353, -1.37445, -1.32536, -1.27627, -1.22718, -1.1781, -1.12901, -1.07992, -1.03084, -0.981748, -0.93266, -0.883573, -0.834486, -0.785398, -0.736311, -0.687223, -0.638136, -0.589049, -0.539961, -0.490874, -0.441786, -0.392699, -0.343612, -0.294524, -0.245437, -0.19635, -0.147262, -0.0981748, -0.0490874, 0, 0.0490874, 0.0981748, 0.147262, 0.19635, 0.245437, 0.294524, 0.343612, 0.392699, 0.441786, 0.490874, 0.539961, 0.589049, 0.638136, 0.687223, 0.736311, 0.785398, 0.834486, 0.883573, 0.93266, 0.981748, 1.03084, 1.07992, 1.12901, 1.1781, 1.22718, 1.27627, 1.32536, 1.37445, 1.42353, 1.47262, 1.52171, 1.5708, 1.61988, 1.66897, 1.71806, 1.76715, 1.81623, 1.86532, 1.91441, 1.9635, 2.01258, 2.06167, 2.11076, 2.15984, 2.20893, 2.25802, 2.30711, 2.35619, 2.40528, 2.45437, 2.50346, 2.55254, 2.60163, 2.65072, 2.69981, 2.74889, 2.79798, 2.84707, 2.89616, 2.94524, 2.99433, 3.04342, 3.09251, 3.14159};

global_variable F64 Reference_SinOutput[] = {-1.22465e-16, -0.0490677, -0.0980171, -0.14673, -0.19509, -0.24298, -0.290285, -0.33689, -0.382683, -0.427555, -0.471397, -0.514103, -0.55557, -0.595699, -0.634393, -0.671559, -0.707107, -0.740951, -0.77301, -0.803208, -0.83147, -0.857729, -0.881921, -0.903989, -0.92388, -0.941544, -0.95694, -0.970031, -0.980785, -0.989177, -0.995185, -0.998795, -1, -0.998795, -0.995185, -0.989177, -0.980785, -0.970031, -0.95694, -0.941544, -0.92388, -0.903989, -0.881921, -0.857729, -0.83147, -0.803208, -0.77301, -0.740951, -0.707107, -0.671559, -0.634393, -0.595699, -0.55557, -0.514103, -0.471397, -0.427555, -0.382683, -0.33689, -0.290285, -0.24298, -0.19509, -0.14673, -0.0980171, -0.0490677, 0, 0.0490677, 0.0980171, 0.14673, 0.19509, 0.24298, 0.290285, 0.33689, 0.382683, 0.427555, 0.471397, 0.514103, 0.55557, 0.595699, 0.634393, 0.671559, 0.707107, 0.740951, 0.77301, 0.803208, 0.83147, 0.857729, 0.881921, 0.903989, 0.92388, 0.941544, 0.95694, 0.970031, 0.980785, 0.989177, 0.995185, 0.998795, 1, 0.998795, 0.995185, 0.989177, 0.980785, 0.970031, 0.95694, 0.941544, 0.92388, 0.903989, 0.881921, 0.857729, 0.83147, 0.803208, 0.77301, 0.740951, 0.707107, 0.671559, 0.634393, 0.595699, 0.55557, 0.514103, 0.471397, 0.427555, 0.382683, 0.33689, 0.290285, 0.24298, 0.19509, 0.14673, 0.0980171, 0.0490677, 1.22465e-16};

global_variable F64 Reference_CosInput[] = {-1.5708, -1.54625, -1.52171, -1.49717, -1.47262, -1.44808, -1.42353, -1.39899, -1.37445, -1.3499, -1.32536, -1.30082, -1.27627, -1.25173, -1.22718, -1.20264, -1.1781, -1.15355, -1.12901, -1.10447, -1.07992, -1.05538, -1.03084, -1.00629, -0.981748, -0.957204, -0.93266, -0.908117, -0.883573, -0.859029, -0.834486, -0.809942, -0.785398, -0.760854, -0.736311, -0.711767, -0.687223, -0.66268, -0.638136, -0.613592, -0.589049, -0.564505, -0.539961, -0.515418, -0.490874, -0.46633, -0.441786, -0.417243, -0.392699, -0.368155, -0.343612, -0.319068, -0.294524, -0.269981, -0.245437, -0.220893, -0.19635, -0.171806, -0.147262, -0.122718, -0.0981748, -0.0736311, -0.0490874, -0.0245437, 0, 0.0245437, 0.0490874, 0.0736311, 0.0981748, 0.122718, 0.147262, 0.171806, 0.19635, 0.220893, 0.245437, 0.269981, 0.294524, 0.319068, 0.343612, 0.368155, 0.392699, 0.417243, 0.441786, 0.46633, 0.490874, 0.515418, 0.539961, 0.564505, 0.589049, 0.613592, 0.638136, 0.66268, 0.687223, 0.711767, 0.736311, 0.760854, 0.785398, 0.809942, 0.834486, 0.859029, 0.883573, 0.908117, 0.93266, 0.957204, 0.981748, 1.00629, 1.03084, 1.05538, 1.07992, 1.10447, 1.12901, 1.15355, 1.1781, 1.20264, 1.22718, 1.25173, 1.27627, 1.30082, 1.32536, 1.3499, 1.37445, 1.39899, 1.42353, 1.44808, 1.47262, 1.49717, 1.52171, 1.54625, 1.5708};

global_variable F64 Reference_CosOutput[] = {6.12323e-17, 0.0245412, 0.0490677, 0.0735646, 0.0980171, 0.122411, 0.14673, 0.170962, 0.19509, 0.219101, 0.24298, 0.266713, 0.290285, 0.313682, 0.33689, 0.359895, 0.382683, 0.405241, 0.427555, 0.449611, 0.471397, 0.492898, 0.514103, 0.534998, 0.55557, 0.575808, 0.595699, 0.615232, 0.634393, 0.653173, 0.671559, 0.689541, 0.707107, 0.724247, 0.740951, 0.757209, 0.77301, 0.788346, 0.803208, 0.817585, 0.83147, 0.844854, 0.857729, 0.870087, 0.881921, 0.893224, 0.903989, 0.91421, 0.92388, 0.932993, 0.941544, 0.949528, 0.95694, 0.963776, 0.970031, 0.975702, 0.980785, 0.985278, 0.989177, 0.99248, 0.995185, 0.99729, 0.998795, 0.999699, 1, 0.999699, 0.998795, 0.99729, 0.995185, 0.99248, 0.989177, 0.985278, 0.980785, 0.975702, 0.970031, 0.963776, 0.95694, 0.949528, 0.941544, 0.932993, 0.92388, 0.91421, 0.903989, 0.893224, 0.881921, 0.870087, 0.857729, 0.844854, 0.83147, 0.817585, 0.803208, 0.788346, 0.77301, 0.757209, 0.740951, 0.724247, 0.707107, 0.689541, 0.671559, 0.653173, 0.634393, 0.615232, 0.595699, 0.575808, 0.55557, 0.534998, 0.514103, 0.492898, 0.471397, 0.449611, 0.427555, 0.405241, 0.382683, 0.359895, 0.33689, 0.313682, 0.290285, 0.266713, 0.24298, 0.219101, 0.19509, 0.170962, 0.14673, 0.122411, 0.0980171, 0.0735646, 0.0490677, 0.0245412, 6.12323e-17};

global_variable F64 Reference_ArcSinInput[] = {0, 0.0122718, 0.0245437, 0.0368155, 0.0490874, 0.0613592, 0.0736311, 0.0859029, 0.0981748, 0.110447, 0.122718, 0.13499, 0.147262, 0.159534, 0.171806, 0.184078, 0.19635, 0.208621, 0.220893, 0.233165, 0.245437, 0.257709, 0.269981, 0.282252, 0.294524, 0.306796, 0.319068, 0.33134, 0.343612, 0.355884, 0.368155, 0.380427, 0.392699, 0.404971, 0.417243, 0.429515, 0.441786, 0.454058, 0.46633, 0.478602, 0.490874, 0.503146, 0.515418, 0.527689, 0.539961, 0.552233, 0.564505, 0.576777, 0.589049, 0.60132, 0.613592, 0.625864, 0.638136, 0.650408, 0.66268, 0.674952, 0.687223, 0.699495, 0.711767, 0.724039, 0.736311, 0.748583, 0.760854, 0.773126, 0.785398, 0.79767, 0.809942, 0.822214, 0.834486, 0.846757, 0.859029, 0.871301, 0.883573, 0.895845, 0.908117, 0.920388, 0.93266, 0.944932, 0.957204, 0.969476, 0.981748, 0.99402};

global_variable F64 Reference_ArcSinOutput[] = {0, 0.0122722, 0.0245462, 0.0368239, 0.0491071, 0.0613978, 0.0736978, 0.0860089, 0.0983332, 0.110672, 0.123029, 0.135404, 0.1478, 0.160219, 0.172662, 0.185133, 0.197634, 0.210165, 0.22273, 0.235331, 0.24797, 0.26065, 0.273373, 0.286141, 0.298958, 0.311825, 0.324746, 0.337723, 0.35076, 0.363859, 0.377024, 0.390258, 0.403565, 0.416947, 0.430409, 0.443955, 0.457589, 0.471315, 0.485138, 0.499062, 0.513092, 0.527235, 0.541495, 0.555878, 0.570391, 0.58504, 0.599833, 0.614778, 0.629881, 0.645153, 0.660602, 0.676239, 0.692075, 0.708121, 0.724391, 0.740899, 0.75766, 0.774691, 0.792011, 0.80964, 0.827602, 0.845922, 0.864629, 0.883756, 0.903339, 0.923422, 0.944053, 0.965289, 0.987199, 1.00986, 1.03337, 1.05785, 1.08344, 1.11033, 1.13876, 1.16907, 1.20172, 1.23739, 1.27718, 1.32308, 1.37944, 1.46138};

global_variable F64 Reference_SqrtInput[] = {0, 0.0122718, 0.0245437, 0.0368155, 0.0490874, 0.0613592, 0.0736311, 0.0859029, 0.0981748, 0.110447, 0.122718, 0.13499, 0.147262, 0.159534, 0.171806, 0.184078, 0.19635, 0.208621, 0.220893, 0.233165, 0.245437, 0.257709, 0.269981, 0.282252, 0.294524, 0.306796, 0.319068, 0.33134, 0.343612, 0.355884, 0.368155, 0.380427, 0.392699, 0.404971, 0.417243, 0.429515, 0.441786, 0.454058, 0.46633, 0.478602, 0.490874, 0.503146, 0.515418, 0.527689, 0.539961, 0.552233, 0.564505, 0.576777, 0.589049, 0.60132, 0.613592, 0.625864, 0.638136, 0.650408, 0.66268, 0.674952, 0.687223, 0.699495, 0.711767, 0.724039, 0.736311, 0.748583, 0.760854, 0.773126, 0.785398, 0.79767, 0.809942, 0.822214, 0.834486, 0.846757, 0.859029, 0.871301, 0.883573, 0.895845, 0.908117, 0.920388, 0.93266, 0.944932, 0.957204, 0.969476, 0.981748, 0.99402};

global_variable F64 Reference_SqrtOutput[] = {0, 0.110778, 0.156664, 0.191874, 0.221557, 0.247708, 0.27135, 0.293092, 0.313329, 0.332335, 0.350312, 0.36741, 0.383748, 0.399417, 0.414495, 0.429043, 0.443113, 0.456751, 0.469993, 0.482872, 0.495416, 0.50765, 0.519597, 0.531274, 0.542701, 0.553892, 0.564861, 0.575621, 0.586184, 0.59656, 0.606758, 0.616788, 0.626657, 0.636373, 0.645943, 0.655374, 0.66467, 0.673838, 0.682884, 0.691811, 0.700624, 0.709328, 0.717926, 0.726422, 0.734821, 0.743124, 0.751335, 0.759458, 0.767495, 0.775449, 0.783321, 0.791116, 0.798834, 0.806479, 0.814051, 0.821554, 0.828989, 0.836358, 0.843663, 0.850905, 0.858086, 0.865207, 0.87227, 0.879276, 0.886227, 0.893124, 0.899968, 0.90676, 0.913502, 0.920194, 0.926838, 0.933435, 0.939986, 0.946491, 0.952952, 0.959369, 0.965743, 0.972076, 0.978368, 0.98462, 0.990832, 0.997005};

STATIC_ASSERT(sizeof(Reference_SinInput) == sizeof(Reference_SinOutput), array_lengths_must_be_equal);
STATIC_ASSERT(sizeof(Reference_CosInput) == sizeof(Reference_CosOutput), array_lengths_must_be_equal);
STATIC_ASSERT(sizeof(Reference_ArcSinInput) == sizeof(Reference_ArcSinOutput), array_lengths_must_be_equal);
STATIC_ASSERT(sizeof(Reference_SqrtInput) == sizeof(Reference_SqrtOutput), array_lengths_must_be_equal);


global_function void PrintValidationSet(validation_set set)
{
    fprintf(stdout, "input: %f\n", set.Input);
    fprintf(stdout, "ground truth: %f\n", set.GroundTruth);

    fprintf(stdout, "reference output: %f\n", set.ReferenceOutput);
    fprintf(stdout, "custom output: %f\n", set.CustomOutput);

    fprintf(stdout, "reference error: %.15f\n", set.ReferenceError);
    fprintf(stdout, "custom error: %.15f\n", set.CustomError);
}


global_function void PrintError(error error, char *label)
{
    fprintf(stdout, "--- %s ---\n", label);

    fprintf(stdout, "reference error: %.15f\n", error.ReferenceAbs);
    fprintf(stdout, "custom error: %.15f\n", error.CustomAbs);
    fprintf(stdout, "sample count: %" PRIu64"\n", error.SampleCount);

    // fprintf(stdout, "reference min: %.15f\n", error.Reference.Min);
    // fprintf(stdout, "reference max: %.15f\n", error.Reference.Max);
    // fprintf(stdout, "custom min: %.15f\n", error.Custom.Min);
    // fprintf(stdout, "custom max: %.15f\n", error.Custom.Max);
}


global_function F64 CustomSin(F64 input)
{
    return 0;
}


global_function F64 CustomCos(F64 input)
{
    return 0;
}


global_function F64 CustomArcSin(F64 input)
{
    return 0;
}


global_function F64 CustomSqrt(F64 input)
{
    return 0;
}


global_function void RangeInclude(range *range, F64 value)
{
    if (value < range->Min) { range->Min = value; }
    if (value > range->Max) { range->Max = value; }
}


global_function void ErrorCalculate(error *error)
{
    F64 refMinAbs = error->Reference.Min < 0 ? error->Reference.Min * -1 : error->Reference.Min;
    F64 refMaxAbs = error->Reference.Max < 0 ? error->Reference.Max * -1 : error->Reference.Max;

    F64 customMinAbs = error->Custom.Min < 0 ? error->Custom.Min * -1 : error->Custom.Min;
    F64 customMaxAbs = error->Custom.Max < 0 ? error->Custom.Max * -1 : error->Custom.Max;

    error->ReferenceAbs = refMaxAbs > refMinAbs ? refMaxAbs : refMinAbs;
    error->CustomAbs = customMaxAbs > customMinAbs ? customMaxAbs : customMinAbs;
}


global_function error MeasureFunctionError(f64_array inputValues, f64_array groundTruthValues, F64 (*referenceFunc)(F64), F64 (*customFunc)(F64))
{
    range invertedInfinite = {DBL_MAX, -DBL_MAX};

    error result = {0};
    result.Reference = invertedInfinite;
    result.Custom = invertedInfinite;

    if (inputValues.Size == groundTruthValues.Size)
    {
        if (referenceFunc && customFunc)
        {
            validation_set set = {0};
            for (int i = 0; i < inputValues.Size; ++i)
            {
                set.Input = inputValues.Data[i];
                set.GroundTruth = groundTruthValues.Data[i];

                set.ReferenceOutput = referenceFunc(set.Input);
                set.CustomOutput = customFunc(set.Input);

                set.ReferenceError = set.GroundTruth - set.ReferenceOutput;
                set.CustomError = set.GroundTruth - set.CustomOutput;

                // PrintValidationSet(set);
                // fprintf(stdout, "\n");

                result.SampleCount++;
                RangeInclude(&result.Reference, set.ReferenceError);
                RangeInclude(&result.Custom, set.CustomError);
            }
        }
        else
        {
            fprintf(stderr, "[ERROR] Invalid reference or custom function\n");
        }
    }
    else
    {
        fprintf(stderr, "[ERROR] Input value array and ground truth value array sizes do not match\n");
    }

    ErrorCalculate(&result);

    return result;
}


int main(int argc, char const *argv[])
{
    fprintf(stdout, "Function error from ground truth\n");

    f64_array sinInput = ARRAY_PARAM(f64_array, Reference_SinInput);
    f64_array sinOutput = ARRAY_PARAM(f64_array, Reference_SinOutput);
    error errorsSin = MeasureFunctionError(sinInput, sinOutput, sin, CustomSin);
    PrintError(errorsSin, "Sin");
    fprintf(stdout, "\n");

    f64_array cosInput = ARRAY_PARAM(f64_array, Reference_CosInput);
    f64_array cosOutput = ARRAY_PARAM(f64_array, Reference_CosOutput);
    error errorsCos = MeasureFunctionError(cosInput, cosOutput, cos, CustomCos);
    PrintError(errorsCos, "Cos");
    fprintf(stdout, "\n");

    f64_array arcSinInput = ARRAY_PARAM(f64_array, Reference_ArcSinInput);
    f64_array arcSinOutput = ARRAY_PARAM(f64_array, Reference_ArcSinOutput);
    error errorsArcSin = MeasureFunctionError(arcSinInput, arcSinOutput, asin, CustomArcSin);
    PrintError(errorsArcSin, "ArcSin");
    fprintf(stdout, "\n");

    f64_array sqrtInput = ARRAY_PARAM(f64_array, Reference_SqrtInput);
    f64_array sqrtOutput = ARRAY_PARAM(f64_array, Reference_SqrtOutput);
    error errorsSqrt = MeasureFunctionError(sqrtInput, sqrtOutput, sqrt, CustomSqrt);
    PrintError(errorsSqrt, "Sqrt");
    fprintf(stdout, "\n");

    return 0;
}
