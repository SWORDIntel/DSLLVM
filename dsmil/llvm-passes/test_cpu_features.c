// test_cpu_features.c - Test program for DSLLVM CPU feature integration
//
// This program exercises various CPU features and allows testing of
// DSLLVM passes that use CPU feature detection.

#include <stdint.h>
#include <string.h>
#include <stdio.h>

// Annotate functions with DSMIL attributes (would normally come from dsmil_attributes.h)
#define DSMIL_LAYER(n) __attribute__((annotate("dsmil_layer(" #n ")")))
#define DSMIL_DEVICE(n) __attribute__((annotate("dsmil_device(" #n ")")))
#define DSMIL_SECRET __attribute__((annotate("dsmil_secret")))

// =============================================================================
// AI Kernel Tests (for DsmilAIAccelerate)
// =============================================================================

// INT8 GEMM kernel - should be optimized with AVX-VNNI
DSMIL_LAYER(7) DSMIL_DEVICE(47)
void gemm_int8(int8_t *A, int8_t *B, int32_t *C, int M, int N, int K) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            int32_t sum = 0;
            for (int k = 0; k < K; k++) {
                sum += (int32_t)A[i * K + k] * (int32_t)B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
}

// 2D Convolution kernel
DSMIL_LAYER(7) DSMIL_DEVICE(47)
void conv2d_int8(int8_t *input, int8_t *kernel, int32_t *output,
                  int H, int W, int KH, int KW) {
    for (int i = 0; i < H - KH + 1; i++) {
        for (int j = 0; j < W - KW + 1; j++) {
            int32_t sum = 0;
            for (int ki = 0; ki < KH; ki++) {
                for (int kj = 0; kj < KW; kj++) {
                    sum += (int32_t)input[(i + ki) * W + (j + kj)] * 
                           (int32_t)kernel[ki * KW + kj];
                }
            }
            output[i * (W - KW + 1) + j] = sum;
        }
    }
}

// Attention mechanism (simplified)
DSMIL_LAYER(7) DSMIL_DEVICE(47)
void attention_qkv(float *Q, float *K, float *V, float *output,
                    int seq_len, int d_model) {
    // Simplified attention: output = softmax(Q @ K^T) @ V
    // Real implementation would be more complex
    for (int i = 0; i < seq_len; i++) {
        for (int j = 0; j < d_model; j++) {
            float sum = 0.0f;
            for (int k = 0; k < seq_len; k++) {
                sum += Q[i * d_model + k] * V[k * d_model + j];
            }
            output[i * d_model + j] = sum;
        }
    }
}

// =============================================================================
// Memory Bandwidth Tests (for DsmilBandwidthEstimate)
// =============================================================================

// Large memcpy (should use ERMS if available)
DSMIL_LAYER(7) DSMIL_DEVICE(47)
void large_memcpy(void *dst, const void *src, size_t n) {
    // memcpy would normally use REP MOVSB with ERMS
    memcpy(dst, src, n);
}

// Small memcpy (should use FSRM if available)
DSMIL_LAYER(7) DSMIL_DEVICE(47)
void small_memcpy(void *dst, const void *src) {
    // Small copy (< 256 bytes) optimized with FSRM
    memcpy(dst, src, 128);
}

// KV cache operations (large sequential memory access)
DSMIL_LAYER(7) DSMIL_DEVICE(47)
void kv_cache_update(float *kv_cache, const float *new_kv, 
                      int cache_size, int update_size) {
    // Sequential write to KV cache
    for (int i = 0; i < update_size; i++) {
        kv_cache[cache_size - update_size + i] = new_kv[i];
    }
}

// =============================================================================
// Crypto Tests (for DsmilConstantTimeCheck)
// =============================================================================

// AES-like encryption (should be constant-time)
DSMIL_LAYER(8) DSMIL_DEVICE(80) DSMIL_SECRET
void aes_encrypt(const uint8_t *key, const uint8_t *plaintext, 
                  uint8_t *ciphertext, int len) {
    // Simplified AES-like operation
    // Real implementation would use AES-NI when available
    for (int i = 0; i < len; i++) {
        ciphertext[i] = plaintext[i] ^ key[i % 16];
    }
    
    // In real constant-time code, we'd need to:
    // 1. Avoid secret-dependent branches
    // 2. Flush cache after operations on key material
    // 3. Use constant-time table lookups
}

