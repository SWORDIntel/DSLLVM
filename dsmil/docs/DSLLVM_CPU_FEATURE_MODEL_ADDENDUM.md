# DSLLVM CPU Feature Model – Addendum (Tier 2 Features)

**Version**: 1.1  
**Date**: 2025-12-07  
**Extends**: `DSLLVM_CPU_FEATURE_MODEL.md`

---

## Purpose

This addendum extends the base CPU feature model with additional features that provide high value for specific DSLLVM use cases.

---

## Tier 2 Features: Cryptography Acceleration

### `sha_ni` – SHA Extensions

**Description**: Hardware acceleration for SHA-1 and SHA-256 hash functions.

**DSLLVM Integration**:

* **Constant-Time Crypto**: `dsmil-ct-check` pass can use `sha_ni` for constant-time SHA operations.
* **Provenance Hashing**: `dsmil-provenance-pass` can use SHA-NI for fast SHA-384 hashing (fallback when dedicated SHA-384 accelerator unavailable).
* **AI Integrity**: Layer 7/8 AI can use SHA-NI for model weight integrity checks.

**Compiler Flag**: `-msha` (enable SHA intrinsics)

**Pass Usage**:
- `dsmil-ct-check`: Prefer `__builtin_ia32_sha256*` intrinsics when `sha_ni` present
- `dsmil-provenance-pass`: Use SHA-NI for hash computation

**Metadata**:
```llvm
!dsllvm.crypto.sha_ni = i1 true
```

---

### `pclmulqdq` – Carry-Less Multiplication

**Description**: PCLMULQDQ instruction for polynomial multiplication (used in AES-GCM, CRC).

**DSLLVM Integration**:

* **AES-GCM**: Layer 8 crypto workers use `pclmulqdq` for GHASH in AES-GCM mode.
* **CRC32**: Fast CRC32c for checksums in provenance and telemetry.
* **Crypto Agility**: Enables constant-time AEAD implementations.

**Compiler Flag**: `-mpclmul` (enable PCLMULQDQ intrinsics)

**Pass Usage**:
- `dsmil-ct-check`: Prefer `__builtin_ia32_pclmulqdq128` for AES-GCM
- `dsmil-crypto-optimize`: Emit PCLMULQDQ-based GHASH

**Metadata**:
```llvm
!dsllvm.crypto.pclmulqdq = i1 true
```

---

### `rdrand` / `rdseed` – Hardware RNG

**Description**:
- `rdrand`: RDRAND instruction (DRBG-based random number generator)
- `rdseed`: RDSEED instruction (direct entropy source)

**DSLLVM Integration**:

* **Quantum RNG Fallback**: When Layer 7 Device 46 quantum RNG is unavailable, use `rdseed` as high-quality entropy source.
* **Nonce Generation**: `dsmil-provenance-pass` uses `rdrand` for nonce generation in signatures.
* **Key Generation**: Crypto workers seed key generation with `rdseed`.

**Compiler Flags**: `-mrdrnd`, `-mrdseed`

**Pass Usage**:
- `dsmil-provenance-pass`: Use `rdrand` for nonce generation
- `dsmil-crypto-keygen`: Prefer `rdseed` over `/dev/urandom` when available
- `dsmil-quantum-fallback`: Use `rdseed` when quantum RNG offline

**Metadata**:
```llvm
!dsllvm.rng.rdrand = i1 true
!dsllvm.rng.rdseed = i1 true
```

**Policy**:
- Production builds (`DSMIL_PRODUCTION`): MUST have either quantum RNG OR `rdseed`
- Lab builds: `rdrand` is acceptable
- `rdseed` preferred over `rdrand` (higher entropy quality)

---

## Tier 2 Features: Memory & Cache Management

### `clflushopt` – Optimized Cache Flush

**Description**: CLFLUSHOPT instruction (optimized CLFLUSH, allows parallel cache line flushing).

**DSLLVM Integration**:

