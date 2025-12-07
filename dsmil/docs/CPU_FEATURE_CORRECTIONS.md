# CPU Feature Description Corrections

**Version**: 1.0  
**Date**: 2025-12-07

This document tracks corrections to commonly misunderstood CPU feature descriptions.

---

## Critical Corrections

### 1. `nopl` - NOT No-Execute Protection ❌

**INCORRECT Description (Common Error)**:
```
nopl: No-execute page protection, a security feature to prevent execution of code in data pages.
```

**CORRECT Description** ✅:
```
nopl: Alternate multi-byte NOP encoding, used for alignment and patchable code sequences.
```

**Explanation**:
- `nopl` is the **NOPL instruction** (e.g., `nopl (%rax, %rax, 1)` - a 5-byte NOP)
- Used for **code alignment** (pad functions to cache line boundaries)
- Used for **patchable sequences** (replace NOP with jump at runtime)
- **NOT** related to W^X or DEP
- **No-execute protection** is provided by the **`nx`** feature (NX bit/DEP)

**Impact**: Confusing `nopl` with `nx` leads to incorrect security assumptions.

**References**:
- Intel SDM Vol. 2B, NOPL instruction (0F 1F)
- Linux kernel: arch/x86/include/asm/alternative.h (uses NOPL for code patching)

---

### 2. `vme` - NOT VT-x Virtualization ❌

**INCORRECT Description (Common Error)**:
```
vme: Virtual Machine Extensions, hardware support for virtualization.
```

**CORRECT Description** ✅:
```
vme: Virtual 8086 Mode Enhancements, assists running legacy 16-bit code under protected mode.
```

**Explanation**:
- `vme` is **Virtual Mode Extensions** for **VM86 mode** (DOS/16-bit support)
- Virtualizes the interrupt flag (VIF) in V86 mode
- Used for running **legacy 16-bit DOS applications** under 32-bit protected mode
- **NOT** hardware virtualization (that's **`vmx`** for VT-x or **`svm`** for AMD-V)
- Modern 64-bit code **never** uses VM86 mode

**Impact**: Confusing `vme` with `vmx` leads to incorrect assumptions about virtualization support.

**References**:
- Intel SDM Vol. 3A, Chapter 20 (8086 Emulation)
- AMD Programmer's Manual Vol. 2, VM86 Mode Extensions

---

## Status of These Errors in Existing Documentation

### Where These Errors Appear

If you see these incorrect descriptions in:
- Comments in code
- Documentation
- CPU feature lists
- `/proc/cpuinfo` parsers

**Action**: Correct them using the descriptions above.

### DSLLVM Status

✅ **Corrected** in all DSLLVM documentation:
- `docs/DSLLVM_CPU_FEATURE_MODEL.md`
- `docs/CPU_FEATURES_REFERENCE.md`
- `config/cpu/mtr-mtl-dsmil.json`
- `tools/dsllvm-cpufeatures`

---

## Other Common Misconceptions

### 3. `rep_good` - Performance Hint, Not an Instruction

**What it is**: A flag indicating that `REP MOVSB/STOSB` are implemented efficiently on this CPU.

**What it's NOT**: An instruction or feature you "enable".

**DSLLVM Usage**: When `rep_good` is present, prefer `REP MOVSB` over explicit loops for memcpy.

---

### 4. `constant_tsc` vs `nonstop_tsc`

**`constant_tsc`**: TSC increments at a constant rate (not affected by frequency scaling).

**`nonstop_tsc`**: TSC continues running in deep C-states (power-saving modes).

**Difference**: `constant_tsc` = reliable timing. `nonstop_tsc` = reliable timing even when CPU is idle.

**DSLLVM Usage**: Both needed for accurate profiling/benchmarking.

---

### 5. `pku` vs `ospke`

**`pku`**: Hardware supports Protection Keys for Userspace.

**`ospke`**: **OS** has enabled PKU (written to CR4.PKE).

**Difference**: `pku` = hardware capability. `ospke` = OS actually using it.

**DSLLVM Usage**: Check `pku` for hardware support, but only use if `ospke` is also set.

---

## Verification

To verify a CPU feature description is correct:

1. **Check Intel SDM** (Software Developer's Manual)
   - Volume 1: Basic Architecture
   - Volume 2: Instruction Set Reference
   - Volume 3: System Programming Guide

2. **Check Linux kernel source**
   - `arch/x86/include/asm/cpufeatures.h` (canonical feature list)
   - `arch/x86/kernel/cpu/common.c` (feature detection)

3. **Compare with DSLLVM docs**
   - `docs/CPU_FEATURES_REFERENCE.md` (authoritative for DSLLVM)

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| v1.0 | 2025-12-07 | DSMIL Kernel Team | Initial corrections (nopl, vme) |

---

**END OF CORRECTIONS**
