/*
 * DSMIL Model APIs Header
 *
 * This header defines the core machine learning model APIs for
 * loading, inference, evaluation, and training operations.
 *
 * Author: DSMIL Development Team
 * Created: 2025-01-11
 */

#ifndef DSMIL_MODEL_APIS_H
#define DSMIL_MODEL_APIS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ============================================================================
 * MODEL LOADING API
 * ============================================================================ */

/**
 * @brief Model loading options structure
 */
typedef struct {
    uint8_t device_id;              /* Device ID (51-58) */
    bool use_gpu;                   /* Use GPU acceleration */
    bool use_npu;                   /* Use NPU acceleration */
    uint8_t quantization_scheme;    /* Quantization type (0=FP32, 1=INT8) */
} dsmil_int8_model_load_options_t;

/**
 * @brief Load INT8 quantized model
 *
 * @param model_path Path to model file (.onnx or .tflite)
 * @param options Loading options (NULL for defaults)
 * @param model_handle Output model handle
 * @return 0 on success, negative error code on failure
 */
int dsmil_int8_model_load(const char *model_path,
                         dsmil_int8_model_load_options_t *options,
                         void **model_handle);

/* ============================================================================
 * MODEL INFERENCE APIs
 * ============================================================================ */

/**
 * @brief General INT8 model inference
 *
 * @param model_handle Loaded model handle
 * @param input Input feature vector
 * @param input_size Number of input features
 * @param output Output buffer
 * @param output_size Number of output values
 * @return 0 on success, negative error code on failure
 */
int dsmil_model_infer_int8(void *model_handle,
                          const float *input,
                          size_t input_size,
                          float *output,
                          size_t output_size);

/**
 * @brief GNN inference for event correlation
 *
 * @param model_handle GNN model handle
 * @param node_features Array of node feature vectors (15 features per node)
 * @param num_nodes Number of nodes in graph
 * @param edge_features Array of edge feature vectors (3 features per edge)
 * @param num_edges Number of edges in graph
 * @param adjacency_matrix Graph adjacency matrix
 * @param node_embeddings Output node embeddings (16 dimensions per node)
 * @param cluster_assignments Output cluster assignments per node
 * @return 0 on success, negative error code on failure
 */
int dsmil_gnn_infer_int8(void *model_handle,
                        float **node_features,
                        size_t num_nodes,
                        float **edge_features,
                        size_t num_edges,
                        void *adjacency_matrix,
                        float **node_embeddings,
                        int *cluster_assignments);

/**
 * @brief RL policy inference for incident response
 *
 * @param model_handle RL policy model handle
 * @param state_vector Current state (32 features)
 * @param state_size Size of state vector (32)
 * @param action_scores Output action Q-values (20 possible actions)
 * @param num_actions Number of possible actions (20)
 * @return 0 on success, negative error code on failure
 */
int dsmil_rl_policy_infer_int8(void *model_handle,
                              const float *state_vector,
                              size_t state_size,
                              float *action_scores,
                              size_t num_actions);

/**
 * @brief Text classifier inference
 *
 * @param model_handle Text classifier model handle
 * @param input_tokens Tokenized input text
 * @param token_count Number of input tokens
 * @param probabilities Output class probabilities
 * @param num_classes Number of output classes
 * @return 0 on success, negative error code on failure
 */
int dsmil_text_classifier_infer_int8(void *model_handle,
                                    const int *input_tokens,
                                    size_t token_count,
                                    float *probabilities,
                                    size_t num_classes);

/**
 * @brief NLP NER inference for IOC extraction
 *
 * @param model_handle NER model handle
 * @param input_tokens Tokenized input text
 * @param token_count Number of input tokens
 * @param entity_tags Output entity tags (BIO format)
 * @param tag_count Number of output tags
 * @return 0 on success, negative error code on failure
 */
int dsmil_nlp_ner_infer_int8(void *model_handle,
                            const int *input_tokens,
                            size_t token_count,
                            int *entity_tags,
                            size_t *tag_count);

/**
 * @brief GAN generator inference
 *
 * @param model_handle GAN generator model handle
 * @param noise_input Random noise vector
 * @param noise_size Size of noise vector
 * @param generated_output Generated output
 * @param output_size Size of output
 * @return 0 on success, negative error code on failure
 */
int dsmil_gan_generator_infer_int8(void *model_handle,
                                  const float *noise_input,
                                  size_t noise_size,
                                  float *generated_output,
                                  size_t output_size);

