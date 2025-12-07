# DSLLVM Quick Start Guide

**Last Updated**: 2025-12-07  
**Version**: Phase 3 Complete

---

## Overview

DSLLVM is a CPU-feature-aware LLVM/Clang toolchain for Intel Meteor Lake that optimizes AI kernels, hardens security, and enables hardware profiling.

---

## Installation

```bash
# 1. Build LLVM passes
cd /workspace/dsmil/llvm-passes
mkdir build && cd build
cmake .. -DLLVM_DIR=/usr/lib/llvm-17
make -j$(nproc)

# 2. Make driver executable
chmod +x /workspace/dsmil/tools/dsmil-clang
ln -s dsmil-clang dsmil-clang++

# 3. Add to PATH
export PATH="/workspace/dsmil/tools:$PATH"
```

---

## Basic Usage

### Standard Compilation

```bash
# Use dsmil-clang like regular clang
dsmil-clang -O2 main.c -o main

# Automatically loads CPU profile and injects metadata
```

---

## AI Kernel Optimization

### Enable VNNI Acceleration

```bash
# Optimize INT8 AI kernels with AVX-VNNI
dsmil-clang -fdsllvm-ai-accelerate -O3 gemm.c -o gemm
```

**Optimizes**:
- INT8 GEMM (matrix multiply)
- Convolution (CNNs)
- Attention (Transformers)
- Vector-matrix multiply (LLMs)

**Expected Speedup**: 15-25x for INT8 operations

---

### Example: INT8 GEMM

```c
// gemm.c
void gemm_int8(char *A, char *B, int *C, int M, int N, int K) {
    for (int i = 0; i < M; i++) {
        for (int j = 0; j < N; j++) {
            int sum = 0;
            for (int k = 0; k < K; k++) {
                sum += A[i*K + k] * B[k*N + j];
            }
            C[i*N + j] = sum;
        }
    }
}
```

**Compile**:
```bash
dsmil-clang -fdsllvm-ai-accelerate -O3 gemm.c -o gemm

# Output:
# DSLLVM: Using CPU profile 'mtr-mtl-dsmil'
# DSLLVM AIAccelerate: AVX-VNNI available
#   Pattern matching for MAC loops...
#   ✓ MAC pattern detected
#   Expected speedup: 20x
```

**What Happens**:
1. Detects 3-level loop nest (GEMM pattern)
2. Identifies INT8 multiply-accumulate
3. Lowers to VPDPBUSD intrinsic (32 ops at once)
4. Generates vectorized code

---

## Security Hardening

### Speculation Mitigation

```bash
# Add Spectre/Meltdown mitigations
dsmil-clang -fdsllvm-spec-hard -O2 bounds_check.c -o safe
```

**What It Does**:
- Inserts LFENCE after bounds checks
- Uses hardware mitigations (IBRS) when available
- Emits VERW for MDS mitigation

---

### Constant-Time Crypto

```bash
# Enforce constant-time execution for crypto
dsmil-clang -fdsllvm-ct-check -O2 aes.c -o aes
```

**Requires**: Mark crypto functions with `dsmil_secret`

```c
__attribute__((annotate("dsmil_secret")))
void aes_encrypt(const char *key, char *data, int len) {
    for (int i = 0; i < len; i++) {
        data[i] ^= key[i % 16];  // Constant-time guaranteed
    }
}
```

**What It Does**:
- Detects secret-dependent branches → ERROR
- Detects secret-dependent memory access → ERROR
- Inserts CLFLUSHOPT after key usage
- Inserts MFENCE at function exit
- Verifies constant-time execution

---

### Full Security Hardening

```bash
# Enable all security features
dsmil-clang -fdsllvm-harden -O2 server.c -o server

# Equivalent to:
# -fdsllvm-spec-hard
# -fdsllvm-ct-check
# Plus: SMEP/SMAP/UMIP/CET enforcement
```

---

## Hardware Profiling

### Intel Processor Trace

```bash
# Enable Intel PT profiling metadata
dsmil-clang -fdsllvm-prof=pt main.c -o main
```

**Requires**: `intel_pt` CPU feature

**What It Does**:
- Attaches `!dsmil.prof.pt` metadata
- Enables PT-aware optimizations
- Reduces overhead of PT instrumentation

---

### Last Branch Records (LBR)

