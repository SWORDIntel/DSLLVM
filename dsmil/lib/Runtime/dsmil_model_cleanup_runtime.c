/**
 * @file dsmil_model_cleanup_runtime.c
 * @brief Model Cleanup Implementation
 * 
 * Implements dsmil_model_cleanup() for proper resource deallocation
 * of model handles and associated ONNX Runtime resources.
 * 
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_int8_model.h"
#include <onnxruntime_c_api.h>
#include <stdlib.h>
#include <stdio.h>

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
 * @brief Cleanup model handle and release all resources
 */
void dsmil_model_cleanup(dsmil_model_handle_t *model_handle) {
    if (!model_handle) {
        return;
    }
    
    const OrtApi* ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    if (ort) {
        // Release input names
        if (model_handle->input_names) {
            for (size_t i = 0; i < model_handle->input_count; i++) {
                if (model_handle->input_names[i]) {
                    ort->AllocatorFree(OrtAllocatorDefault, model_handle->input_names[i]);
                }
            }
            free(model_handle->input_names);
        }
        
        // Release output names
        if (model_handle->output_names) {
            for (size_t i = 0; i < model_handle->output_count; i++) {
                if (model_handle->output_names[i]) {
                    ort->AllocatorFree(OrtAllocatorDefault, model_handle->output_names[i]);
                }
            }
            free(model_handle->output_names);
        }
        
        // Release input shapes
        if (model_handle->input_shapes) {
            for (size_t i = 0; i < model_handle->input_count; i++) {
                free(model_handle->input_shapes[i]);
            }
            free(model_handle->input_shapes);
        }
        free(model_handle->input_shape_ranks);
        
        // Release output shapes
        if (model_handle->output_shapes) {
            for (size_t i = 0; i < model_handle->output_count; i++) {
                free(model_handle->output_shapes[i]);
            }
            free(model_handle->output_shapes);
        }
        free(model_handle->output_shape_ranks);
        
        // Release memory info
        if (model_handle->memory_info) {
            ort->ReleaseMemoryInfo(model_handle->memory_info);
        }
        
        // Release session
        if (model_handle->session) {
            ort->ReleaseSession(model_handle->session);
        }
    }
    
    // Free model path
    if (model_handle->model_path) {
        free(model_handle->model_path);
    }
    
    // Free handle structure
    free(model_handle);
}

