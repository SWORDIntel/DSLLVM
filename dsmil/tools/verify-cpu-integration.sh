#!/bin/bash
# DSLLVM CPU Feature Integration Verification Script
# Tests that CPU feature model is correctly integrated

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DSMIL_ROOT="$(dirname "$SCRIPT_DIR")"
CONFIG_DIR="$DSMIL_ROOT/config/cpu"
DOCS_DIR="$DSMIL_ROOT/docs"

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

pass_count=0
fail_count=0

function check_pass() {
    echo -e "${GREEN}✓${NC} $1"
    ((pass_count++))
}

function check_fail() {
    echo -e "${RED}✗${NC} $1"
    ((fail_count++))
}

function check_warn() {
    echo -e "${YELLOW}⚠${NC} $1"
}

echo "======================================================================"
echo "DSLLVM CPU Feature Integration Verification"
echo "======================================================================"
echo ""

# Phase 1: Documentation
echo "Phase 1: Documentation Files"
echo "----------------------------------------------------------------------"

if [ -f "$DOCS_DIR/DSLLVM_CPU_FEATURE_MODEL.md" ]; then
    check_pass "DSLLVM_CPU_FEATURE_MODEL.md exists"
else
    check_fail "DSLLVM_CPU_FEATURE_MODEL.md missing"
fi

if [ -f "$DOCS_DIR/CPU_FEATURES_REFERENCE.md" ]; then
    check_pass "CPU_FEATURES_REFERENCE.md exists"
else
    check_fail "CPU_FEATURES_REFERENCE.md missing"
fi

if [ -f "$DOCS_DIR/DSLLVM_CPU_INTEGRATION_SUMMARY.md" ]; then
    check_pass "DSLLVM_CPU_INTEGRATION_SUMMARY.md exists"
else
    check_fail "DSLLVM_CPU_INTEGRATION_SUMMARY.md missing"
fi

# Check for corrected descriptions
if grep -q "Alternate multi-byte NOP encoding" "$DOCS_DIR/CPU_FEATURES_REFERENCE.md"; then
    check_pass "nopl description corrected (multi-byte NOP, not no-execute)"
else
    check_fail "nopl description not corrected"
fi

if grep -q "Virtual 8086 Mode Enhancements" "$DOCS_DIR/CPU_FEATURES_REFERENCE.md"; then
    check_pass "vme description corrected (VM86, not VT-x)"
else
    check_fail "vme description not corrected"
fi

echo ""

# Phase 2: Configuration Files
echo "Phase 2: Configuration Files"
echo "----------------------------------------------------------------------"

if [ -f "$CONFIG_DIR/mtr-mtl-dsmil.json" ]; then
    check_pass "mtr-mtl-dsmil.json CPU profile exists"
    
    # Validate JSON
    if command -v jq >/dev/null 2>&1; then
        if jq empty "$CONFIG_DIR/mtr-mtl-dsmil.json" 2>/dev/null; then
            check_pass "mtr-mtl-dsmil.json is valid JSON"
        else
            check_fail "mtr-mtl-dsmil.json is invalid JSON"
        fi
        
        # Check for key features
        if jq -e '.features.ai_acceleration | index("avx_vnni")' "$CONFIG_DIR/mtr-mtl-dsmil.json" >/dev/null; then
            check_pass "CPU profile includes avx_vnni (AI acceleration)"
        else
            check_fail "CPU profile missing avx_vnni"
        fi
        
        if jq -e '.features.security | index("user_shstk")' "$CONFIG_DIR/mtr-mtl-dsmil.json" >/dev/null; then
            check_pass "CPU profile includes user_shstk (CET)"
        else
            check_fail "CPU profile missing user_shstk"
        fi
        
        if jq -e '.features.profiling | index("intel_pt")' "$CONFIG_DIR/mtr-mtl-dsmil.json" >/dev/null; then
            check_pass "CPU profile includes intel_pt (profiling)"
        else
            check_fail "CPU profile missing intel_pt"
        fi
    else
        check_warn "jq not installed, skipping JSON validation"
    fi
else
    check_fail "mtr-mtl-dsmil.json CPU profile missing"
fi

echo ""

# Phase 3: Tools
echo "Phase 3: Tools"
echo "----------------------------------------------------------------------"

if [ -f "$SCRIPT_DIR/dsllvm-cpufeatures" ]; then
    check_pass "dsllvm-cpufeatures tool exists"
    
    if [ -x "$SCRIPT_DIR/dsllvm-cpufeatures" ]; then
        check_pass "dsllvm-cpufeatures is executable"
    else
        check_fail "dsllvm-cpufeatures is not executable"
    fi
    
    # Test execution (only on Linux)
    if [ -f /proc/cpuinfo ]; then
        if python3 "$SCRIPT_DIR/dsllvm-cpufeatures" >/dev/null 2>&1; then
            check_pass "dsllvm-cpufeatures runs successfully"
        else
            check_fail "dsllvm-cpufeatures execution failed"
        fi
    else
        check_warn "Not running on Linux, skipping dsllvm-cpufeatures execution test"
    fi
else
    check_fail "dsllvm-cpufeatures tool missing"
fi

echo ""

# Phase 4: DSLLVM Design Integration
echo "Phase 4: DSLLVM Design Integration"
echo "----------------------------------------------------------------------"

if grep -q "CPU Feature Integration" "$DOCS_DIR/DSLLVM-DESIGN.md"; then
    check_pass "DSLLVM-DESIGN.md references CPU feature integration"
else
    check_fail "DSLLVM-DESIGN.md missing CPU feature integration section"
fi

if grep -q "fdsllvm-ai-accelerate" "$DOCS_DIR/DSLLVM-DESIGN.md"; then
    check_pass "DSLLVM-DESIGN.md documents -fdsllvm-ai-accelerate flag"
else
    check_fail "DSLLVM-DESIGN.md missing -fdsllvm-ai-accelerate flag"
fi

if grep -q "fdsllvm-spec-hard" "$DOCS_DIR/DSLLVM-DESIGN.md"; then
    check_pass "DSLLVM-DESIGN.md documents -fdsllvm-spec-hard flag"
else
    check_fail "DSLLVM-DESIGN.md missing -fdsllvm-spec-hard flag"
fi

echo ""

# Phase 5: README Integration
echo "Phase 5: Documentation Index Integration"
echo "----------------------------------------------------------------------"

if grep -q "Hardware Integration" "$DOCS_DIR/README.md"; then
    check_pass "README.md includes Hardware Integration section"
else
    check_fail "README.md missing Hardware Integration section"
fi

if grep -q "DSLLVM_CPU_FEATURE_MODEL.md" "$DOCS_DIR/README.md"; then
    check_pass "README.md references DSLLVM_CPU_FEATURE_MODEL.md"
else
    check_fail "README.md missing reference to DSLLVM_CPU_FEATURE_MODEL.md"
fi

echo ""

# Summary
echo "======================================================================"
echo "Verification Summary"
echo "======================================================================"
echo -e "Passed: ${GREEN}${pass_count}${NC}"
echo -e "Failed: ${RED}${fail_count}${NC}"
echo ""

if [ $fail_count -eq 0 ]; then
    echo -e "${GREEN}✓ All checks passed! CPU feature integration is complete.${NC}"
    exit 0
else
    echo -e "${RED}✗ Some checks failed. Review the output above.${NC}"
    exit 1
fi
