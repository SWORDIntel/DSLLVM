#!/bin/bash
# test-integration.sh - Integration test for DSLLVM CPU feature passes

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo "======================================================================"
echo "DSLLVM CPU Feature Integration Test"
echo "======================================================================"
echo ""

# Check if build exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory not found${NC}"
    echo "Run: cd $SCRIPT_DIR && mkdir build && cd build && cmake .. && make"
    exit 1
fi

# Check for test program
if [ ! -f "$SCRIPT_DIR/test_cpu_features.c" ]; then
    echo -e "${RED}Error: test_cpu_features.c not found${NC}"
    exit 1
fi

# Compile to LLVM IR
echo -e "${YELLOW}Step 1: Compiling test program to LLVM IR${NC}"
clang -S -emit-llvm -O1 -o "$BUILD_DIR/test_cpu_features.ll" \
      "$SCRIPT_DIR/test_cpu_features.c"

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Compilation successful${NC}"
else
    echo -e "${RED}✗ Compilation failed${NC}"
    exit 1
fi

# Add CPU feature metadata to IR
echo ""
echo -e "${YELLOW}Step 2: Adding CPU feature metadata${NC}"

cat >> "$BUILD_DIR/test_cpu_features.ll" << 'EOF'

!dsllvm.cpu.profile = !{!0}
!dsllvm.cpu.features = !{!1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12}

!0 = !{!"mtr-mtl-dsmil"}
!1 = !{!"avx_vnni"}
!2 = !{!"fsrm"}
!3 = !{!"erms"}
!4 = !{!"bmi1"}
!5 = !{!"constant_tsc"}
!6 = !{!"ibrs_enhanced"}
!7 = !{!"ssbd"}
!8 = !{!"md_clear"}
!9 = !{!"user_shstk"}
!10 = !{!"clflushopt"}
!11 = !{!"sha_ni"}
!12 = !{!"pclmulqdq"}
EOF

echo -e "${GREEN}✓ Metadata added${NC}"

# Check if LLVM opt is available
if ! command -v opt &> /dev/null; then
    echo -e "${RED}Error: LLVM opt not found${NC}"
    exit 1
fi

echo ""
echo -e "${YELLOW}Step 3: Running DSLLVM passes${NC}"
echo "----------------------------------------------------------------------"

# Note: These commands would work if the passes were properly registered
# For now, we'll show what they would do

echo ""
echo "The following passes would be run:"
echo "  1. DsmilBandwidthEstimate - Estimate memory bandwidth"
echo "  2. DsmilAIAccelerate - Optimize AI kernels with AVX-VNNI"
echo "  3. DsmilSpecHardening - Insert speculation mitigations"
echo "  4. DsmilConstantTimeCheck - Verify constant-time crypto"
echo ""

echo -e "${YELLOW}Pass 1: Bandwidth Estimation${NC}"
echo "Would analyze: gemm_int8, conv2d_int8, large_memcpy, small_memcpy"
echo "Would report:"
echo "  - gemm_int8: Uses AVX-VNNI for vectorization"
echo "  - large_memcpy: Uses ERMS for fast copying"
echo "  - small_memcpy: Uses FSRM for <256 byte copies"
echo ""

echo -e "${YELLOW}Pass 2: AI Acceleration${NC}"
echo "Would optimize: gemm_int8, conv2d_int8, attention_qkv"
echo "Would emit:"
echo "  - VPDPBUSD intrinsics for INT8 multiply-accumulate"
echo "  - Vectorized loops with AVX-VNNI"
echo ""

echo -e "${YELLOW}Pass 3: Speculation Hardening${NC}"
echo "Would harden: array_access, indirect_call, speculative_load_chain"
echo "Would insert:"
echo "  - LFENCE after bounds checks (Spectre v1)"
echo "  - IBRS-aware code for indirect calls (Spectre v2)"
echo "  - SSBD-aware code for speculative loads (Spectre v4)"
echo ""

echo -e "${YELLOW}Pass 4: Constant-Time Check${NC}"
echo "Would verify: aes_encrypt, hmac_compute, crypto_compare"
echo "Would check for:"
echo "  - Secret-dependent branches"
echo "  - Secret-dependent memory accesses"
echo "  - Variable-time operations (div/mod)"
echo "  - Missing cache flushes"
echo ""

# Show metadata that would be attached
echo -e "${YELLOW}Expected Metadata:${NC}"
cat << 'EOF'
Function: gemm_int8
  !dsmil.ai.kernel_type = "gemm"
  !dsmil.ai.vnni_optimized = true
  !dsmil.bw_gbps_estimate = 23.5

Function: aes_encrypt
  !dsmil.ct_verified = true
  !dsmil.ct_violation_count = 0

Function: array_access
  !dsmil.spec.hazard_count = 1
  !dsmil.spec.mitigated_count = 1
EOF

echo ""
echo "======================================================================"
echo "Integration Test Summary"
echo "======================================================================"
echo ""
echo -e "${GREEN}✓ Test IR generated successfully${NC}"
echo -e "${GREEN}✓ CPU feature metadata attached${NC}"
echo -e "${YELLOW}⚠ Pass execution is stubbed (need full LLVM integration)${NC}"
echo ""
echo "Next steps:"
echo "  1. Complete LLVM pass registration"
echo "  2. Implement VNNI intrinsic lowering"
echo "  3. Implement cache flush intrinsics"
echo "  4. Run: opt -load-pass-plugin=... -passes=dsmil-bandwidth-estimate ..."
echo ""
echo "Test IR available at: $BUILD_DIR/test_cpu_features.ll"
echo ""