* **Constant-Time Crypto**: Flush cache lines to prevent cache timing attacks on secret data.
* **Persistent Memory**: Flush data to PM before fence operations.
* **Side-Channel Defense**: Clear sensitive data from cache after crypto operations.

**Compiler Flag**: `-mclflushopt`

**Pass Usage**:
- `dsmil-ct-check`: Insert `clflushopt` after operations on `dsmil_secret` data
- `dsmil-crypto-cleanup`: Flush key material from cache on function exit
- `dsmil-pm-sync`: Use for persistent memory synchronization

**Metadata**:
```llvm
!dsllvm.cache.clflushopt = i1 true
```

**Example**:
```c
__attribute__((dsmil_secret))
void aes_encrypt(uint8_t *key, ...) {
    // ... crypto operations ...
    
    // Flush key from cache (constant-time cleanup)
    _mm_clflushopt(key);
    _mm_mfence();
}
```

---

### `clwb` – Cache Line Write-Back

**Description**: CLWB instruction (write-back cache line without invalidate).

**DSLLVM Integration**:

* **Persistent Memory**: Efficient PM synchronization (write-back without evicting from cache).
* **Telemetry**: Flush telemetry data to backing store without cache pollution.

**Compiler Flag**: `-mclwb`

**Pass Usage**:
- `dsmil-pm-sync`: Prefer `clwb` over `clflushopt` for PM (better performance)
- `dsmil-telemetry-flush`: Efficient telemetry buffer flushing

**Metadata**:
```llvm
!dsllvm.cache.clwb = i1 true
```

**Note**: `clwb` is more efficient than `clflushopt` for PM because it doesn't invalidate the cache line.

---

## Tier 2 Features: Timing & Profiling

### `constant_tsc` / `nonstop_tsc` – Reliable TSC

**Description**:
- `constant_tsc`: TSC increments at constant rate (unaffected by frequency scaling)
- `nonstop_tsc`: TSC continues in deep C-states (power-saving modes)

**DSLLVM Integration**:

* **Profiling**: Intel PT, LBR, PEBS rely on accurate TSC for timestamps.
* **Benchmarking**: `dsmil-bandwidth-estimate` uses TSC for micro-benchmarks.
* **Telemetry**: Accurate timing for OT telemetry (see `OT-TELEMETRY-GUIDE.md`).

**Pass Usage**:
- `dsmil-prof-instrument`: Verify `constant_tsc` present before enabling PT/LBR
- `dsmil-bandwidth-estimate`: Use RDTSC for micro-benchmarks only if `constant_tsc`
- `dsmil-telemetry-timing`: Emit TSC-based timestamps if `nonstop_tsc`

**Metadata**:
```llvm
!dsllvm.timing.constant_tsc = i1 true
!dsllvm.timing.nonstop_tsc = i1 true
```

**Policy**:
- Profiling modes (`-fdsllvm-prof=pt|lbr|pebs`) REQUIRE `constant_tsc`
- Telemetry with accurate timestamps REQUIRES `nonstop_tsc`

---

### `tsc_deadline_timer` – TSC Deadline Mode

**Description**: APIC timer can trigger interrupts based on TSC value (more precise than periodic mode).

**DSLLVM Integration**:

* **Informational Only**: DSLLVM doesn't directly use this (OS-level feature).
* **Metadata**: Record in provenance for system characterization.

**Pass Usage**: None (OS-level)

**Metadata**:
```llvm
!dsllvm.timing.tsc_deadline = i1 true  // informational
```

---

## Tier 2 Features: TLB & Paging

### `pcid` / `invpcid` – Process-Context Identifiers

**Description**:
- `pcid`: Tag TLB entries with process ID (reduces TLB flushes on context switch)
- `invpcid`: Invalidate specific PCID entries

**DSLLVM Integration**:

* **Performance Modeling**: `dsmil-bandwidth-estimate` can model TLB pressure more accurately when `pcid` present.
* **Hypervisor Builds**: Hint that TLB management is more efficient.

**Pass Usage**:
- `dsmil-bandwidth-estimate`: Adjust TLB miss penalty when `pcid` present
- `dsmil-device-placement`: Prefer CPU for TLB-sensitive workloads if `pcid` available

