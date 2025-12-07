# DSLLVM CPU Feature Model – Meteor Lake DSMIL Node

## 1. Goals

1. **Exploit hardware**: Use AI / perf features (AVX-VNNI, FSRM, ERMS, BMI1/ABM, arch_perfmon, HFI) in DSLLVM's codegen and auto-tuning.
2. **Harden binaries**: Align DSLLVM's security and constant-time passes with architectural security flags (SMEP, SMAP, UMIP, user_shstk, TME, SMX, md_clear, flush_l1d, ssbd, ibrs/stibp/ibpb).
3. **Improve observability**: Integrate Intel PT, arch LBR, PEBS, BTS, HFI into DSLLVM's profiling/coverage modes.
4. **Keep portability**: Features drive *profiles*, not unconditional use. DSLLVM must still emit portable builds when requested.

---

## 2. Feature Discovery & Profiles

### 2.1 Runtime feature enumeration

Add a small `dsllvm-cpufeatures` helper (or extend existing probe) that:

- Runs CPUID + MSR probes on the DSMIL node.
- Emits a JSON like:

```json
{
  "arch": "x86_64",
  "vendor": "GenuineIntel",
  "family_model_stepping": "06_aa_02",
  "features": [
    "avx_vnni", "fsrm", "erms", "bmi1", "abm",
    "intel_pt", "arch_lbr", "pebs", "bts",
    "smap", "smep", "umip", "user_shstk",
    "tme", "smx", "md_clear", "flush_l1d",
    "md_clear", "ssbd", "ibrs_enhanced", "stibp",
    "hfi", "arch_perfmon", "bus_lock_detect", "split_lock_detect",
    "3dnowprefetch", "fsrm", "rep_good",
    "vmx", "ept", "ept_ad", "vpid", "x2apic"
  ]
}
```

Store it under e.g. `config/cpu/mtr-mtl-dsmil.json`.

### 2.2 DSLLVM CPU profile

Define a named CPU profile, e.g.:

* `-march=dsmil-mtl` or `-fdsllvm-profile=mtr-mtl-dsmil`

This profile:

* Enables the appropriate LLVM x86 target features:

  * `+avx2`, `+avx_vnni`, `+bmi`, `+bmi2`, `+popcnt`, `+sse4.2`, etc.
* Annotates the module with DSLLVM metadata:

```llvm
!dsllvm.cpu.features = !{ !"avx_vnni", !"fsrm", !"intel_pt", !"arch_lbr", !"user_shstk", !"smap", !"smep", !"umip", !"tme", !"md_clear", !"flush_l1d", !"ssbd", !"ibrs_enhanced", !"stibp" }
```

DSLLVM passes can query this metadata to decide behaviour.

---

## 3. Optimisation & AI Hotpaths

### 3.1 AI / vector math (avx_vnni, bmi1, fsrm, erms, rep_good)

* For **AI kernels** (GEMM, conv, attention), when `avx_vnni` is present:

  * Enable lowering of INT8 paths to AVX-VNNI intrinsics.
  * Prefer VNNI + FSRM/ERMS for tight loops and memcpy/memset.

* Add a DSLLVM flag, e.g.:

  * `-fdsllvm-ai-accelerate`:

    * If CPU profile has `avx_vnni`, emit VNNI codegen.
    * Otherwise fall back to AVX2/FMA.

### 3.2 Bit-work (abm, bmi1)

* When `abm`/`bmi1` present:

  * Aggressively map popcount, leading/trailing zero ops to POPCNT/LZCNT/TZCNT.
  * Allow constant-time bitset operations to use POPCNT where safe.

---

## 4. Security-Aware Codegen

### 4.1 CET: user_shstk

If `user_shstk` present *and* DSLLVM is building user-mode binaries with CET enabled:

* Emit function prologues/epilogues that cooperate with shadow stack.
* For hardened builds (`-fdsllvm-harden`):

  * Prefer call/ret patterns that work cleanly with CET shadow stacks.
  * Avoid exotic control-flow constructs that confuse CET (document exceptions).

### 4.2 SMEP/SMAP/UMIP

* When `smep`/`smap`/`umip` are present:

  * DSLLVM should:

    * Avoid generating code that relies on kernel executing user pages or accessing user data without explicit copying (for kernel builds).
    * For userland hardening modes, treat these as **assumptions** in analyses:

      * e.g., static analyzer can assume kernel can't execute user mappings.

### 4.3 Speculation mitigations: ibrs/ibpb/stibp/md_clear/flush_l1d/ssbd

* When `arch_capabilities`, `ibrs_enhanced`, `md_clear`, `flush_l1d`, `ssbd`, `stibp` present:

  * DSLLVM's speculation-hardened mode (`-fdsllvm-spec-hard`) should:

    * Insert LFENCE/serialization only where necessary.
    * Prefer hardware mitigations (IBRS, STIBP, SSBD) when the runtime says they're active, rather than always bloating code.

  * Optional: tag hazard sites in IR and let a late pass decide:

    * fence vs rely-on-hardware based on module metadata (`!dsllvm.cpu.features`).

