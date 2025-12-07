# DSLLVM CPU Feature Integration Passes

This directory contains LLVM optimization passes that leverage CPU features for the DSLLVM toolchain.

## Overview

The DSLLVM CPU feature model treats hardware features as **first-class inputs** to optimization decisions. These passes query CPU features from module metadata (`!dsllvm.cpu.features`) and adjust their behavior accordingly.

## Passes

### 1. DsmilCPUFeatures

**Purpose**: CPU feature query interface

**Implementation**: `DsmilCPUFeatures.{h,cpp}`

**Capabilities**:
- Parse `!dsllvm.cpu.features` metadata from module
- Provide feature query methods (e.g., `hasAVXVNNI()`, `hasIBRSEnhanced()`)
- Categorize features (AI, security, profiling, crypto, etc.)
- Check for feature combinations (e.g., `hasConstantTimeSupport()`)

**Usage**:
```cpp
#include "DsmilCPUFeatures.h"

using namespace llvm::dsmil;

PreservedAnalyses MyPass::run(Module &M, ModuleAnalysisManager &AM) {
  CPUFeatures Features(M);
  
  if (Features.hasAVXVNNI()) {
    // Emit AVX-VNNI codegen
  }
}
```

---

### 2. DsmilBandwidthEstimate

**Purpose**: Estimate memory bandwidth usage using CPU features

**Implementation**: `DsmilBandwidthEstimate.{h,cpp}`

**CPU Features Used**:
- `fsrm` – Fast Short REP MOVSB (< 256 bytes)
- `erms` – Enhanced REP MOVSB/STOSB (large copies)
- `avx_vnni` – Vector memory ops
- `rep_good` – REP string ops performance

**Functionality**:
- Counts loads/stores per function
- Estimates memcpy/memset cost based on CPU features
- Classifies access patterns (contiguous, strided, gather-scatter)
- Determines memory class (kv_cache, model_weights, hot_ram, etc.)

**Metadata Attached**:
- `!dsmil.bw_bytes_read`
- `!dsmil.bw_bytes_written`
- `!dsmil.bw_gbps_estimate`
- `!dsmil.memory_class`
- `!dsmil.access_pattern`

**Example Output**:
```
Function 'llm_decode_step': read=1048576B, write=262144B, est=23.5 GB/s, class=kv_cache, pattern=contiguous
```

---

### 3. DsmilAIAccelerate

**Purpose**: Optimize AI kernels using AVX-VNNI and BMI instructions

**Implementation**: `DsmilAIAccelerate.{h,cpp}`

**CPU Features Used**:
- `avx_vnni` – INT8 vector neural network instructions
- `bmi1` / `abm` – Bit manipulation for sparse ops
- `fsrm` / `erms` – Efficient matrix loads

**Functionality**:
- Detects AI kernel types (GEMM, Conv2D, Attention)
- Lowers INT8 operations to AVX-VNNI intrinsics (VPDPBUSD)
- Optimizes bit operations with POPCNT/LZCNT/TZCNT
- Checks if kernel operates on INT8 data

**Metadata Attached**:
- `!dsmil.ai.kernel_type` (gemm, conv2d, attention, etc.)
- `!dsmil.ai.vnni_optimized` (true/false)

**Example Output**:
```
Optimized GEMM kernel: matmul_int8
  [STUB] Would lower to VPDPBUSD intrinsics
```

---

### 4. DsmilSpecHardening

**Purpose**: Speculation mitigation using hardware features

**Implementation**: `DsmilSpecHardening.{h,cpp}`

**CPU Features Used**:
- `ibrs_enhanced` – Enhanced Indirect Branch Restricted Speculation
- `ssbd` – Speculative Store Bypass Disable
- `md_clear` – Microarchitectural Data Sampling mitigation
- `stibp` / `ibpb` – Additional Spectre mitigations

**Hardening Modes**:
- `Off` – No hardening
- `Hardware` – Rely on hardware mitigations (default)
- `Hybrid` – Hardware + selective fences
- `Paranoid` – Always fence (ignore hardware)

**Functionality**:
- Identifies hazard sites (indirect branches, bounds checks, speculative loads, MDS-vulnerable ops)
- Mitigates hazards using LFENCE or hardware features
- Inserts MD_CLEAR (VERW) for MDS mitigation
- Prefers hardware mitigations over unconditional fences

**Metadata Attached**:
- `!dsmil.spec.hazard_count`
- `!dsmil.spec.mitigated_count`

**Example Output**:
```
Function 'crypto_sign': 12 hazards
  Inserting LFENCE after bounds check
  Relying on IBRS for indirect branch
  Relying on SSBD for speculative load
  Summary: 8/12 hazards mitigated
```