**Metadata**:
```llvm
!dsllvm.tlb.pcid = i1 true
```

**Priority**: LOW (minor performance impact, informational)

---

### `pdpe1gb` – 1GB Page Support

**Description**: CPU supports 1GB huge pages (in addition to 4KB/2MB).

**DSLLVM Integration**:

* **Hypervisor Builds**: `-fdsllvm-profile=mtr-mtl-hv` can use 1GB pages for EPT mappings.
* **Large Model Mapping**: Layer 7 LLMs can use 1GB pages to reduce TLB pressure (70B model = ~140 GB = 140 pages).
* **Bandwidth Optimization**: Fewer TLB misses for large sequential memory access.

**Compiler Flag**: `-Wl,-z,max-page-size=1073741824` (linker hint)

**Pass Usage**:
- `dsmil-device-placement`: Recommend 1GB pages for models >10GB when `pdpe1gb` present
- `dsmil-hypervisor-hints`: Emit 1GB EPT alignment hints

**Metadata**:
```llvm
!dsllvm.paging.pdpe1gb = i1 true
```

**Priority**: MEDIUM (useful for large AI models)

---

## Tier 2 Features: Power Management

### `hwp` / `hwp_epp` – Hardware P-States

**Description**:
- `hwp`: Hardware-Controlled Performance States (CPU manages frequency)
- `hwp_epp`: Energy Performance Preference (hint to HWP)

**DSLLVM Integration**:

* **Informational**: Track HWP availability for HFI integration.
* **Auto-Tuning**: Layer 5 performance AI can query HWP state via HFI.

**Pass Usage**:
- `dsmil-prof-hfi`: Check `hwp` to determine if HFI hints are actionable
- `dsmil-power-model`: Record HWP state in provenance

**Metadata**:
```llvm
!dsllvm.power.hwp = i1 true  // informational
```

**Priority**: LOW (OS-level, informational only)

---

## Tier 2 Features: Architecture Enumeration

### `arch_capabilities` – Architecture Capabilities MSR

**Description**: IA32_ARCH_CAPABILITIES MSR enumerates architectural security features and mitigations available.

**DSLLVM Integration**:

* **Security Posture**: Read MSR to determine which speculation mitigations are active.
* **Spec Hardening**: `dsmil-spec-hardening` can query `arch_capabilities` to decide which mitigations to rely on.

**Pass Usage**:
- `dsmil-spec-hardening`: Read MSR 0x10A to check RDCL_NO, IBRS_ALL, RSBA, etc.
- `dsmil-provenance-pass`: Record arch_capabilities bits in provenance

**Metadata**:
```llvm
!dsllvm.security.arch_capabilities = i1 true
```

**MSR Bits** (relevant to DSLLVM):
- `RDCL_NO` (bit 0): Not susceptible to Meltdown
- `IBRS_ALL` (bit 1): IBRS applies to all privilege levels
- `RSBA` (bit 2): RSB may use alternate predictors
- `SKIP_L1DFL_VMENTRY` (bit 3): No need to flush L1D on VM entry
- `SSB_NO` (bit 4): Not susceptible to Spectre v4
- `MDS_NO` (bit 5): Not susceptible to MDS
- `TAA_NO` (bit 8): Not susceptible to TAA

**Priority**: HIGH (security assumptions)

---

## Updated CPU Profile JSON

Add these features to `config/cpu/mtr-mtl-dsmil.json`:

