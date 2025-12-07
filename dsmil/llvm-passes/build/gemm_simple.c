#include <stdint.h>

void gemm_4x4(int8_t *A, int8_t *B, int32_t *C) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int32_t sum = 0;
            for (int k = 0; k < 4; k++) {
                sum += (int32_t)A[i*4 + k] * (int32_t)B[k*4 + j];
            }
            C[i*4 + j] = sum;
        }
    }
}
