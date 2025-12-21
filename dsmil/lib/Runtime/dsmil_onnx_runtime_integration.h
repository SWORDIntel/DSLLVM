/**
 * @file dsmil_onnx_runtime_integration.h
 * @brief ONNX Runtime Integration Layer Header
 * 
 * Internal header for ONNX Runtime integration functions.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#ifndef DSMIL_ONNX_RUNTIME_INTEGRATION_H
#define DSMIL_ONNX_RUNTIME_INTEGRATION_H

#include <onnxruntime_c_api.h>
#include "dsmil_int8_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize ONNX Runtime environment
 * 
 * @return 0 on success, negative on error
 */
int dsmil_onnx_env_init(void);

/**
 * @brief Cleanup ONNX Runtime environment
 */
void dsmil_onnx_env_cleanup(void);

/**
 * @brief Get ONNX Runtime environment
 * 
 * @return OrtEnv pointer, or NULL if not initialized
 */
OrtEnv* dsmil_onnx_get_env(void);

/**
 * @brief Translate ONNX Runtime error to DSMIL error code
 * 
 * @param status ONNX Runtime status
 * @return DSMIL error code (negative)
 */
int dsmil_onnx_translate_error(OrtStatus* status);

/**
 * @brief Create ONNX Runtime session
 */
int dsmil_onnx_create_session(OrtEnv* env,
                              const char *model_path,
                              const dsmil_int8_model_load_options_t *options,
                              OrtSession **session);

/**
 * @brief Create input tensor
 */
int dsmil_onnx_create_input_tensor(const OrtApi* ort,
                                   const float *data,
                                   size_t data_size,
                                   const int64_t *shape,
                                   size_t shape_len,
                                   OrtValue **tensor);

/**
 * @brief Run inference
 */
int dsmil_onnx_run_inference(const OrtApi* ort,
                             OrtSession* session,
                             const char* const* input_names,
                             const OrtValue* const* input_tensors,
                             size_t num_inputs,
                             const char* const* output_names,
                             OrtValue** output_tensors,
                             size_t num_outputs);

/**
 * @brief Extract output from tensor
 */
int dsmil_onnx_extract_output(const OrtApi* ort,
                              OrtValue* tensor,
                              float *data,
                              size_t data_size);

#ifdef __cplusplus
}
#endif

#endif /* DSMIL_ONNX_RUNTIME_INTEGRATION_H */