// HMAC-like operation
DSMIL_LAYER(8) DSMIL_DEVICE(80) DSMIL_SECRET
void hmac_compute(const uint8_t *key, const uint8_t *message, 
                   uint8_t *mac, int msg_len) {
    // Simplified HMAC (would use SHA-NI if available)
    uint32_t hash = 0;
    
    for (int i = 0; i < msg_len; i++) {
        hash ^= (uint32_t)message[i] * (uint32_t)key[i % 32];
        hash = (hash << 5) | (hash >> 27);  // rotate
    }
    
    *(uint32_t*)mac = hash;
}

// Constant-time comparison (for signature verification)
DSMIL_LAYER(8) DSMIL_DEVICE(80) DSMIL_SECRET
int crypto_compare(const uint8_t *a, const uint8_t *b, size_t len) {
    uint8_t diff = 0;
    
    // Constant-time compare: always check all bytes
    for (size_t i = 0; i < len; i++) {
        diff |= a[i] ^ b[i];
    }
    
    return diff;  // 0 if equal, non-zero otherwise
}

// =============================================================================
// Speculation Hazard Tests (for DsmilSpecHardening)
// =============================================================================

// Bounds check (Spectre v1 vulnerable)
DSMIL_LAYER(8) DSMIL_DEVICE(80)
uint8_t array_access(uint8_t *array, size_t array_len, size_t index) {
    // Bounds check - needs LFENCE after check
    if (index < array_len) {
        return array[index];
    }
    return 0;
}

// Indirect branch (Spectre v2 vulnerable)
DSMIL_LAYER(8) DSMIL_DEVICE(80)
void indirect_call(void (*func_table[])(void), int index) {
    // Indirect call - needs IBRS or retpoline
    func_table[index]();
}

// Speculative load (Spectre v4 / SSB vulnerable)
DSMIL_LAYER(8) DSMIL_DEVICE(80)
uint64_t speculative_load_chain(uint64_t *data, size_t index) {
    // Load chain that could be speculated
    uint64_t val = data[index];
    return data[val & 0xFF];  // second load depends on first
}

// =============================================================================
// Main (Test Driver)
// =============================================================================

int main(void) {
    printf("DSLLVM CPU Feature Integration Test\n");
    printf("====================================\n\n");
    
    // AI kernels
    printf("Testing AI kernels...\n");
    int8_t A[4*4], B[4*4];
    int32_t C[4*4];
    gemm_int8(A, B, C, 4, 4, 4);
    printf("  GEMM INT8: OK\n");
    
    int8_t input[16*16], kernel[3*3];
    int32_t output[14*14];
    conv2d_int8(input, kernel, output, 16, 16, 3, 3);
    printf("  Conv2D: OK\n");
    
    // Memory bandwidth
    printf("\nTesting memory operations...\n");
    uint8_t large_src[1024*1024], large_dst[1024*1024];
    large_memcpy(large_dst, large_src, sizeof(large_src));
    printf("  Large memcpy: OK (should use ERMS)\n");
    
    uint8_t small_src[128], small_dst[128];
    small_memcpy(small_dst, small_src);
    printf("  Small memcpy: OK (should use FSRM)\n");
    
    // Crypto
    printf("\nTesting crypto operations...\n");
    uint8_t key[16], plaintext[16], ciphertext[16];
    aes_encrypt(key, plaintext, ciphertext, 16);
    printf("  AES encrypt: OK (should be constant-time)\n");
    
    uint8_t message[32], mac[4];
    hmac_compute(key, message, mac, 32);
    printf("  HMAC compute: OK (should use SHA-NI if available)\n");
    
    // Speculation hazards
    printf("\nTesting speculation hazards...\n");
    uint8_t array[256];
    uint8_t val = array_access(array, 256, 42);
    printf("  Array access: OK (should insert LFENCE)\n");
    
    printf("\nAll tests completed!\n");
    printf("Run DSLLVM passes to see optimization/hardening applied.\n");
    
    return 0;
}
