/**
 * @file dsmil_model_infer_int8_runtime.c
 * @brief INT8 Model Inference Implementation
 * 
 * Implements dsmil_model_infer_int8() for executing inference on loaded
 * INT8 quantized models with full error handling and resource management.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_int8_model.h"
#include "dsmil_onnx_runtime_integration.h"
#include <onnxruntime_c_api.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <stdbool.h>

/**
 * @brief Model handle structure (matches dsmil_int8_model_load_runtime.c)
 */
struct dsmil_model_handle {
    OrtSession *session;
    OrtMemoryInfo *memory_info;
    char *model_path;
    uint8_t device_id;
    bool use_gpu;
    bool use_npu;
    size_t input_count;
    size_t output_count;
    char **input_names;
    char **output_names;
    int64_t **input_shapes;
    size_t *input_shape_ranks;
    int64_t **output_shapes;
    size_t *output_shape_ranks;
};

/**
 * @brief Forward declarations
 */
extern int dsmil_onnx_create_input_tensor(const OrtApi* ort,
                                          const float *data,
                                          size_t data_size,
                                          const int64_t *shape,
                                          size_t shape_len,
                                          OrtValue **tensor);
extern int dsmil_onnx_run_inference(const OrtApi* ort,
                                    OrtSession* session,
                                    const char* const* input_names,
                                    const OrtValue* const* input_tensors,
                                    size_t num_inputs,
                                    const char* const* output_names,
                                    OrtValue** output_tensors,
                                    size_t num_outputs);
extern int dsmil_onnx_extract_output(const OrtApi* ort,
                                     OrtValue* tensor,
                                     float *data,
                                     size_t data_size);
extern int dsmil_onnx_translate_error(OrtStatus* status);

/**
 * @brief Validate input/output sizes match model expectations
 * 
 * @param handle Model handle
 * @param input_size Provided input size
 * @param output_size Provided output size
 * @return 0 if valid, negative on error
 */
static int validate_io_sizes(dsmil_model_handle_t *handle,
                             size_t input_size,
                             size_t output_size) {
    if (!handle) {
        return -EINVAL;
    }
    
    // Check if we have shape information
    if (handle->input_count > 0 && handle->input_shapes && handle->input_shape_ranks) {
        // Calculate expected input size from shape
        size_t expected_input_size = 1;
        for (size_t i = 0; i < handle->input_shape_ranks[0]; i++) {
            if (handle->input_shapes[0][i] > 0) {
                expected_input_size *= (size_t)handle->input_shapes[0][i];
            }
        }
        
        // Allow batch dimension to be 1 (we always use batch size 1)
        if (handle->input_shape_ranks[0] > 1 && handle->input_shapes[0][0] == 1) {
            // Skip batch dimension
            expected_input_size = 1;
            for (size_t i = 1; i < handle->input_shape_ranks[0]; i++) {
                if (handle->input_shapes[0][i] > 0) {
                    expected_input_size *= (size_t)handle->input_shapes[0][i];
                }
            }
        }
        
        if (input_size != expected_input_size && expected_input_size > 0) {
            fprintf(stderr, "ERROR: Input size mismatch: expected %zu, got %zu\n",
                    expected_input_size, input_size);
            return -ERANGE;
        }
    }
    
    if (handle->output_count > 0 && handle->output_shapes && handle->output_shape_ranks) {
        // Calculate expected output size from shape
        size_t expected_output_size = 1;
        for (size_t i = 0; i < handle->output_shape_ranks[0]; i++) {
            if (handle->output_shapes[0][i] > 0) {
                expected_output_size *= (size_t)handle->output_shapes[0][i];
            }
        }
        
        // Allow batch dimension to be 1
        if (handle->output_shape_ranks[0] > 1 && handle->output_shapes[0][0] == 1) {
            // Skip batch dimension
            expected_output_size = 1;
            for (size_t i = 1; i < handle->output_shape_ranks[0]; i++) {
                if (handle->output_shapes[0][i] > 0) {
                    expected_output_size *= (size_t)handle->output_shapes[0][i];
                }
            }
        }
        
        if (output_size < expected_output_size && expected_output_size > 0) {
            fprintf(stderr, "ERROR: Output buffer too small: need %zu, got %zu\n",
                    expected_output_size, output_size);
            return -ERANGE;
        }
    }
    
    return 0;
}

/**
 * @brief Validate output data for NaN and infinities
 * 
 * @param output Output data
 * @param output_size Number of elements
 * @return 0 if valid, negative on error
 */
static int validate_output_data(const float *output, size_t output_size) {
    if (!output) {
        return -EINVAL;
    }
    
    for (size_t i = 0; i < output_size; i++) {
        if (isnan(output[i]) || isinf(output[i])) {
            fprintf(stderr, "ERROR: Invalid output value at index %zu: %f\n", i, output[i]);
            return -EIO;
        }
    }
    
    return 0;
}

/**
 * @brief Run INT8 model inference
 */
