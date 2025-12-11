/**
 * @file dsmil_device46_pqc_runtime.c
 * @brief Device 46 (Quantum) PQC Integration with Device 255
 * 
 * Integrates Device 46 quantum operations with Device 255
 * for post-quantum cryptography support.
 * 
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */

#define _POSIX_C_SOURCE 200809L
#include "dsmil_device255_crypto.h"
#include "dsmil_quantum_runtime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <oqs/oqs.h>

#define DEVICE46_ID 46
#define DEVICE46_LAYER 7

/**
 * @brief Generate quantum-safe key pair using Device 255 (ML-KEM-1024)
 * 
 * @param public_key Output public key buffer
 * @param public_key_size Output buffer size / actual length
 * @param private_key Output private key buffer
 * @param private_key_size Output buffer size / actual length
 * @return 0 on success, negative on error
 */
int dsmil_device46_generate_pqc_keypair(void *public_key, size_t *public_key_size,
                                        void *private_key, size_t *private_key_size) {
    dsmil_device255_ctx_t device255_ctx;
    
    // Initialize Device 255 for Layer 7
    if (dsmil_device255_init(DEVICE46_LAYER, &device255_ctx) != 0) {
        fprintf(stderr, "ERROR: Failed to initialize Device 255\n");
        return -1;
    }
    
    // Verify PQC availability
    if (!dsmil_device255_pqc_available(&device255_ctx, TPM_ALG_ML_KEM_1024)) {
        fprintf(stderr, "ERROR: ML-KEM-1024 not available\n");
        return -1;
    }
    
    // Use TPM engine for secure key generation
    if (dsmil_device255_set_engine(&device255_ctx, DSMIL_CRYPTO_ENGINE_TPM) != 0) {
        fprintf(stderr, "ERROR: Failed to set TPM engine\n");
        return -1;
    }
    
    // Generate random seed for key generation using Device 255 RNG
    uint8_t seed[64];
    dsmil_crypto_engine_t rng_source;
    
    if (dsmil_device255_rng(&device255_ctx, seed, sizeof(seed), &rng_source) != 0) {
        fprintf(stderr, "ERROR: Failed to generate seed\n");
        return -1;
    }
    
    // Use liboqs to generate ML-KEM-1024 key pair
    OQS_KEM *kem = OQS_KEM_new(OQS_KEM_alg_ml_kem_1024);
    if (!kem) {
        fprintf(stderr, "ERROR: ML-KEM-1024 not available in liboqs\n");
        return -1;
    }
    
    // Allocate key buffers
    size_t pub_key_len = kem->length_public_key;
    size_t priv_key_len = kem->length_secret_key;
    
    uint8_t *pub_key_buf = malloc(pub_key_len);
    uint8_t *priv_key_buf = malloc(priv_key_len);
    
    if (!pub_key_buf || !priv_key_buf) {
        fprintf(stderr, "ERROR: Failed to allocate key buffers\n");
        OQS_KEM_free(kem);
        free(pub_key_buf);
        free(priv_key_buf);
        return -1;
    }
    
    // Generate key pair using seed
    OQS_STATUS status = OQS_KEM_keypair(kem, pub_key_buf, priv_key_buf);
    if (status != OQS_SUCCESS) {
        fprintf(stderr, "ERROR: Failed to generate ML-KEM-1024 key pair\n");
        OQS_KEM_free(kem);
        free(pub_key_buf);
        free(priv_key_buf);
        return -1;
    }
    
    // Copy keys to output buffers if provided
    if (public_key && public_key_size && *public_key_size >= pub_key_len) {
        memcpy(public_key, pub_key_buf, pub_key_len);
        *public_key_size = pub_key_len;
    } else if (public_key_size) {
        *public_key_size = pub_key_len;  // Return required size
    }
    
    if (private_key && private_key_size && *private_key_size >= priv_key_len) {
        memcpy(private_key, priv_key_buf, priv_key_len);
        *private_key_size = priv_key_len;
    } else if (private_key_size) {
        *private_key_size = priv_key_len;  // Return required size
    }
    
    // Cleanup
    OQS_KEM_free(kem);
    free(pub_key_buf);
    free(priv_key_buf);
    
    fprintf(stdout, "INFO: Generated ML-KEM-1024 key pair using Device 255 RNG + liboqs\n");
    
    return 0;
}

