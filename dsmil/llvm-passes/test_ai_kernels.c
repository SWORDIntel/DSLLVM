// test_ai_kernels.c - AI kernel test cases for VNNI optimization
//
// This file contains realistic AI kernels that should be optimized
// with AVX-VNNI (VPDPBUSD) intrinsics.

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// =============================================================================
// INT8 GEMM Kernel (Core AI Operation)
// =============================================================================

// Matrix multiply: C = A @ B
// A: M x K (INT8)
// B: K x N (INT8)
// C: M x N (INT32)
__attribute__((annotate("dsmil_layer(7)")))
__attribute__((annotate("dsmil_device(47)")))
void gemm_int8(const int8_t *A, const int8_t *B, int32_t *C,
                int M, int N, int K) {
    // Classic 3-loop GEMM: perfect candidate for VNNI
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            int32_t sum = 0;
            for (int k = 0; k < K; k++) {
                // MAC pattern: sum += A[i,k] * B[k,j]
                sum += (int32_t)A[i * K + k] * (int32_t)B[k * N + j];
            }
            C[i * N + j] = sum;
        }
    }
}

// Optimized GEMM with blocking (better cache utilization)
__attribute__((annotate("dsmil_layer(7)")))
void gemm_int8_blocked(const int8_t *A, const int8_t *B, int32_t *C,
                        int M, int N, int K) {
    const int BLOCK_SIZE = 32; // Matches VNNI vector width
    
    for (int i = 0; i < M; i += BLOCK_SIZE) {
        for (int j = 0; j < N; j += BLOCK_SIZE) {
            for (int k = 0; k < K; k += BLOCK_SIZE) {
                // Process block
                int i_end = (i + BLOCK_SIZE < M) ? i + BLOCK_SIZE : M;
                int j_end = (j + BLOCK_SIZE < N) ? j + BLOCK_SIZE : N;
                int k_end = (k + BLOCK_SIZE < K) ? k + BLOCK_SIZE : K;
                
                for (int ii = i; ii < i_end; ii++) {
                    for (int jj = j; jj < j_end; jj++) {
                        int32_t sum = C[ii * N + jj];
                        for (int kk = k; kk < k_end; kk++) {
                            sum += (int32_t)A[ii * K + kk] * (int32_t)B[kk * N + jj];
                        }
                        C[ii * N + jj] = sum;
                    }
                }
            }
        }
    }
}

// =============================================================================
// INT8 Convolution (Used in CNNs)
// =============================================================================

// 2D Convolution: output = conv(input, kernel)
__attribute__((annotate("dsmil_layer(7)")))
void conv2d_int8(const int8_t *input, const int8_t *kernel, int32_t *output,
                  int H, int W, int KH, int KW) {
    int out_h = H - KH + 1;
    int out_w = W - KW + 1;
    
    for (int i = 0; i < out_h; i++) {
        for (int j = 0; j < out_w; j++) {
            int32_t sum = 0;
            
            // Kernel sliding window
            for (int ki = 0; ki < KH; ki++) {
                for (int kj = 0; kj < KW; kj++) {
                    int input_i = i + ki;
                    int input_j = j + kj;
                    
                    // MAC pattern (can be optimized with VNNI)
                    sum += (int32_t)input[input_i * W + input_j] * 
                           (int32_t)kernel[ki * KW + kj];
                }
            }
            
            output[i * out_w + j] = sum;
        }
    }
}

// Depthwise convolution (efficient variant)
__attribute__((annotate("dsmil_layer(7)")))
void depthwise_conv_int8(const int8_t *input, const int8_t *kernel, 
                          int32_t *output, int H, int W, int C, int KH, int KW) {
    int out_h = H - KH + 1;
    int out_w = W - KW + 1;
    
    for (int c = 0; c < C; c++) {
        for (int i = 0; i < out_h; i++) {
            for (int j = 0; j < out_w; j++) {
                int32_t sum = 0;
                
                for (int ki = 0; ki < KH; ki++) {
                    for (int kj = 0; kj < KW; kj++) {
                        int input_idx = ((i + ki) * W + (j + kj)) * C + c;
                        int kernel_idx = (ki * KW + kj) * C + c;
                        sum += (int32_t)input[input_idx] * (int32_t)kernel[kernel_idx];
                    }
                }
                
                output[(i * out_w + j) * C + c] = sum;
            }
        }
    }
}

// =============================================================================
// INT8 Attention Mechanism (Transformer)
// =============================================================================

