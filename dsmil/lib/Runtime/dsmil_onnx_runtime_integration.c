/**
 * @file dsmil_onnx_runtime_integration.c
 * @brief ONNX Runtime Integration Layer
 * 
 * Low-level wrapper for ONNX Runtime C API providing:
 * - OrtEnv singleton management
 * - Session creation and management
 * - Tensor allocation and manipulation
 * - Execution provider selection
 * - Error code translation
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_int8_model.h"
#include <onnxruntime_c_api.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>

/**
 * @brief ONNX Runtime environment (singleton)
 */
static struct {
    OrtEnv *env;
    bool initialized;
    pthread_mutex_t init_mutex;
} g_onnx_env = {
    .env = NULL,
    .initialized = false,
    .init_mutex = PTHREAD_MUTEX_INITIALIZER
};

/**
 * @brief Get ONNX Runtime API structure
 */
static const OrtApi* get_ort_api(void) {
    return OrtGetApiBase()->GetApi(ORT_API_VERSION);
}

/**
 * @brief Initialize ONNX Runtime environment (thread-safe singleton)
 * 
 * @return 0 on success, negative on error
 */
int dsmil_onnx_env_init(void) {
    if (g_onnx_env.initialized && g_onnx_env.env != NULL) {
        return 0;  // Already initialized
    }
    
    pthread_mutex_lock(&g_onnx_env.init_mutex);
    
    // Double-check after acquiring lock
    if (g_onnx_env.initialized && g_onnx_env.env != NULL) {
        pthread_mutex_unlock(&g_onnx_env.init_mutex);
        return 0;
    }
    
    const OrtApi* ort = get_ort_api();
    if (!ort) {
        fprintf(stderr, "ERROR: Failed to get ONNX Runtime API\n");
        pthread_mutex_unlock(&g_onnx_env.init_mutex);
        return -ENOTSUP;
    }
    
    OrtStatus* status = ort->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "DSMIL", &g_onnx_env.env);
    if (status != NULL) {
        const char* msg = ort->GetErrorMessage(status);
        fprintf(stderr, "ERROR: ONNX Runtime environment initialization failed: %s\n", msg);
        ort->ReleaseStatus(status);
        pthread_mutex_unlock(&g_onnx_env.init_mutex);
        return -EIO;
    }
    
    g_onnx_env.initialized = true;
    pthread_mutex_unlock(&g_onnx_env.init_mutex);
    
    return 0;
}

/**
 * @brief Cleanup ONNX Runtime environment
 */
void dsmil_onnx_env_cleanup(void) {
    pthread_mutex_lock(&g_onnx_env.init_mutex);
    
    if (g_onnx_env.env != NULL) {
        const OrtApi* ort = get_ort_api();
        if (ort) {
            ort->ReleaseEnv(g_onnx_env.env);
        }
        g_onnx_env.env = NULL;
    }
    
    g_onnx_env.initialized = false;
    pthread_mutex_unlock(&g_onnx_env.init_mutex);
}

/**
 * @brief Get ONNX Runtime environment
 * 
 * @return OrtEnv pointer, or NULL if not initialized
 */
OrtEnv* dsmil_onnx_get_env(void) {
    if (!g_onnx_env.initialized || g_onnx_env.env == NULL) {
        int ret = dsmil_onnx_env_init();
        if (ret != 0) {
            return NULL;
        }
    }
    return g_onnx_env.env;
}

/**
 * @brief Translate ONNX Runtime error to DSMIL error code
 * 
 * @param status ONNX Runtime status
 * @return DSMIL error code (negative)
 */