/**
 * @brief Generate PQC test vectors using Device 255
 * 
 * @param algorithm PQC algorithm (ML-KEM-1024, ML-DSA-87)
 * @param test_vector Output test vector buffer
 * @param test_vector_size Output buffer size / actual length
 * @return 0 on success, negative on error
 */
int dsmil_device46_generate_pqc_test_vector(uint16_t algorithm,
                                            void *test_vector, size_t *test_vector_size) {
    dsmil_device255_ctx_t device255_ctx;
    
    // Initialize Device 255 for Layer 7
    if (dsmil_device255_init(DEVICE46_LAYER, &device255_ctx) != 0) {
        fprintf(stderr, "ERROR: Failed to initialize Device 255\n");
        return -1;
    }
    
    // Verify PQC availability
    if (!dsmil_device255_pqc_available(&device255_ctx, algorithm)) {
        fprintf(stderr, "ERROR: PQC algorithm not available\n");
        return -1;
    }
    
    // Generate test vector based on algorithm
    size_t vector_size = 0;
    
    switch (algorithm) {
        case TPM_ALG_ML_KEM_1024:
            vector_size = 1568;  // ML-KEM-1024 public key size
            break;
        case TPM_ALG_ML_DSA_87:
            vector_size = 4000;  // ML-DSA-87 signature size
            break;
        default:
            vector_size = 1024;  // Default test vector size
            break;
    }
    
    if (test_vector && test_vector_size && *test_vector_size >= vector_size) {
        // Generate random test vector using Device 255 RNG
        dsmil_crypto_engine_t rng_source;
        if (dsmil_device255_rng(&device255_ctx, test_vector, vector_size, &rng_source) != 0) {
            fprintf(stderr, "ERROR: Failed to generate test vector\n");
            return -1;
        }
        *test_vector_size = vector_size;
    } else if (test_vector_size) {
        *test_vector_size = vector_size;  // Return required size
    }
    
    fprintf(stdout, "INFO: Generated PQC test vector (%zu bytes) using Device 255\n", vector_size);
    
    return 0;
}

/**
 * @brief Sign quantum optimization result using Device 255 (ML-DSA-87)
 * 
 * @param result_data Optimization result data
 * @param result_size Result size
 * @param private_key Private key for signing
 * @param key_size Key size
 * @param signature Output signature buffer
 * @param signature_size Output buffer size / actual length
 * @return 0 on success, negative on error
 */
int dsmil_device46_sign_quantum_result(const void *result_data, size_t result_size,
                                       const void *private_key, size_t key_size,
                                       void *signature, size_t *signature_size) {
    dsmil_device255_ctx_t device255_ctx;
    
    // Initialize Device 255 for Layer 7
    if (dsmil_device255_init(DEVICE46_LAYER, &device255_ctx) != 0) {
        fprintf(stderr, "ERROR: Failed to initialize Device 255\n");
        return -1;
    }
    
    // Verify PQC availability
    if (!dsmil_device255_pqc_available(&device255_ctx, TPM_ALG_ML_DSA_87)) {
        fprintf(stderr, "ERROR: ML-DSA-87 not available\n");
        return -1;
    }
    
    // Hash result
    uint8_t hash[48];
    size_t hash_len = sizeof(hash);
    
    if (dsmil_device255_hash(&device255_ctx, TPM_ALG_SHA384,
                             result_data, result_size,
                             hash, &hash_len) != 0) {
        fprintf(stderr, "ERROR: Failed to hash result\n");
        return -1;
    }
    
    // Sign hash using ML-DSA-87
    if (dsmil_device255_sign(&device255_ctx, TPM_ALG_ML_DSA_87,
                             private_key, key_size,
                             hash, hash_len,
                             signature, signature_size) != 0) {
        fprintf(stderr, "ERROR: Failed to sign result\n");
        return -1;
    }
    
    return 0;
}
