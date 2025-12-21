/**
 * @file dsmil_int8_model.h
 * @brief INT8 Model Loading and Inference API for DSMIL
 * 
 * Production-grade API for loading and executing INT8 quantized ONNX/TFLite
 * models with support for multiple execution providers (CPU/GPU/NPU) and
 * device-specific optimization.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef DSMIL_INT8_MODEL_H
#define DSMIL_INT8_MODEL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup DSMIL_INT8_MODEL INT8 Model Operations
 * @{
 */

/**
 * @brief Quantization scheme types
 */
typedef enum {
    DSMIL_QUANT_INT8_SYMMETRIC,      // Symmetric quantization (zero point = 0)
    DSMIL_QUANT_INT8_ASYMMETRIC,     // Asymmetric quantization (zero point != 0)
    DSMIL_QUANT_INT8_PER_TENSOR,    // Single scale/zero-point per tensor
    DSMIL_QUANT_INT8_PER_CHANNEL,    // Per-channel quantization
    DSMIL_QUANT_INT8_DYNAMIC         // Dynamic quantization (runtime scales)
} dsmil_quantization_scheme_t;

/**
 * @brief Model loading options
 */
typedef struct {
    uint8_t device_id;              // Device ID (51-58)
    bool use_gpu;                   // Use GPU acceleration (CUDA)
    bool use_npu;                   // Use NPU acceleration (OpenVINO/TensorRT)
    dsmil_quantization_scheme_t quantization_scheme;  // Quantization scheme
    uint32_t intra_op_num_threads;  // Number of threads for intra-op parallelism
    uint32_t inter_op_num_threads;  // Number of threads for inter-op parallelism
    bool enable_memory_pattern;     // Enable memory pattern optimization
    bool enable_cpu_mem_arena;      // Enable CPU memory arena
    int graph_optimization_level;    // Graph optimization level (0-4)
} dsmil_int8_model_load_options_t;

/**
 * @brief Model handle (opaque type)
 */
typedef struct dsmil_model_handle dsmil_model_handle_t;

/**
 * @brief Load INT8 quantized ONNX or TensorFlow Lite model
 * 
 * Loads a model file and creates a session for inference. The model handle
 * must be freed with dsmil_model_cleanup() when no longer needed.
 * 
 * @param model_path Path to model file (.onnx or .tflite)
 * @param options Model loading options (NULL for defaults)
 * @param model_handle Output model handle pointer
 * @return 0 on success, negative error code on failure
 * 
 * Error codes:
 * - -EINVAL: Invalid parameters (NULL path or handle)
 * - -ENOENT: Model file not found
 * - -ENOMEM: Memory allocation failed
 * - -EIO: Model file read/parse error
 * - -ENOTSUP: Model format not supported or execution provider not available
 * 
 * @thread_safety Thread-safe (can be called from multiple threads)
 * @memory_ownership Caller must call dsmil_model_cleanup() to free model_handle
 */
int dsmil_int8_model_load(const char *model_path,
                          const dsmil_int8_model_load_options_t *options,
                          dsmil_model_handle_t **model_handle);

/**
 * @brief Run INT8 quantized model inference
 * 
 * Executes inference on a loaded model with the provided input data.
 * Input and output are expected to be float arrays, with INT8 quantization
 * handled internally by ONNX Runtime.
 * 
 * @param model_handle Model handle from dsmil_int8_model_load()
 * @param input Input feature vector (float array)
 * @param input_size Number of input features
 * @param output Output buffer (float array, must be pre-allocated)
 * @param output_size Number of output values
 * @return 0 on success, negative error code on failure
 * 
 * Error codes:
 * - -EINVAL: Invalid parameters (NULL handle, input, or output)
 * - -ENODEV: Invalid model handle (not loaded or cleaned up)
 * - -ENOMEM: Memory allocation failed
 * - -EIO: Inference execution error
 * - -ERANGE: Input/output size mismatch with model expectations
 * 
 * @thread_safety Not thread-safe (model_handle must not be used concurrently)
 * @memory_ownership Input/output buffers are caller-owned
 */
int dsmil_model_infer_int8(dsmil_model_handle_t *model_handle,
                           const float *input,
                           size_t input_size,
                           float *output,
                           size_t output_size);

/**
 * @brief Cleanup and free model handle
 * 
 * Releases all resources associated with a model handle, including
 * the ONNX Runtime session, metadata, and allocated memory.
 * 
 * @param model_handle Model handle to cleanup (can be NULL)
 * 
 * @thread_safety Not thread-safe (model_handle must not be used concurrently)
 */
void dsmil_model_cleanup(dsmil_model_handle_t *model_handle);

/**
 * @brief Get model input/output metadata
 * 
 * Retrieves information about model inputs and outputs, including
 * names, shapes, and data types.
 * 
 * @param model_handle Model handle
 * @param input_count Output number of input nodes
 * @param output_count Output number of output nodes
 * @param input_names Output array of input node names (caller must free)
 * @param output_names Output array of output node names (caller must free)
 * @return 0 on success, negative error code on failure
 * 
 * @thread_safety Not thread-safe
 * @memory_ownership Caller must free input_names and output_names arrays
 */
int dsmil_model_get_metadata(dsmil_model_handle_t *model_handle,
                            size_t *input_count,
                            size_t *output_count,
                            char ***input_names,
                            char ***output_names);

/**
 * @brief Get model input shape
 * 
 * Retrieves the expected input shape for a specific input node.
 * 
 * @param model_handle Model handle
 * @param input_index Input node index (0-based)
 * @param shape Output shape array (caller must free)
 * @param shape_rank Output number of dimensions
 * @return 0 on success, negative error code on failure
 * 
 * @thread_safety Not thread-safe
 * @memory_ownership Caller must free shape array
 */
int dsmil_model_get_input_shape(dsmil_model_handle_t *model_handle,
                                size_t input_index,
                                int64_t **shape,
                                size_t *shape_rank);

/**
 * @brief Get model output shape
 * 
 * Retrieves the expected output shape for a specific output node.
 * 
 * @param model_handle Model handle
 * @param output_index Output node index (0-based)
 * @param shape Output shape array (caller must free)
 * @param shape_rank Output number of dimensions
 * @return 0 on success, negative error code on failure
 * 
 * @thread_safety Not thread-safe
 * @memory_ownership Caller must free shape array
 */
int dsmil_model_get_output_shape(dsmil_model_handle_t *model_handle,
                                size_t output_index,
                                int64_t **shape,
                                size_t *shape_rank);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* DSMIL_INT8_MODEL_H */

