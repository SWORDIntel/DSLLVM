/**
 * @file dsmil_int8_model_load_runtime.c
 * @brief INT8 Model Loading Implementation
 * 
 * Implements dsmil_int8_model_load() for loading INT8 quantized ONNX/TFLite
 * models with full error handling, device selection, and execution provider
 * configuration.
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
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>

/**
 * @brief Model handle structure
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
 * @brief Forward declaration for integration functions
 */
extern OrtEnv* dsmil_onnx_get_env(void);
extern int dsmil_onnx_create_session(OrtEnv* env,
                                     const char *model_path,
                                     const dsmil_int8_model_load_options_t *options,
                                     OrtSession **session);
extern int dsmil_onnx_translate_error(OrtStatus* status);

/**
 * @brief Verify model file exists and is readable
 * 
 * @param model_path Path to model file
 * @return 0 if valid, negative on error
 */
static int verify_model_file(const char *model_path) {
    if (!model_path) {
        return -EINVAL;
    }
    
    // Check file exists and is readable
    if (access(model_path, R_OK) != 0) {
        if (errno == ENOENT) {
            fprintf(stderr, "ERROR: Model file not found: %s\n", model_path);
            return -ENOENT;
        } else if (errno == EACCES) {
            fprintf(stderr, "ERROR: Model file not readable: %s\n", model_path);
            return -EACCES;
        } else {
            fprintf(stderr, "ERROR: Cannot access model file: %s\n", model_path);
            return -EIO;
        }
    }
    
    // Check file is not a directory
    struct stat st;
    if (stat(model_path, &st) != 0) {
        fprintf(stderr, "ERROR: Cannot stat model file: %s\n", model_path);
        return -EIO;
    }
    
    if (S_ISDIR(st.st_mode)) {
        fprintf(stderr, "ERROR: Model path is a directory: %s\n", model_path);
        return -EINVAL;
    }
    
    // Check file has reasonable size (> 0 bytes)
    if (st.st_size == 0) {
        fprintf(stderr, "ERROR: Model file is empty: %s\n", model_path);
        return -EIO;
    }
    
    return 0;
}

/**
 * @brief Extract model metadata (input/output names and shapes)
 * 
 * @param ort ONNX Runtime API
 * @param session Model session
 * @param handle Model handle to populate
 * @return 0 on success, negative on error
 */