int dsmil_onnx_translate_error(OrtStatus* status) {
    if (status == NULL) {
        return 0;  // Success
    }
    
    const OrtApi* ort = get_ort_api();
    if (!ort) {
        return -EIO;
    }
    
    OrtErrorCode error_code = ort->GetErrorCode(status);
    const char* error_msg = ort->GetErrorMessage(status);
    
    fprintf(stderr, "ERROR: ONNX Runtime error [%d]: %s\n", (int)error_code, error_msg);
    
    int dsmil_error;
    switch (error_code) {
        case ORT_FAIL:
            dsmil_error = -EIO;
            break;
        case ORT_INVALID_ARGUMENT:
            dsmil_error = -EINVAL;
            break;
        case ORT_NO_SUCHFILE:
            dsmil_error = -ENOENT;
            break;
        case ORT_NO_MODEL:
            dsmil_error = -EIO;
            break;
        case ORT_ENGINE_ERROR:
            dsmil_error = -EIO;
            break;
        case ORT_RUNTIME_EXCEPTION:
            dsmil_error = -EIO;
            break;
        case ORT_INVALID_GRAPH:
            dsmil_error = -EIO;
            break;
        case ORT_EP_FAIL:
            dsmil_error = -ENOTSUP;
            break;
        default:
            dsmil_error = -EIO;
            break;
    }
    
    ort->ReleaseStatus(status);
    return dsmil_error;
}

/**
 * @brief Create ONNX Runtime session with options
 * 
 * @param env ONNX Runtime environment
 * @param model_path Path to model file
 * @param options Model loading options
 * @param session Output session pointer
 * @return 0 on success, negative on error
 */
int dsmil_onnx_create_session(OrtEnv* env,
                              const char *model_path,
                              const dsmil_int8_model_load_options_t *options,
                              OrtSession **session) {
    if (!env || !model_path || !session) {
        return -EINVAL;
    }
    
    const OrtApi* ort = get_ort_api();
    if (!ort) {
        return -ENOTSUP;
    }
    
    // Create session options
    OrtSessionOptions* session_options = NULL;
    OrtStatus* status = ort->CreateSessionOptions(&session_options);
    if (status != NULL) {
        int ret = dsmil_onnx_translate_error(status);
        return ret;
    }
    
    // Set graph optimization level
    // ORT_ENABLE_ALL = 99, ORT_DISABLE_ALL = 0, ORT_ENABLE_BASIC = 1, ORT_ENABLE_EXTENDED = 2
    int opt_level = 99;  // ORT_ENABLE_ALL
    if (options && options->graph_optimization_level >= 0) {
        opt_level = options->graph_optimization_level;
    }
    ort->SetSessionGraphOptimizationLevel(session_options, opt_level);
    
    // Set thread counts
    uint32_t intra_threads = 0;  // 0 = use default
    uint32_t inter_threads = 0;
    if (options) {
        if (options->intra_op_num_threads > 0) {
            intra_threads = options->intra_op_num_threads;
        }
        if (options->inter_op_num_threads > 0) {
            inter_threads = options->inter_op_num_threads;
        }
    }
    if (intra_threads > 0) {
        ort->SetIntraOpNumThreads(session_options, (int)intra_threads);
    }
    if (inter_threads > 0) {
        ort->SetInterOpNumThreads(session_options, (int)inter_threads);
    }
    
    // Enable memory optimizations
    bool enable_mem_pattern = true;
    bool enable_cpu_arena = true;
    if (options) {
        enable_mem_pattern = options->enable_memory_pattern;
        enable_cpu_arena = options->enable_cpu_mem_arena;
    }
    if (enable_mem_pattern) {
        ort->EnableMemPattern(session_options);
    }
    if (enable_cpu_arena) {
        ort->EnableCpuMemArena(session_options);
    }
    
    // Configure execution provider based on options
    if (options) {
        if (options->use_gpu) {
            // Try CUDA execution provider
            OrtStatus* ep_status = ort->SessionOptionsAppendExecutionProvider_CUDA(
                session_options, 0);
            if (ep_status != NULL) {
                // CUDA not available, continue with CPU
                ort->ReleaseStatus(ep_status);
            }
        }
        
        if (options->use_npu) {
            // Try OpenVINO execution provider (Intel NPU)
            OrtStatus* ep_status = ort->SessionOptionsAppendExecutionProvider_OpenVINO(
                session_options, NULL);
            if (ep_status != NULL) {
                // OpenVINO not available, try TensorRT
                ort->ReleaseStatus(ep_status);
                ep_status = ort->SessionOptionsAppendExecutionProvider_TensorRT(
                    session_options, 0);
                if (ep_status != NULL) {
                    // TensorRT not available, continue with CPU
                    ort->ReleaseStatus(ep_status);
                }
            }
        }
    }
    
    // Create session
    status = ort->CreateSession(env, model_path, session_options, session);
    if (status != NULL) {
        int ret = dsmil_onnx_translate_error(status);
        ort->ReleaseSessionOptions(session_options);
        return ret;
    }
    
    // Cleanup session options (session retains a copy)
    ort->ReleaseSessionOptions(session_options);
    
    return 0;
}