```json
{
  "features": {
    "crypto": [
      "sha_ni",
      "pclmulqdq",
      "rdrand",
      "rdseed"
    ],
    "cache": [
      "clflushopt",
      "clwb"
    ],
    "timing": [
      "constant_tsc",
      "nonstop_tsc",
      "tsc_deadline_timer"
    ],
    "tlb": [
      "pcid",
      "invpcid",
      "pdpe1gb"
    ],
    "power": [
      "hwp",
      "hwp_epp"
    ],
    "arch": [
      "arch_capabilities"
    ]
  },
  
  "feature_descriptions": {
    "sha_ni": "SHA-1/SHA-256 hardware acceleration",
    "pclmulqdq": "Carry-less multiplication for AES-GCM/CRC",
    "rdrand": "Hardware DRBG random number generator",
    "rdseed": "Direct entropy source (higher quality than rdrand)",
    "clflushopt": "Optimized cache line flush (prevents cache timing attacks)",
    "clwb": "Cache line write-back without invalidate (persistent memory)",
    "constant_tsc": "TSC increments at constant rate (reliable timing)",
    "nonstop_tsc": "TSC continues in deep C-states (always-on timing)",
    "tsc_deadline_timer": "APIC timer deadline mode",
    "pcid": "Process-Context Identifiers (reduces TLB flushes)",
    "invpcid": "Invalidate PCID (paired with pcid)",
    "pdpe1gb": "1GB page support (reduces TLB pressure for large models)",
    "hwp": "Hardware P-states (informational)",
    "hwp_epp": "HWP energy performance preference (informational)",
    "arch_capabilities": "IA32_ARCH_CAPABILITIES MSR (security feature enumeration)"
  }
}
```

---

## Pass Integration Summary

| Pass | New Features Used | Purpose |
|------|-------------------|---------|
| `dsmil-ct-check` | `sha_ni`, `pclmulqdq`, `clflushopt` | Constant-time crypto with hardware acceleration |
| `dsmil-crypto-optimize` | `sha_ni`, `pclmulqdq`, `rdrand`, `rdseed` | Crypto performance and RNG |
| `dsmil-crypto-cleanup` | `clflushopt`, `clwb` | Cache flushing for side-channel defense |
| `dsmil-bandwidth-estimate` | `constant_tsc`, `pcid`, `pdpe1gb` | Accurate performance modeling |
| `dsmil-prof-instrument` | `constant_tsc`, `nonstop_tsc` | Reliable profiling timestamps |
| `dsmil-spec-hardening` | `arch_capabilities` | Query MSR for security posture |
| `dsmil-provenance-pass` | `sha_ni`, `rdrand`, `arch_capabilities` | Fast hashing, nonce generation, security metadata |
| `dsmil-device-placement` | `pdpe1gb` | Large model TLB optimization |

---

## Policy Updates

### Production Builds (`DSMIL_PRODUCTION`)

```
MUST have ONE OF:
  - Quantum RNG (Layer 7 Device 46) active
  - rdseed present and functioning
  
MUST have BOTH:
  - constant_tsc (for profiling/telemetry)
  - arch_capabilities (for security enumeration)

SHOULD have:
  - sha_ni (crypto performance)
  - pclmulqdq (AES-GCM performance)
  - clflushopt (side-channel defense)
```

### Crypto Workloads (`dsmil_sandbox("crypto_worker")`)

```
MUST have:
  - clflushopt OR clwb (cache flushing for constant-time)
  - rdseed OR rdrand (RNG)
  
SHOULD have:
  - sha_ni (hash performance)
  - pclmulqdq (AEAD performance)
```

### Profiling Modes (`-fdsllvm-prof=pt|lbr|pebs`)

```
MUST have:
  - constant_tsc (accurate timestamps)
  
SHOULD have:
  - nonstop_tsc (profiling in power-save modes)
```

---

## Verification

Add to `tools/verify-cpu-integration.sh`:

```bash
# Check for Tier 2 features
if jq -e '.features.crypto | index("sha_ni")' config/cpu/mtr-mtl-dsmil.json >/dev/null; then
    echo "✓ SHA-NI support documented"
fi

if jq -e '.features.timing | index("constant_tsc")' config/cpu/mtr-mtl-dsmil.json >/dev/null; then
    echo "✓ Constant TSC support documented"
fi
```

---

## Document History

| Version | Date | Author | Changes |
|---------|------|--------|---------|
| v1.0 | 2025-12-07 | DSMIL Kernel Team | Initial Tier 2 features |

---

**END OF ADDENDUM**
