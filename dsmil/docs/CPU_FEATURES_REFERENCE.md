# CPU Features Reference – Meteor Lake DSMIL Node

**Version**: 1.0  
**Date**: 2025-12-07  
**Hardware**: Intel Core Ultra 7 165H (Meteor Lake)

---

## Overview

This document provides detailed descriptions of all CPU features available on the Meteor Lake DSMIL node, extracted from `/proc/cpuinfo` flags. These features are first-class inputs to DSLLVM for optimization, security, and profiling decisions.

**IMPORTANT CORRECTIONS**:

* `nopl`: **NOT** no-execute protection (that's `nx`). It's alternate multi-byte NOP encoding for alignment and patching.
* `vme`: **NOT** VT-x virtualization. It's Virtual 8086 Mode Enhancements for legacy 16-bit code.

---

## Feature Categories

### 1. AI & Vector Acceleration

| Feature | Description | DSLLVM Usage |
|---------|-------------|--------------|
| `avx` | Advanced Vector Extensions (256-bit SIMD) | Base vector operations |
| `avx2` | Advanced Vector Extensions 2 (enhanced 256-bit) | GEMM, convolutions |
| `avx_vnni` | AVX Vector Neural Network Instructions (INT8) | **AI hotpaths**, quantized models |
| `fma` | Fused Multiply-Add | Dense math kernels |
| `bmi1` | Bit Manipulation Instruction Set 1 | Popcount, bit scans for sparse ops |
| `bmi2` | Bit Manipulation Instruction Set 2 | Advanced bit manipulation |
| `abm` | Advanced Bit Manipulation (LZCNT, POPCNT) | Constant-time bitset ops |
| `sse4_1` | Streaming SIMD Extensions 4.1 | Vector operations |
| `sse4_2` | Streaming SIMD Extensions 4.2 | String/text processing, CRC32 |
| `popcnt` | Population Count instruction | Hamming distance, sparse matrices |
| `fsrm` | Fast Short REP MOVSB | **Optimized memcpy/memset < 256 bytes** |
| `erms` | Enhanced REP MOVSB/STOSB | **Optimized large memory operations** |
| `rep_good` | REP string ops are fast | Prefer REP over explicit loops |

**DSLLVM Integration**: When `-fdsllvm-ai-accelerate` is set and `avx_vnni` is present, DSLLVM emits INT8 VNNI instructions for AI kernels. `fsrm` and `erms` optimize loop-based memory operations.

---

### 2. Security Features

| Feature | Description | DSLLVM Usage |
|---------|-------------|--------------|
| `smep` | Supervisor Mode Execution Prevention | Kernel can't exec user pages |
| `smap` | Supervisor Mode Access Prevention | Kernel can't access user data without STAC/CLAC |
| `umip` | User Mode Instruction Prevention | Blocks SGDT/SIDT/SLDT/SMSW/STR in userland |
| `user_shstk` | User-mode shadow stack (CET) | **Call/ret integrity**, `-fdsllvm-harden` |
| `tme` | Total Memory Encryption | All DRAM encrypted at memory controller |
| `smx` | Safer Mode Extensions (Intel TXT) | Measured launch, trusted boot |
| `md_clear` | Microarchitectural Data Sampling mitigation | MDS/RIDL/Fallout defenses |
| `flush_l1d` | L1D cache flush capability | L1TF mitigation |
| `ssbd` | Speculative Store Bypass Disable | Spectre v4 mitigation |
| `ibrs_enhanced` | Enhanced Indirect Branch Restricted Speculation | **Hardware-based Spectre v2 mitigation** |
| `stibp` | Single Thread Indirect Branch Predictor | Cross-HT Spectre v2 defense |
| `ibpb` | Indirect Branch Prediction Barrier | Flush branch predictor state |
| `nx` | No-eXecute (DEP) | W^X enforcement |
| `pku` | Protection Keys for Userspace | Fine-grained memory permissions |

**DSLLVM Integration**:

* `-fdsllvm-harden` emits CET-aware prologues/epilogues when `user_shstk` is present.
* `-fdsllvm-spec-hard` prefers hardware mitigations (`ibrs_enhanced`, `ssbd`) over unconditional LFENCE insertion.
* Provenance records assume `tme=true`, `smx=false` for this node.

---

### 3. Profiling & Observability

| Feature | Description | DSLLVM Usage |
|---------|-------------|--------------|
| `intel_pt` | Intel Processor Trace | **Hardware execution tracing**, `-fdsllvm-prof=pt` |
| `arch_lbr` | Architectural Last Branch Records | Control flow profiling, `-fdsllvm-prof=lbr` |
| `pebs` | Precise Event Based Sampling | **Hotspot identification**, `-fdsllvm-prof=pebs` |
| `bts` | Branch Trace Store | Legacy branch tracing |
| `arch_perfmon` | Architectural Performance Monitoring | Generic PMU events |
| `hfi` | Hardware Feedback Interface | **Power/thermal hints**, auto-tuning |
| `pdcm` | Performance & Debug Capability MSR | Advanced PMU config |
| `dtes64` | 64-bit Debug Store | Debug store mechanism |
| `monitor` | MONITOR/MWAIT instructions | Efficient idle loops |

**DSLLVM Integration**:

* `-fdsllvm-prof=pt` instruments binaries for Intel PT; runtime tools decode traces to IR/BB coverage.
* `-fdsllvm-prof=hfi` integrates HFI thermal hints to adjust device placement (CPU vs NPU).

---

### 4. Virtualization

| Feature | Description | DSLLVM Usage |
|---------|-------------|--------------|
| `vmx` | Virtual Machine Extensions (VT-x) | Hardware virtualization |
| `ept` | Extended Page Tables | Second-level address translation |
| `ept_ad` | EPT Accessed/Dirty bits | Efficient guest page tracking |
| `vpid` | Virtual Processor ID | Tagged TLB for VMs |
| `x2apic` | Extended xAPIC (x2APIC) | Scalable interrupt delivery |
| `tpr_shadow` | Task Priority Register shadowing | Optimized interrupt virtualization |
| `flexpriority` | Flexible priority (APIC virtualization) | Efficient guest interrupt handling |
| `vnmi` | Virtual NMI | Virtualized non-maskable interrupts |
| `rdtscp` | RDTSCP instruction | TSC + processor ID |

**DSLLVM Integration**: Hypervisor builds (`-fdsllvm-profile=mtr-mtl-hv`) use these for EPT alignment hints and VMX-aware code paths.

---

### 5. Memory Operations

| Feature | Description | DSLLVM Usage |
|---------|-------------|--------------|
| `fsrm` | Fast Short REP MOVSB | **Small memcpy (<256B) optimization** |
| `erms` | Enhanced REP MOVSB/STOSB | **Large memcpy/memset optimization** |
| `3dnowprefetch` | PREFETCH/PREFETCHW instructions | Software prefetching |
| `clflushopt` | Optimized CLFLUSH | Cache line writeback/invalidate |
| `clwb` | Cache Line Write Back | Persistent memory support |
| `clflush` | CLFLUSH instruction | Cache line flush |
| `invpcid` | Invalidate Process-Context ID | TLB management |

**DSLLVM Integration**: Bandwidth estimation pass uses `fsrm`/`erms` to model memcpy/memset costs accurately.

---

### 6. Miscellaneous

| Feature | Description | DSLLVM Usage |
|---------|-------------|--------------|
| `nopl` | **Alternate multi-byte NOP encoding** | **Alignment, patchable code sequences** |
| `vme` | **Virtual 8086 Mode Enhancements** | **Legacy 16-bit code support** |
| `bus_lock_detect` | Bus lock detection & notification | `-fdsllvm-sanitize=locks` |
| `split_lock_detect` | Split lock detection & notification | `-fdsllvm-sanitize=locks` |
| `constant_tsc` | Constant-rate TSC | Reliable timing |
| `nonstop_tsc` | TSC doesn't stop in C-states | Always-on timing |
| `tsc_deadline_timer` | TSC deadline mode | Precise timer interrupts |
| `rdrand` | RDRAND instruction | Hardware RNG |
| `rdseed` | RDSEED instruction | Entropy source |

**CRITICAL CORRECTIONS**:

* **`nopl`**: This is **NOT** no-execute protection (that's `nx`). It's multi-byte NOP encoding used for:
  * Code alignment (pad to cache line boundaries).
  * Patchable sequences (replace NOP with jump for runtime patching).
  * Example: `nopl (%rax, %rax, 1)` is a 5-byte NOP.

* **`vme`**: This is **NOT** VT-x virtualization. It's "Virtual 8086 Mode Enhancements" for:
  * Running legacy DOS/16-bit code under protected mode.
  * Virtualizing interrupt flag (VIF) in V86 mode.
  * Modern code doesn't use this; it's historical baggage.

**DSLLVM Integration**: Sanitizers detect `bus_lock_detect` / `split_lock_detect` and warn about misaligned atomics.

---

## Complete Feature List for Meteor Lake

Below is the canonical list of CPU flags from `/proc/cpuinfo` for the Meteor Lake DSMIL node, with corrected descriptions:

```
fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov 
pat pse36 clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall 
nx pdpe1gb rdtscp lm constant_tsc art arch_perfmon pebs bts rep_good 
nopl xtopology nonstop_tsc cpuid aperfmperf tsc_known_freq pni 
pclmulqdq dtes64 monitor ds_cpl vmx smx est tm2 ssse3 sdbg fma cx16 
xtpr pdcm sse4_1 sse4_2 x2apic movbe popcnt tsc_deadline_timer aes 
xsave avx f16c rdrand lahf_lm abm 3dnowprefetch cpuid_fault ssbd 
ibrs ibpb stibp ibrs_enhanced tpr_shadow flexpriority ept vpid 
ept_ad fsgsbase tsc_adjust bmi1 avx2 smep bmi2 erms invpcid smx 
rdseed adx smap clflushopt clwb intel_pt sha_ni xsaveopt xsavec 
xgetbv1 xsaves split_lock_detect user_shstk avx_vnni waitpkg umip 
pku vaes vpclmulqdq rdpid movdiri movdir64b fsrm md_clear serialize 
arch_lbr ibt flush_l1d arch_capabilities
```

### Grouped by Category

**AI/Vector**: `avx`, `avx2`, `avx_vnni`, `fma`, `bmi1`, `bmi2`, `abm`, `sse4_1`, `sse4_2`, `popcnt`, `fsrm`, `erms`, `rep_good`

**Security**: `smep`, `smap`, `umip`, `user_shstk`, `tme`, `smx`, `md_clear`, `flush_l1d`, `ssbd`, `ibrs_enhanced`, `stibp`, `ibpb`, `nx`, `pku`, `ibt`

**Profiling**: `intel_pt`, `arch_lbr`, `pebs`, `bts`, `arch_perfmon`, `hfi`, `pdcm`, `dtes64`, `monitor`

**Virtualization**: `vmx`, `ept`, `ept_ad`, `vpid`, `x2apic`, `tpr_shadow`, `flexpriority`, `vnmi`, `rdtscp`

**Memory**: `fsrm`, `erms`, `3dnowprefetch`, `clflushopt`, `clwb`, `clflush`, `invpcid`

**Misc**: `nopl`, `vme`, `bus_lock_detect`, `split_lock_detect`, `constant_tsc`, `nonstop_tsc`, `tsc_deadline_timer`, `rdrand`, `rdseed`

---

## Integration with DSLLVM

### Metadata Emission

When compiling with `-march=dsmil-mtl` or `-fdsllvm-profile=mtr-mtl-dsmil`, DSLLVM attaches:

```llvm
!dsllvm.cpu.profile = !{!"mtr-mtl-dsmil"}
!dsllvm.cpu.features = !{
  !"avx_vnni", !"fsrm", !"erms", !"bmi1", !"abm",
  !"intel_pt", !"arch_lbr", !"pebs", !"bts",
  !"smep", !"smap", !"umip", !"user_shstk",
  !"tme", !"smx", !"md_clear", !"flush_l1d",
  !"ssbd", !"ibrs_enhanced", !"stibp"
}
```

### Pass Usage

| Pass | Features Used | Purpose |
|------|---------------|---------|
| `dsmil-bandwidth-estimate` | `fsrm`, `erms`, `avx_vnni` | Model memcpy/GEMM bandwidth accurately |
| `dsmil-ai-accelerate` | `avx_vnni`, `bmi1`, `abm` | Emit VNNI codegen for AI kernels |
| `dsmil-ct-check` | `user_shstk`, `pku` | Constant-time enforcement with CET |
| `dsmil-spec-hardening` | `ibrs_enhanced`, `ssbd`, `md_clear` | Speculation mitigation using hardware |
| `dsmil-prof-instrument` | `intel_pt`, `arch_lbr`, `pebs` | Profiling instrumentation |

### Provenance

Build provenance includes CPU feature profile:

```json
{
  "cpu_profile": "mtr-mtl-dsmil",
  "cpu_features": {
    "ai_acceleration": ["avx_vnni", "fsrm", "erms"],
    "security": ["smep", "smap", "umip", "user_shstk", "ibrs_enhanced"],
    "profiling": ["intel_pt", "arch_lbr", "pebs"]
  },
  "feature_assumptions": {
    "tme_required": true,
    "txt_expected": false,
    "cet_available": true
  }
}
```

---

## Tool: `dsllvm-cpufeatures`

Use `dsllvm-cpufeatures` to probe the current system and generate a JSON profile:

```bash
$ /workspace/dsmil/tools/dsllvm-cpufeatures > /tmp/my-cpu-profile.json
```

Output includes:

* `llvm_target_features`: LLVM flags to enable (e.g., `+avx_vnni`).
* `features`: Categorized feature lists.
* `all_flags`: Raw `/proc/cpuinfo` flags.

---

## References

* **DSLLVM Design**: `docs/DSLLVM-DESIGN.md`
* **CPU Feature Model**: `docs/DSLLVM_CPU_FEATURE_MODEL.md`
* **CPU Profile JSON**: `config/cpu/mtr-mtl-dsmil.json`
* **Intel SDM**: Software Developer's Manual, Vol. 2 (Instruction Set Reference)
* **Meteor Lake Specs**: Intel Core Ultra 7 165H product brief

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| v1.0 | 2025-12-07 | DSMIL Kernel Team | Initial reference with corrected `nopl` and `vme` descriptions |

---

**End of Reference**