static int extract_model_metadata(const OrtApi* ort,
                                 OrtSession* session,
                                 dsmil_model_handle_t *handle) {
    if (!ort || !session || !handle) {
        return -EINVAL;
    }
    
    // Get input count
    size_t num_input_nodes = 0;
    OrtStatus* status = ort->SessionGetInputCount(session, &num_input_nodes);
    if (status != NULL) {
        return dsmil_onnx_translate_error(status);
    }
    
    // Get output count
    size_t num_output_nodes = 0;
    status = ort->SessionGetOutputCount(session, &num_output_nodes);
    if (status != NULL) {
        return dsmil_onnx_translate_error(status);
    }
    
    handle->input_count = num_input_nodes;
    handle->output_count = num_output_nodes;
    
    // Allocate arrays for input names and shapes
    if (num_input_nodes > 0) {
        handle->input_names = calloc(num_input_nodes, sizeof(char*));
        handle->input_shapes = calloc(num_input_nodes, sizeof(int64_t*));
        handle->input_shape_ranks = calloc(num_input_nodes, sizeof(size_t));
        
        if (!handle->input_names || !handle->input_shapes || !handle->input_shape_ranks) {
            return -ENOMEM;
        }
        
        // Extract input names and shapes
        for (size_t i = 0; i < num_input_nodes; i++) {
            char* input_name = NULL;
            status = ort->SessionGetInputName(session, i, OrtAllocatorDefault, &input_name);
            if (status != NULL) {
                // Cleanup on error
                for (size_t j = 0; j < i; j++) {
                    ort->AllocatorFree(OrtAllocatorDefault, handle->input_names[j]);
                }
                free(handle->input_names);
                free(handle->input_shapes);
                free(handle->input_shape_ranks);
                return dsmil_onnx_translate_error(status);
            }
            handle->input_names[i] = input_name;
            
            // Get input type and shape
            OrtTypeInfo* type_info = NULL;
            status = ort->SessionGetInputTypeInfo(session, i, &type_info);
            if (status != NULL) {
                // Cleanup on error
                for (size_t j = 0; j <= i; j++) {
                    ort->AllocatorFree(OrtAllocatorDefault, handle->input_names[j]);
                }
                free(handle->input_names);
                free(handle->input_shapes);
                free(handle->input_shape_ranks);
                return dsmil_onnx_translate_error(status);
            }
            
            const OrtTensorTypeAndShapeInfo* tensor_info = ort->CastTypeInfoToTensorInfo(type_info);
            if (tensor_info) {
                size_t num_dims = 0;
                status = ort->GetDimensionsCount(tensor_info, &num_dims);
                if (status == NULL && num_dims > 0) {
                    int64_t* dims = malloc(num_dims * sizeof(int64_t));
                    if (dims) {
                        status = ort->GetDimensions(tensor_info, dims, num_dims);
                        if (status == NULL) {
                            handle->input_shapes[i] = dims;
                            handle->input_shape_ranks[i] = num_dims;
                        } else {
                            free(dims);
                            ort->ReleaseStatus(status);
                        }
                    }
                }
            }
            
            ort->ReleaseTypeInfo(type_info);
        }
    }
    
    // Allocate arrays for output names and shapes
    if (num_output_nodes > 0) {
        handle->output_names = calloc(num_output_nodes, sizeof(char*));
        handle->output_shapes = calloc(num_output_nodes, sizeof(int64_t*));
        handle->output_shape_ranks = calloc(num_output_nodes, sizeof(size_t));
        
        if (!handle->output_names || !handle->output_shapes || !handle->output_shape_ranks) {
            // Cleanup inputs
            if (handle->input_names) {
                for (size_t i = 0; i < handle->input_count; i++) {
                    ort->AllocatorFree(OrtAllocatorDefault, handle->input_names[i]);
                    free(handle->input_shapes[i]);
                }
                free(handle->input_names);
                free(handle->input_shapes);
                free(handle->input_shape_ranks);
            }
            return -ENOMEM;
        }
        
        // Extract output names and shapes
        for (size_t i = 0; i < num_output_nodes; i++) {
            char* output_name = NULL;
            status = ort->SessionGetOutputName(session, i, OrtAllocatorDefault, &output_name);
            if (status != NULL) {
                // Cleanup on error
                for (size_t j = 0; j < i; j++) {
                    ort->AllocatorFree(OrtAllocatorDefault, handle->output_names[j]);
                }
                // Cleanup inputs
                if (handle->input_names) {
                    for (size_t k = 0; k < handle->input_count; k++) {
                        ort->AllocatorFree(OrtAllocatorDefault, handle->input_names[k]);
                        free(handle->input_shapes[k]);
                    }
                    free(handle->input_names);
                    free(handle->input_shapes);
                    free(handle->input_shape_ranks);
                }
                free(handle->output_names);
                free(handle->output_shapes);
                free(handle->output_shape_ranks);
                return dsmil_onnx_translate_error(status);
            }
            handle->output_names[i] = output_name;
            
            // Get output type and shape
            OrtTypeInfo* type_info = NULL;
            status = ort->SessionGetOutputTypeInfo(session, i, &type_info);
            if (status != NULL) {
                // Cleanup on error
                for (size_t j = 0; j <= i; j++) {
                    ort->AllocatorFree(OrtAllocatorDefault, handle->output_names[j]);
                }
                // Cleanup inputs
                if (handle->input_names) {
                    for (size_t k = 0; k < handle->input_count; k++) {
                        ort->AllocatorFree(OrtAllocatorDefault, handle->input_names[k]);
                        free(handle->input_shapes[k]);
                    }
                    free(handle->input_names);
                    free(handle->input_shapes);
                    free(handle->input_shape_ranks);
                }
                free(handle->output_names);
                free(handle->output_shapes);
                free(handle->output_shape_ranks);
                return dsmil_onnx_translate_error(status);
            }
            
            const OrtTensorTypeAndShapeInfo* tensor_info = ort->CastTypeInfoToTensorInfo(type_info);
            if (tensor_info) {
                size_t num_dims = 0;
                status = ort->GetDimensionsCount(tensor_info, &num_dims);
                if (status == NULL && num_dims > 0) {
                    int64_t* dims = malloc(num_dims * sizeof(int64_t));
                    if (dims) {
                        status = ort->GetDimensions(tensor_info, dims, num_dims);
                        if (status == NULL) {
                            handle->output_shapes[i] = dims;
                            handle->output_shape_ranks[i] = num_dims;
                        } else {
                            free(dims);
                            ort->ReleaseStatus(status);
                        }
                    }
                }
            }
            
            ort->ReleaseTypeInfo(type_info);
        }
    }
    
    return 0;
}