/**
 * @brief Create input tensor from float array
 * 
 * @param ort ONNX Runtime API
 * @param data Input data (float array)
 * @param data_size Number of elements
 * @param shape Tensor shape
 * @param shape_len Number of dimensions
 * @param tensor Output tensor pointer
 * @return 0 on success, negative on error
 */
int dsmil_onnx_create_input_tensor(const OrtApi* ort,
                                   const float *data,
                                   size_t data_size,
                                   const int64_t *shape,
                                   size_t shape_len,
                                   OrtValue **tensor) {
    if (!ort || !data || !shape || !tensor || data_size == 0) {
        return -EINVAL;
    }
    
    // Create memory info for CPU
    OrtMemoryInfo* memory_info = NULL;
    OrtStatus* status = ort->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &memory_info);
    if (status != NULL) {
        return dsmil_onnx_translate_error(status);
    }
    
    // Create tensor
    status = ort->CreateTensorWithDataAsOrtValue(
        memory_info,
        (void*)data, data_size * sizeof(float),
        (int64_t*)shape, shape_len,
        ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
        tensor);
    
    ort->ReleaseMemoryInfo(memory_info);
    
    if (status != NULL) {
        return dsmil_onnx_translate_error(status);
    }
    
    return 0;
}

/**
 * @brief Run inference
 * 
 * @param ort ONNX Runtime API
 * @param session Model session
 * @param input_names Input node names
 * @param input_tensors Input tensors
 * @param num_inputs Number of inputs
 * @param output_names Output node names
 * @param output_tensors Output tensors (pre-allocated array)
 * @param num_outputs Number of outputs
 * @return 0 on success, negative on error
 */
int dsmil_onnx_run_inference(const OrtApi* ort,
                             OrtSession* session,
                             const char* const* input_names,
                             const OrtValue* const* input_tensors,
                             size_t num_inputs,
                             const char* const* output_names,
                             OrtValue** output_tensors,
                             size_t num_outputs) {
    if (!ort || !session || !input_names || !input_tensors || 
        !output_names || !output_tensors) {
        return -EINVAL;
    }
    
    OrtStatus* status = ort->Run(session, NULL,
                                 input_names, input_tensors, num_inputs,
                                 output_names, output_tensors, num_outputs);
    
    if (status != NULL) {
        return dsmil_onnx_translate_error(status);
    }
    
    return 0;
}

/**
 * @brief Extract output data from tensor
 * 
 * @param ort ONNX Runtime API
 * @param tensor Output tensor
 * @param data Output buffer (must be pre-allocated)
 * @param data_size Number of elements
 * @return 0 on success, negative on error
 */
int dsmil_onnx_extract_output(const OrtApi* ort,
                               OrtValue* tensor,
                               float *data,
                               size_t data_size) {
    if (!ort || !tensor || !data || data_size == 0) {
        return -EINVAL;
    }
    
    void* tensor_data = NULL;
    OrtStatus* status = ort->GetTensorMutableData(tensor, &tensor_data);
    if (status != NULL) {
        return dsmil_onnx_translate_error(status);
    }
    
    // Get tensor info to verify type and size
    OrtTensorTypeAndShapeInfo* type_info = NULL;
    status = ort->GetTensorTypeAndShape(tensor, &type_info);
    if (status != NULL) {
        return dsmil_onnx_translate_error(status);
    }
    
    size_t tensor_size = 0;
    status = ort->GetTensorShapeElementCount(type_info, &tensor_size);
    if (status != NULL) {
        ort->ReleaseTensorTypeAndShapeInfo(type_info);
        return dsmil_onnx_translate_error(status);
    }
    
    if (tensor_size > data_size) {
        ort->ReleaseTensorTypeAndShapeInfo(type_info);
        return -ERANGE;
    }
    
    // Copy data
    memcpy(data, tensor_data, tensor_size * sizeof(float));
    
    ort->ReleaseTensorTypeAndShapeInfo(type_info);
    
    return 0;
}