### 4.4 TME / SMX

* `tme` / `smx` don't change codegen per se, but DSLLVM should:

  * Record in provenance that builds assume:

    * **TME enabled** (all RAM encrypted).
    * **TXT/SMX available** for measured launch where relevant.

  * This can show up in signed build manifests:

    * `tme_required: true/false`
    * `txt_expected: true/false`

---

## 5. Profiling, Coverage & Telemetry

### 5.1 Intel PT, arch_lbr, PEBS, BTS, arch_perfmon, hfi

When these are present:

* Expose a DSLLVM "hardware profiling" mode:

  * `-fdsllvm-prof=pt` → instrument for Intel PT traces.
  * `-fdsllvm-prof=lbr` → use architectural LBR for last-branch histograms.
  * `-fdsllvm-prof=pebs` → use PEBS events for hotspots.
  * `-fdsllvm-prof=hfi` → integrate HFI hints into auto-tuning (e.g., throttle "hot" cores for power-aware builds).

* DSLLVM should emit perf metadata so your profiler / SHRINK integration knows:

  * sampling period,
  * what events to collect,
  * how to map them back to DSLLVM's IR/basic-block IDs.

### 5.2 Bus lock / split lock detection

With `bus_lock_detect` and `split_lock_detect`:

* Add an optional sanitizer:

  * `-fdsllvm-sanitize=locks`:

    * Identifies instructions that may cause split/bus locks (e.g., certain atomic sequences, misaligned atomics).
    * Emits warnings or transforms them into alternative patterns where possible.

---

## 6. Virtualization Hooks (vmx, ept, vpid, ept_ad, tpr_shadow, flexpriority, vnmi)

For code compiled as **hypervisor/guest tools**:

* If profile says `vmx`, `ept`, `ept_ad`, `vpid`:

  * DSLLVM may:

    * Emit hints aligned with EPT usage (e.g. align large pages, encourage 1G pages with `pdpe1gb` when requested).
    * Annotate hot code paths that will run in VMX root vs non-root.

* This likely becomes a separate DSLLVM profile, e.g. `-fdsllvm-profile=mtr-mtl-hv`.

---

## 7. Implementation Checklist

1. **Feature JSON**

   * Generate `config/cpu/mtr-mtl-dsmil.json` via `dsllvm-cpufeatures`.
2. **x86 Target Integration**

   * Map JSON feature list → LLVM x86 subtarget features in `X86.td` / DSLLVM driver.
3. **Module Metadata**

   * Attach `!dsllvm.cpu.features` metadata to every module compiled under the DSMIL Meteor profile.
4. **Pass Integration**

   * Update:

     * AI/vec passes to use `avx_vnni`, `fsrm`, `erms`, `rep_good`.
     * Security passes to use `user_shstk`, `smap/smep/umip`, `md_clear/flush_l1d`, `ibrs/stibp/ssbd`.
     * Profiling/telemetry passes to hook into PT/LBR/PEBS/HFI.
5. **Docs & Provenance**

   * Extend DSLLVM build manifests to record which CPU feature profile was assumed at build time.

---

## 8. CPU Feature Corrections

### 8.1 nopl

**Incorrect**: No-execute protection

**Correct**: Alternate multi-byte NOP encoding, used for alignment and patchable code sequences.

* In practice: "multi-byte NOPs / NOP with displacement" used for alignment and patching.
* It is **not** no-execute protection. That's `nx`.

### 8.2 vme

**Incorrect**: Virtualization (like VT-x)

**Correct**: Virtual 8086 Mode Enhancements, assists running legacy 16-bit code under protected mode.

* It's "Virtual 8086 Mode Enhancements" (old 16-bit/VM86 support), not virtualization like VT-x.

---

## 9. Verification

You're in good shape if, after wiring this:

- `dsclang -march=dsmil-mtl -fdsllvm-ai-accelerate ...`  
  produces AVX-VNNI AI hotpaths and tags the module with the correct feature metadata.

- `dsclang -fdsllvm-spec-hard ...`  
  emits speculation mitigations that *respect* IBRS/SSBD/MD_CLEAR etc. instead of blindly fencing.

- `dsclang -fdsllvm-prof=pt ...`  
  + runtime tools give you PT/LBR-based coverage tied back to DSLLVM IR.

---

## 10. Contingency

If later you want to share DSLLVM builds across machines:

- Keep this profile as **"Meteor-only"** and add a `-fdsllvm-profile=x86_64-baseline` that uses a much smaller feature set.
- The JSON-based feature model makes it trivial to define new profiles for other hosts without changing the core logic – you just add another `config/cpu/*.json` and corresponding profile flag.

---

## 11. Integration with DSLLVM Design

This CPU feature model integrates with the existing DSLLVM architecture (see `DSLLVM-DESIGN.md`):

### 11.1 Target Triple Extension

The existing target triple:

```
x86_64-dsmil-meteorlake-elf
```

