# DSLLVM - DS LLVM Compiler

**Version**: 1.6.0 (Phase 3: High-Assurance)
**Repository**: https://github.com/SWORDIntel/DSLLVM

---
## 🚀 Quick Links

- **[DSLLVM Build Guide](DSLLVM-BUILD-GUIDE.md)**: How to use DSLLVM as your default compiler
- **[TPM2 Algorithms](tpm2_compat/README.md)**: 88 cryptographic algorithms reference
### Upstream LLVM
- [Getting Started with LLVM](https://llvm.org/docs/GettingStarted.html)
- [Contributing to LLVM](https://llvm.org/docs/Contributing.html)

### Security Articles
- [Constant-Time Support Lands in LLVM: Protecting Cryptographic Code at the Compiler Level](https://securityboulevard.com/2025/11/constant-time-support-lands-in-llvm-protecting-cryptographic-code-at-the-compiler-level/)

### DSLLVM-Specific
**Quick Start**:
```bash
cd tpm2_compat
cmake -S . -B build -DENABLE_HARDWARE_ACCEL=ON
cmake --build build
```

## 📦 Building DSLLVM

### Quick Install (Recommended)

**Automated installer** - Builds and installs DSLLVM, replacing system LLVM:

```bash
# System-wide installation (requires sudo)
sudo ./build-dsllvm.sh

# Install to custom prefix (no sudo needed)
./build-dsllvm.sh --prefix /opt/dsllvm

# See all options
./build-dsllvm.sh --help
```

The installer automatically:
- ✅ Checks prerequisites
- ✅ Backs up existing LLVM installation
- ✅ Builds DSLLVM with all DSMIL features
- ✅ Creates system symlinks (clang → dsmil-clang, etc.)
- ✅ Sets up environment configuration
- ✅ Verifies installation

**Build Types:**
- `Release` (default): Optimized production build, faster execution, larger binaries
- `Debug`: Full debug symbols, assertions enabled, slower execution, easier debugging
- `RelWithDebInfo`: Optimized with debug info, good for profiling
- `MinSizeRel`: Optimized for smallest binary size

### Manual Build

#### Prerequisites
```bash
sudo apt-get install -y build-essential cmake ninja-build python3 git libssl-dev
```

#### Build LLVM/Clang + DSMIL
```bash
cmake -G Ninja -S llvm -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DLLVM_ENABLE_DSMIL=ON \
  -DLLVM_TARGETS_TO_BUILD="X86"

ninja -C build
```

### Build TPM2 Library
```bash
cd tpm2_compat
cmake -S . -B build -DENABLE_HARDWARE_ACCEL=ON
cmake --build build -j$(nproc)
```

---

## What DSLLVM Is

DSLLVM is a **standard LLVM/Clang toolchain** with additional telemetry and metadata collection capabilities.

- Keeps the **standard LLVM/Clang toolchain behaviour**;
- Adds **optional telemetry hooks** for build and compilation metrics;
- Can be used as a regular `clang`/`lld` toolchain with enhanced observability.

If you already know LLVM, you can treat DSLLVM as "LLVM with extra telemetry" rather than a new compiler.

---

## Highlights

- ✅ **LLVM-first design**
  - Tracks upstream LLVM closely; core passes and IR semantics are unchanged.
  - Can be used as a regular `clang`/`lld` toolchain.

- 📊 **Telemetry and metadata collection**
  - Build artefacts can carry compact telemetry metadata for:
    - performance/size profiles,
    - compilation metrics,
    - build-time observability.
  - Optional and encoded as normal IR / object metadata.

---

## What DSLLVM Is (and Is Not)

**Is:**

- A **minimally invasive** extension layer on top of LLVM/Clang/LLD.
- A way to **collect telemetry** during compilation.
- A place to keep **build metrics and metadata** close to the code that produced the binaries.

**Is *not*:**

- Not a new IR or language.
- Not a replacement for upstream security guidance or crypto libraries.
- Not a mandatory runtime or kernel – it's "just" the compiler side with telemetry.

---

## Building & Using DSLLVM

**Recommended**: Use the automated installer (`./build-dsllvm.sh`) for a complete build and system integration. See the [Build Guide](DSLLVM-BUILD-GUIDE.md) for details.

DSLLVM follows the **standard LLVM build flow**:

1. Configure with CMake (out-of-tree build directory).
2. Build with Ninja or Make.
3. Use `clang`/`clang++`/`lld` as usual.

If you don’t enable any DSMIL/AI options, DSLLVM behaves like a regular LLVM toolchain.

---

## Status

- Core compiler functionality: ✅ usable
- Telemetry and metadata collection: ✅ stable
- Downstream integrations: out of scope for this repo

For most users, DSLLVM can be dropped in as **"LLVM with extra telemetry"** and left at that.
## 📚 Documentation

**Note:** DSMIL documentation is ignored in this repository and not included in builds.

- **[DSLLVM-BUILD-GUIDE.md](DSLLVM-BUILD-GUIDE.md)**: Default compiler configuration
- **[tpm2_compat/README.md](tpm2_compat/README.md)**: TPM2 algorithms reference


[![Upstream](https://img.shields.io/badge/LLVM-upstream%20aligned-262D3A?logo=llvm&logoColor=white)](https://llvm.org/)
[![DSMIL Stack](https://img.shields.io/badge/DSMIL-multi--layer%20architecture-0B8457.svg)](#what-is-dsmil)
[![Quantum Ready](https://img.shields.io/badge/quantum-Qiskit%20%7C%20hybrid-6C2DC7.svg)](#quantum--ai-integration)
[![PQC Profile](https://img.shields.io/badge/CNSA%202.0-ML--KEM--1024%20%E2%80%A2%20ML--DSA--87%20%E2%80%A2%20SHA--384-E67E22.svg)](#pqc--security-posture)
[![AI-Integrated](https://img.shields.io/badge/AI-instrumented%20toolchain-1F7A8C.svg)](#ai--telemetry-hooks)