int dsmil_model_infer_int8(dsmil_model_handle_t *model_handle,
                           const float *input,
                           size_t input_size,
                           float *output,
                           size_t output_size) {
    if (!model_handle || !input || !output) {
        fprintf(stderr, "ERROR: Invalid parameters to dsmil_model_infer_int8\n");
        return -EINVAL;
    }
    
    if (input_size == 0 || output_size == 0) {
        fprintf(stderr, "ERROR: Input or output size is zero\n");
        return -EINVAL;
    }
    
    // Validate model handle
    if (!model_handle->session) {
        fprintf(stderr, "ERROR: Invalid model handle (session is NULL)\n");
        return -ENODEV;
    }
    
    // Validate input/output sizes
    int ret = validate_io_sizes(model_handle, input_size, output_size);
    if (ret != 0) {
        return ret;
    }
    
    const OrtApi* ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    if (!ort) {
        fprintf(stderr, "ERROR: Failed to get ONNX Runtime API\n");
        return -ENOTSUP;
    }
    
    // Prepare input shape (batch size 1)
    int64_t input_shape[2] = {1, (int64_t)input_size};
    size_t input_shape_len = 2;
    
    // If model has shape information, use it
    if (model_handle->input_count > 0 && model_handle->input_shapes && 
        model_handle->input_shape_ranks && model_handle->input_shape_ranks[0] > 0) {
        // Use model's input shape
        input_shape_len = model_handle->input_shape_ranks[0];
        if (input_shape_len > 2) {
            // Allocate larger shape array if needed
            int64_t *dynamic_shape = malloc(input_shape_len * sizeof(int64_t));
            if (!dynamic_shape) {
                return -ENOMEM;
            }
            memcpy(dynamic_shape, model_handle->input_shapes[0], 
                   input_shape_len * sizeof(int64_t));
            // Ensure batch size is 1
            dynamic_shape[0] = 1;
            
            // Create input tensor
            OrtValue* input_tensor = NULL;
            ret = dsmil_onnx_create_input_tensor(ort, input, input_size,
                                                 dynamic_shape, input_shape_len,
                                                 &input_tensor);
            free(dynamic_shape);
            
            if (ret != 0) {
                return ret;
            }
            
            // Get input/output names
            if (model_handle->input_count == 0 || !model_handle->input_names ||
                model_handle->output_count == 0 || !model_handle->output_names) {
                ort->ReleaseValue(input_tensor);
                return -EINVAL;
            }
            
            const char* input_name = model_handle->input_names[0];
            const char* output_name = model_handle->output_names[0];
            
            // Prepare arrays for Run()
            const char* const input_names[] = {input_name};
            const OrtValue* input_tensors[] = {input_tensor};
            const char* const output_names[] = {output_name};
            OrtValue* output_tensor = NULL;
            OrtValue* output_tensors[] = {&output_tensor};
            
            // Run inference
            ret = dsmil_onnx_run_inference(ort, model_handle->session,
                                          input_names, input_tensors, 1,
                                          output_names, output_tensors, 1);
            
            if (ret != 0) {
                ort->ReleaseValue(input_tensor);
                return ret;
            }
            
            // Extract output
            ret = dsmil_onnx_extract_output(ort, output_tensor, output, output_size);
            
            // Cleanup
            ort->ReleaseValue(input_tensor);
            if (output_tensor) {
                ort->ReleaseValue(output_tensor);
            }
            
            if (ret != 0) {
                return ret;
            }
            
            // Validate output
            ret = validate_output_data(output, output_size);
            return ret;
        }
    }
    
    // Create input tensor with default shape [1, input_size]
    OrtValue* input_tensor = NULL;
    ret = dsmil_onnx_create_input_tensor(ort, input, input_size,
                                         input_shape, input_shape_len,
                                         &input_tensor);
    if (ret != 0) {
        return ret;
    }
    
    // Get input/output names
    if (model_handle->input_count == 0 || !model_handle->input_names ||
        model_handle->output_count == 0 || !model_handle->output_names) {
        ort->ReleaseValue(input_tensor);
        fprintf(stderr, "ERROR: Model metadata not available\n");
        return -EINVAL;
    }
    
    const char* input_name = model_handle->input_names[0];
    const char* output_name = model_handle->output_names[0];
    
    // Prepare arrays for Run()
    const char* const input_names[] = {input_name};
    const OrtValue* input_tensors[] = {input_tensor};
    const char* const output_names[] = {output_name};
    OrtValue* output_tensor = NULL;
    OrtValue* output_tensors[] = {&output_tensor};
    
    // Run inference
    ret = dsmil_onnx_run_inference(ort, model_handle->session,
                                  input_names, input_tensors, 1,
                                  output_names, output_tensors, 1);
    
    if (ret != 0) {
        ort->ReleaseValue(input_tensor);
        return ret;
    }
    
    // Extract output
    ret = dsmil_onnx_extract_output(ort, output_tensor, output, output_size);
    
    // Cleanup
    ort->ReleaseValue(input_tensor);
    if (output_tensor) {
        ort->ReleaseValue(output_tensor);
    }
    
    if (ret != 0) {
        return ret;
    }
    
    // Validate output
    ret = validate_output_data(output, output_size);
    return ret;
}