Now includes CPU feature metadata in the subtarget, extending the `+dsmil-optimal` feature group to include:

* `+avx_vnni` (AI acceleration)
* `+fsrm` (fast short REP MOVSB)
* `+erms` (enhanced REP MOVSB/STOSB)
* `+bmi`, `+bmi2` (bit manipulation)
* Security features (SMEP, SMAP, UMIP, CET) as **metadata** (not necessarily enabled in codegen unless explicitly requested)

### 11.2 Provenance Integration

The `dsmil-provenance-pass` (§5 of DSLLVM-DESIGN.md) now includes:

```json
{
  "cpu_profile": "mtr-mtl-dsmil",
  "cpu_features": {
    "ai_acceleration": ["avx_vnni", "fsrm"],
    "security": ["smep", "smap", "umip", "user_shstk", "ibrs_enhanced"],
    "profiling": ["intel_pt", "arch_lbr", "pebs"],
    "virtualization": ["vmx", "ept", "ept_ad"]
  },
  "feature_assumptions": {
    "tme_required": true,
    "txt_expected": false
  }
}
```

### 11.3 AI Advisor Integration

The Layer 7/8 AI advisors (§8 of DSLLVM-DESIGN.md) can now query CPU features:

* **L7 LLM Advisor**: Suggests whether to use AVX-VNNI based on workload characteristics.
* **L8 Security AI**: Validates that security-critical code uses CET/SMEP/SMAP assumptions correctly.
* **L5 Performance AI**: Uses HFI hints to predict thermal throttling and adjust device placement.

### 11.4 Pass Pipeline Updates

The standard `dsmil-default` pipeline (§7.2 of DSLLVM-DESIGN.md) now includes:

1. **Feature Discovery** (pre-compile): `dsllvm-cpufeatures` → `config/cpu/mtr-mtl-dsmil.json`
2. **Target Setup**: Load CPU profile and attach `!dsllvm.cpu.features` metadata
3. **Optimization Passes**:
   * `dsmil-bandwidth-estimate` (uses `avx_vnni`, `fsrm` for bandwidth modeling)
   * `dsmil-device-placement` (considers CPU features when choosing CPU/NPU/GPU)
4. **Security Passes**:
   * `dsmil-ct-check` (constant-time enforcement, uses `user_shstk` assumptions)
   * `dsmil-spec-hardening` (new pass, uses `ibrs_enhanced`/`md_clear`/`flush_l1d`)
5. **Profiling Instrumentation** (optional):
   * `dsmil-prof-instrument` (emits PT/LBR/PEBS metadata)

---

## 12. Example Usage

### Compile with full Meteor Lake optimization

```bash
dsclang -march=dsmil-mtl -fdsllvm-ai-accelerate \
        -fdsllvm-profile=mtr-mtl-dsmil \
        -O3 -o myapp.elf myapp.c
```

**Result**: AVX-VNNI codegen, CPU feature metadata in provenance, optimized for Meteor Lake.

### Compile with security hardening

```bash
dsclang -march=dsmil-mtl -fdsllvm-harden \
        -fdsllvm-spec-hard -fdsllvm-profile=mtr-mtl-dsmil \
        -O2 -o secure_app.elf secure_app.c
```

**Result**: CET shadow stack support, speculation mitigations using hardware features, security metadata in provenance.

### Compile with Intel PT profiling

```bash
dsclang -march=dsmil-mtl -fdsllvm-prof=pt \
        -fdsllvm-profile=mtr-mtl-dsmil \
        -O3 -o traced_app.elf traced_app.c
```

**Result**: Binary instrumented for Intel PT, perf metadata for post-processing.

### Portable build (baseline x86-64)

```bash
dsclang -march=x86-64-v2 -fdsllvm-profile=x86_64-baseline \
        -O3 -o portable_app.elf portable_app.c
```

**Result**: No Meteor Lake-specific features, runs on any modern x86-64 CPU.

---

## 13. Future Work

### 13.1 Multi-Profile Compilation

Support compiling multiple profiles in a single build:

```bash
dsclang -fdsllvm-multi-profile=mtr-mtl-dsmil,x86_64-baseline \
        -o myapp.fat myapp.c
```

**Result**: Fat binary with function multi-versioning; runtime selects best implementation based on CPUID.

### 13.2 Dynamic Feature Queries

Add runtime library for querying CPU features:

```c
#include <dsmil/cpu_features.h>

if (dsmil_cpu_has_feature("avx_vnni")) {
    // Use optimized path
    gemm_avx_vnni(A, B, C);
} else {
    // Fall back
    gemm_generic(A, B, C);
}
```

### 13.3 Cloud/Container Profiles

Define profiles for virtualized environments where certain features (PT, LBR) may be unavailable:

* `-fdsllvm-profile=cloud-generic` (no PT/LBR, limited PMU)
* `-fdsllvm-profile=container-restricted` (no virtualization features)

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| v1.0 | 2025-12-07 | DSMIL Kernel Team | Initial CPU feature model specification for Meteor Lake DSMIL node |

---

**End of Specification**