---

### 5. DsmilConstantTimeCheck

**Purpose**: Enforce constant-time execution for crypto functions

**Implementation**: `DsmilConstantTimeCheck.{h,cpp}`

**CPU Features Used**:
- `user_shstk` – User-mode shadow stack (CET)
- `clflushopt` – Optimized cache flush (prevents cache timing attacks)
- `clwb` – Cache line write-back (persistent memory / cleanup)

**Functionality**:
- Detects functions marked `dsmil_secret` or crypto-related names
- Identifies constant-time violations:
  - Secret-dependent branches (if/switch on secret data)
  - Secret-dependent memory accesses (array[secret])
  - Variable-time instructions (div/mod on secrets)
  - Missing cache flushes
- Inserts CLFLUSHOPT/CLWB after secret operations
- Inserts MFENCE at function exit

**Metadata Attached**:
- `!dsmil.ct_verified` (true/false)
- `!dsmil.ct_violation_count`

**Example Output**:
```
Checking crypto function: aes_encrypt
  VIOLATION: Secret-dependent branch detected
  Inserting CLFLUSHOPT after secret store
  Inserting MFENCE at function exit
  ✗ 1 violations found
```

---

## Building

### Prerequisites

- LLVM 14+ (development headers)
- CMake 3.13+
- C++17 compiler

### Build Instructions

```bash
cd /workspace/dsmil/llvm-passes
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

This installs passes to `/usr/local/lib/dsllvm/` and headers to `/usr/local/include/dsllvm/`.

---

## Usage

### Via opt (LLVM Optimizer)

```bash
# Load and run a pass
opt -load-pass-plugin=/usr/local/lib/dsllvm/libDsmilBandwidthEstimate.so \
    -passes=dsmil-bandwidth-estimate \
    -S < input.ll > output.ll
```

### Via Clang (with plugin)

```bash
# Enable AI acceleration
clang -fplugin=/usr/local/lib/dsllvm/libDsmilAIAccelerate.so \
      -fpass-plugin=dsmil-ai-accelerate \
      -march=dsmil-mtl \
      -O3 -o myapp myapp.c
```

### Via DSLLVM Wrapper

```bash
# Use dsmil-clang with automatic pass loading
dsmil-clang -fdsllvm-ai-accelerate \
            -fdsllvm-spec-hard \
            -fdsllvm-profile=mtr-mtl-dsmil \
            -O3 -o myapp myapp.c
```

---

## Testing

### Unit Tests

```bash
cd build
ctest
```

### Integration Test

```bash
# Run all passes on a test program
./test-integration.sh
```

---

## Development

### Adding a New Pass

1. Create `DsmilNewPass.h` and `DsmilNewPass.cpp`
2. Include `DsmilCPUFeatures.h` and query features
3. Implement `PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM)`
4. Add to `CMakeLists.txt`
5. Register pass in LLVM pass manager

### Pass Template

```cpp
#include "DsmilCPUFeatures.h"

using namespace llvm;
using namespace llvm::dsmil;

class MyPass : public PassInfoMixin<MyPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    CPUFeatures Features(M);
    
    if (!Features.hasMyFeature())
      return PreservedAnalyses::all();
    
    bool Modified = false;
    
    for (Function &F : M) {
      // ... analyze and transform ...
      Modified = true;
    }
    
    return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
  }
};
```

---

## References

- **Specification**: `/workspace/dsmil/docs/DSLLVM_CPU_FEATURE_MODEL.md`
- **Feature Reference**: `/workspace/dsmil/docs/CPU_FEATURES_REFERENCE.md`
- **CPU Profile**: `/workspace/dsmil/config/cpu/mtr-mtl-dsmil.json`
- **DSLLVM Design**: `/workspace/dsmil/docs/DSLLVM-DESIGN.md`

---

## Status

| Pass | Status | Notes |
|------|--------|-------|
| DsmilCPUFeatures | ✅ Complete | Feature query interface |
| DsmilBandwidthEstimate | ✅ Complete | Bandwidth estimation with CPU features |
| DsmilAIAccelerate | 🚧 Stub | VNNI lowering needs implementation |
| DsmilSpecHardening | 🚧 Stub | Intrinsic insertion needs implementation |
| DsmilConstantTimeCheck | 🚧 Stub | Cache flush intrinsics need implementation |

**Legend**:
- ✅ Complete: Fully implemented
- 🚧 Stub: Framework complete, intrinsics/lowering pending
- ❌ Not Started

---

## License

Part of the DSLLVM Project – DSMIL Kernel Team