// Simplified attention: Attention(Q, K, V) = softmax(Q @ K^T) @ V
// For VNNI demo, we just do Q @ K^T (GEMM)
__attribute__((annotate("dsmil_layer(7)")))
void attention_qk_int8(const int8_t *Q, const int8_t *K, int32_t *scores,
                        int seq_len, int d_model) {
    // Q @ K^T: (seq_len, d_model) @ (d_model, seq_len) -> (seq_len, seq_len)
    
    for (int i = 0; i < seq_len; i++) {
        for (int j = 0; j < seq_len; j++) {
            int32_t sum = 0;
            
            for (int k = 0; k < d_model; k++) {
                // Q[i,k] @ K^T[k,j] = Q[i,k] @ K[j,k]
                sum += (int32_t)Q[i * d_model + k] * (int32_t)K[j * d_model + k];
            }
            
            scores[i * seq_len + j] = sum;
        }
    }
}

// =============================================================================
// Vector-Matrix Multiply (Common in LLMs)
// =============================================================================

// y = x @ W (vector-matrix multiply)
__attribute__((annotate("dsmil_layer(7)")))
void matvec_int8(const int8_t *x, const int8_t *W, int32_t *y,
                  int in_features, int out_features) {
    for (int i = 0; i < out_features; i++) {
        int32_t sum = 0;
        
        for (int j = 0; j < in_features; j++) {
            sum += (int32_t)x[j] * (int32_t)W[i * in_features + j];
        }
        
        y[i] = sum;
    }
}

// Batched matrix-vector multiply
__attribute__((annotate("dsmil_layer(7)")))
void batched_matvec_int8(const int8_t *X, const int8_t *W, int32_t *Y,
                          int batch_size, int in_features, int out_features) {
    for (int b = 0; b < batch_size; b++) {
        const int8_t *x = X + b * in_features;
        int32_t *y = Y + b * out_features;
        
        matvec_int8(x, W, y, in_features, out_features);
    }
}

// =============================================================================
// Test/Benchmark Functions
// =============================================================================

void benchmark_gemm(int M, int N, int K) {
    int8_t *A = malloc(M * K);
    int8_t *B = malloc(K * N);
    int32_t *C = calloc(M * N, sizeof(int32_t));
    
    // Initialize with random data
    for (int i = 0; i < M * K; i++) A[i] = (i % 127) - 64;
    for (int i = 0; i < K * N; i++) B[i] = (i % 127) - 64;
    
    printf("Benchmark: GEMM INT8 (%d x %d x %d)\n", M, N, K);
    
    // Run GEMM
    gemm_int8(A, B, C, M, N, K);
    
    // Check result
    printf("  Result: C[0]=%d, C[%d]=%d\n", C[0], M*N-1, C[M*N-1]);
    
    free(A);
    free(B);
    free(C);
}

void benchmark_conv(int H, int W, int KH, int KW) {
    int8_t *input = malloc(H * W);
    int8_t *kernel = malloc(KH * KW);
    int32_t *output = calloc((H - KH + 1) * (W - KW + 1), sizeof(int32_t));
    
    // Initialize
    for (int i = 0; i < H * W; i++) input[i] = (i % 127) - 64;
    for (int i = 0; i < KH * KW; i++) kernel[i] = (i % 9) - 4;
    
    printf("Benchmark: Conv2D INT8 (%dx%d input, %dx%d kernel)\n", H, W, KH, KW);
    
    // Run convolution
    conv2d_int8(input, kernel, output, H, W, KH, KW);
    
    printf("  Result: output[0]=%d\n", output[0]);
    
    free(input);
    free(kernel);
    free(output);
}

int main() {
    printf("==========================================================\n");
    printf("DSLLVM Phase 3: AI Kernel Tests (AVX-VNNI Optimization)\n");
    printf("==========================================================\n\n");
    
    // Small tests (for correctness)
    printf("Small tests (4x4):\n");
    benchmark_gemm(4, 4, 4);
    printf("\n");
    
    // Medium tests (typical AI workload)
    printf("Medium tests (32x32, 64x64):\n");
    benchmark_gemm(32, 32, 32);
    benchmark_gemm(64, 64, 64);
    printf("\n");
    
    // Convolution tests
    printf("Convolution tests:\n");
    benchmark_conv(28, 28, 3, 3);  // MNIST-like
    benchmark_conv(224, 224, 7, 7); // ImageNet first layer
    printf("\n");
    
    // Large GEMM (LLM-like)
    printf("Large GEMM (LLM inference):\n");
    benchmark_gemm(128, 4096, 4096); // Typical transformer layer
    printf("\n");
    
    printf("==========================================================\n");
    printf("All tests complete!\n");
    printf("Compile with: dsmil-clang -fdsllvm-ai-accelerate -O3\n");
    printf("Expected: VPDPBUSD intrinsics in generated code\n");
    printf("==========================================================\n");
    
    return 0;
}