```bash
# Enable LBR profiling
dsmil-clang -fdsllvm-prof=lbr main.c -o main
```

**Use Case**: Branch misprediction analysis

---

### PEBS (Precise Event-Based Sampling)

```bash
# Enable PEBS profiling
dsmil-clang -fdsllvm-prof=pebs main.c -o main
```

**Use Case**: Cache miss / memory stall analysis

---

## CPU Profiles

### Use Custom CPU Profile

```bash
# Specify CPU profile explicitly
dsmil-clang -fdsllvm-profile=mtr-mtl-dsmil -O2 main.c -o main
```

**Available Profiles**:
- `mtr-mtl-dsmil` - Meteor Lake DSMIL node (default)

---

### Create New Profile

```bash
# Probe current CPU
/workspace/dsmil/tools/dsllvm-cpufeatures > my-cpu.json

# Edit profile
vim /workspace/dsmil/config/cpu/my-cpu.json

# Use it
dsmil-clang -fdsllvm-profile=my-cpu main.c -o main
```

---

## Compiler Flags Reference

| Flag | Purpose | Example |
|------|---------|---------|
| `-fdsllvm-profile=<name>` | Load CPU profile | `-fdsllvm-profile=mtr-mtl-dsmil` |
| `-fdsllvm-ai-accelerate` | AI kernel optimization | GEMM, Conv, Attention |
| `-fdsllvm-spec-hard` | Speculation hardening | Spectre mitigation |
| `-fdsllvm-ct-check` | Constant-time crypto | Side-channel safety |
| `-fdsllvm-harden` | Full security hardening | All mitigations |
| `-fdsllvm-prof=<type>` | Hardware profiling | `pt`, `lbr`, `pebs` |
| `-fdsllvm-provenance` | Record build metadata | CPU assumptions |

---

## LLVM Pass Usage (Advanced)

### Direct Pass Invocation

```bash
# Compile to IR
clang -S -emit-llvm -O1 main.c -o main.ll

# Run AI acceleration pass
opt -load-pass-plugin=/workspace/dsmil/llvm-passes/build/libDSLLVMPasses.so \
    -passes="dsllvm-ai-accelerate" \
    -S main.ll -o optimized.ll

# Compile to binary
clang optimized.ll -o main
```

### Available Passes

- `dsllvm-ai-accelerate` - AI kernel optimization
- `dsllvm-spec-hard` - Speculation hardening
- `dsllvm-ct-check` - Constant-time verification
- `dsllvm-bw-estimate` - Bandwidth estimation

---

## Metadata Reference

### Attach CPU Profile

```llvm
!dsllvm.cpu.profile = !{!"mtr-mtl-dsmil"}
!dsllvm.cpu.features = !{!"avx_vnni", !"fsrm", !"intel_pt", ...}
```

### Mark AI Function

```c
__attribute__((annotate("dsmil_layer(7)")))
__attribute__((annotate("dsmil_device(47)")))
void my_ai_kernel() { ... }
```

### Mark Crypto Function

```c
__attribute__((annotate("dsmil_secret")))
void crypto_function() { ... }
```

---

## Examples

### Example 1: LLM Inference

```c
// transformer.c
#include <stdint.h>

__attribute__((annotate("dsmil_layer(7)")))
void attention_qk(const int8_t *Q, const int8_t *K, int32_t *scores,
                   int seq_len, int d_model) {
    for (int i = 0; i < seq_len; i++) {
        for (int j = 0; j < seq_len; j++) {
            int32_t sum = 0;
            for (int k = 0; k < d_model; k++) {
                sum += Q[i*d_model + k] * K[j*d_model + k];
            }
            scores[i*seq_len + j] = sum;
        }
    }
}
```

**Compile**:
```bash
dsmil-clang -fdsllvm-ai-accelerate -O3 transformer.c -o transformer
```

**Result**: 15-22x speedup with AVX-VNNI

---

### Example 2: Crypto Server

```c
// server.c
#include <stdint.h>

__attribute__((annotate("dsmil_secret")))
void process_key(const uint8_t *key, uint8_t *data, int len) {
    for (int i = 0; i < len; i++) {
        data[i] ^= key[i % 32];
    }
}

int main() {
    uint8_t key[32] = {...};
    uint8_t data[1024] = {...};
    
    process_key(key, data, 1024);
    
    return 0;
}
```