/**
 * @brief Load INT8 quantized model
 */
int dsmil_int8_model_load(const char *model_path,
                         const dsmil_int8_model_load_options_t *options,
                         dsmil_model_handle_t **model_handle) {
    if (!model_path || !model_handle) {
        fprintf(stderr, "ERROR: Invalid parameters to dsmil_int8_model_load\n");
        return -EINVAL;
    }
    
    // Verify model file
    int ret = verify_model_file(model_path);
    if (ret != 0) {
        return ret;
    }
    
    // Initialize ONNX Runtime environment
    OrtEnv* env = dsmil_onnx_get_env();
    if (!env) {
        fprintf(stderr, "ERROR: Failed to initialize ONNX Runtime environment\n");
        return -EIO;
    }
    
    // Allocate model handle
    dsmil_model_handle_t *handle = calloc(1, sizeof(dsmil_model_handle_t));
    if (!handle) {
        fprintf(stderr, "ERROR: Memory allocation failed for model handle\n");
        return -ENOMEM;
    }
    
    // Store model path
    handle->model_path = strdup(model_path);
    if (!handle->model_path) {
        free(handle);
        fprintf(stderr, "ERROR: Memory allocation failed for model path\n");
        return -ENOMEM;
    }
    
    // Store options
    if (options) {
        handle->device_id = options->device_id;
        handle->use_gpu = options->use_gpu;
        handle->use_npu = options->use_npu;
    } else {
        handle->device_id = 51;  // Default device
        handle->use_gpu = false;
        handle->use_npu = false;
    }
    
    // Create ONNX Runtime session
    ret = dsmil_onnx_create_session(env, model_path, options, &handle->session);
    if (ret != 0) {
        free(handle->model_path);
        free(handle);
        return ret;
    }
    
    // Create memory info for CPU
    const OrtApi* ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    OrtStatus* status = ort->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &handle->memory_info);
    if (status != NULL) {
        ret = dsmil_onnx_translate_error(status);
        ort->ReleaseSession(handle->session);
        free(handle->model_path);
        free(handle);
        return ret;
    }
    
    // Extract model metadata
    ret = extract_model_metadata(ort, handle->session, handle);
    if (ret != 0) {
        ort->ReleaseMemoryInfo(handle->memory_info);
        ort->ReleaseSession(handle->session);
        free(handle->model_path);
        free(handle);
        return ret;
    }
    
    *model_handle = handle;
    
    fprintf(stdout, "INFO: Model loaded successfully: %s (inputs: %zu, outputs: %zu)\n",
            model_path, handle->input_count, handle->output_count);
    
    return 0;
}