/* ============================================================================
 * MODEL MANAGEMENT APIs
 * ============================================================================ */

/**
 * @brief Save INT8 quantized model
 *
 * @param model_handle Model handle to save
 * @param save_path Path to save model file
 * @return 0 on success, negative error code on failure
 */
int dsmil_model_save_int8(void *model_handle, const char *save_path);

/**
 * @brief Evaluate model performance
 *
 * @param model_handle Model handle
 * @param test_input Test input data
 * @param test_output Expected output data
 * @param num_samples Number of test samples
 * @param metrics Output performance metrics
 * @return 0 on success, negative error code on failure
 */
int dsmil_model_evaluate(void *model_handle,
                        const float *test_input,
                        const float *test_output,
                        size_t num_samples,
                        float *metrics);

/**
 * @brief Evaluate model robustness (adversarial testing)
 *
 * @param model_handle Model handle
 * @param input Clean input data
 * @param input_size Input size
 * @param adversarial_input Adversarially perturbed input
 * @param robustness_score Output robustness score [0.0, 1.0]
 * @return 0 on success, negative error code on failure
 */
int dsmil_model_evaluate_robust(void *model_handle,
                               const float *input,
                               size_t input_size,
                               const float *adversarial_input,
                               float *robustness_score);

/**
 * @brief Validate model integrity
 *
 * @param model_handle Model handle
 * @param expected_hash Expected model hash for integrity check
 * @param is_valid Output validation result
 * @return 0 on success, negative error code on failure
 */
int dsmil_model_validate(void *model_handle,
                        const uint8_t *expected_hash,
                        bool *is_valid);

/**
 * @brief Train model batch (online learning)
 *
 * @param model_handle Model handle
 * @param input_batch Training input batch
 * @param output_batch Expected output batch
 * @param batch_size Number of samples in batch
 * @return 0 on success, negative error code on failure
 */
int dsmil_model_train_batch_int8(void *model_handle,
                                const float *input_batch,
                                const float *output_batch,
                                size_t batch_size);

/**
 * @brief Cleanup model resources
 *
 * @param model_handle Model handle to cleanup
 * @return 0 on success, negative error code on failure
 */
int dsmil_model_cleanup(void *model_handle);

/* ============================================================================
 * MODEL UTILITIES
 * ============================================================================ */

/**
 * @brief Check if model system is available
 *
 * @return 1 if available, 0 otherwise
 */
int dsmil_model_system_available(void);

/**
 * @brief Get supported model formats
 *
 * @return Bitmask of supported formats (1=ONNX, 2=TFLite, 4=Custom)
 */
int dsmil_model_get_supported_formats(void);

/**
 * @brief Get model memory usage
 *
 * @param model_handle Model handle
 * @param memory_usage Output memory usage in bytes
 * @return 0 on success, negative error code on failure
 */
int dsmil_model_get_memory_usage(void *model_handle, size_t *memory_usage);

/* ============================================================================
 * MODEL CONSTANTS
 * ============================================================================ */

/* Model input/output sizes for different model types */
#define ANOMALY_DETECTOR_INPUT_SIZE 262
#define ANOMALY_DETECTOR_OUTPUT_SIZE 1

#define ATTACK_PATTERN_INPUT_SIZE 128
#define ATTACK_PATTERN_OUTPUT_SIZE 1

#define GNN_MAX_NODES 100
#define GNN_NODE_FEATURES 15
#define GNN_EDGE_FEATURES 3
#define GNN_EMBEDDING_SIZE 16

#define RL_STATE_SIZE 32
#define RL_NUM_ACTIONS 20

#define TEXT_CLASSIFIER_MAX_TOKENS 512
#define TEXT_CLASSIFIER_NUM_CLASSES 6

#define NER_MAX_TOKENS 512

#define GAN_NOISE_SIZE 128
#define GAN_OUTPUT_SIZE 784  /* 28x28 image */

/* Device IDs */
#define DEVICE_51_ENHANCED_SECURITY 51
#define DEVICE_52_AI_ACCELERATOR 52
#define DEVICE_53_QUANTUM_PROCESSOR 53
#define DEVICE_54_NEURAL_PROCESSOR 54
#define DEVICE_55_GRAPH_PROCESSOR 55
#define DEVICE_56_RL_PROCESSOR 56
#define DEVICE_57_NLP_PROCESSOR 57
#define DEVICE_58_VISION_PROCESSOR 58

#endif /* DSMIL_MODEL_APIS_H */