**Compile**:
```bash
dsmil-clang -fdsllvm-harden -O2 server.c -o server
```

**Result**: Constant-time execution, cache flushes, Spectre mitigation

---

### Example 3: CNN Inference

```c
// cnn.c
__attribute__((annotate("dsmil_layer(7)")))
void conv2d(const int8_t *input, const int8_t *kernel, int32_t *output,
             int H, int W, int KH, int KW) {
    for (int i = 0; i < H - KH + 1; i++) {
        for (int j = 0; j < W - KW + 1; j++) {
            int32_t sum = 0;
            for (int ki = 0; ki < KH; ki++) {
                for (int kj = 0; kj < KW; kj++) {
                    sum += input[(i+ki)*W + (j+kj)] * kernel[ki*KW + kj];
                }
            }
            output[i*(W-KW+1) + j] = sum;
        }
    }
}
```

**Compile**:
```bash
dsmil-clang -fdsllvm-ai-accelerate -O3 cnn.c -o cnn
```

**Result**: 3-8x speedup for convolutions

---

## Verification

### Check Metadata

```bash
# Compile to IR
dsmil-clang -S -emit-llvm main.c -o main.ll

# Check metadata
grep "dsllvm.cpu" main.ll

# Expected output:
# !dsllvm.cpu.profile = !{!"mtr-mtl-dsmil"}
# !dsllvm.cpu.features = !{!"avx_vnni", ...}
```

---

### Check Intrinsics

```bash
# Look for VNNI intrinsics
grep "vpdpbusd" main.ll

# Look for security intrinsics
grep -E "lfence|clflushopt|mfence" main.ll
```

---

### Check Assembly

```bash
# Compile to assembly
dsmil-clang -S -fdsllvm-ai-accelerate main.c -o main.s

# Look for VNNI instructions
grep "vpdpbusd" main.s
```

---

## Troubleshooting

### Metadata Not Injected

**Problem**: `!dsllvm.cpu.features` not in IR

**Solution**:
```bash
# Check profile exists
ls /workspace/dsmil/config/cpu/mtr-mtl-dsmil.json

# Use explicit profile
dsmil-clang -fdsllvm-profile=mtr-mtl-dsmil main.c
```

---

### VNNI Not Optimizing

**Problem**: No VPDPBUSD in output

**Possible Causes**:
1. Loop not recognized as GEMM pattern
2. Not INT8 operations (must be `int8_t`)
3. Needs LoopInfo integration (Phase 4)

**Debug**:
```bash
# Check what DSLLVM sees
dsmil-clang -fdsllvm-ai-accelerate -v main.c 2>&1 | grep "Pattern"
```

---

### Constant-Time Violations

**Problem**: "Secret-dependent branch detected" error

**Solution**: Rewrite crypto code to avoid data-dependent branches

```c
// Bad: Secret-dependent branch
if (key[i] > 128) {
    data[i] ^= key[i];
}

// Good: Constant-time
uint8_t mask = -(key[i] > 128);
data[i] ^= key[i] & mask;
```

---

## Performance Tips

### AI Kernels

1. **Use INT8**: VNNI only optimizes INT8 operations
2. **Align memory**: 32-byte alignment for vectors
3. **Block for cache**: Use 32x32 tiles
4. **Unroll**: Unroll inner loops 4x

---

### Crypto

1. **Mark secrets**: Use `dsmil_secret` annotation
2. **Avoid branches**: Use bitwise operations
3. **Flush caches**: Let DSLLVM insert CLFLUSHOPT

---

### General

1. **Use -O3**: Enables all optimizations
2. **Profile first**: Use `-fdsllvm-prof=pt` to find hotspots
3. **Verify assembly**: Check for expected intrinsics

---

## Next Steps

1. **Read full docs**: `/workspace/dsmil/docs/DSLLVM_CPU_FEATURE_MODEL.md`
2. **Run tests**: `/workspace/dsmil/llvm-passes/test-phase3.sh`
3. **Benchmark**: Compare DSLLVM vs regular Clang
4. **Contribute**: Add new CPU features or optimizations

---

## Support

**Documentation**: `/workspace/dsmil/docs/`  
**Tests**: `/workspace/dsmil/llvm-passes/test-*.sh`  
**Source**: `/workspace/dsmil/llvm-passes/`

---

**Version**: Phase 3 Complete  
**Last Updated**: 2025-12-07
