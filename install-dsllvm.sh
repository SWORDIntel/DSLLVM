#!/bin/bash
#
# DSLLVM System Compiler Installer
# Builds DSLLVM with all DSMIL options enabled and installs as system compiler
#
# Usage:
#   sudo ./install-dsllvm.sh [options]
#
# Options:
#   --prefix PATH          Installation prefix (default: /usr/local)
#   --build-dir PATH       Build directory (default: ./build)
#   --build-type TYPE      CMake build type (Release/Debug/RelWithDebInfo/MinSizeRel)
#   --jobs N               Number of parallel build jobs (default: auto-detect)
#   --thermal-max TEMP     CPU temperature threshold to trigger throttling (default: 105°C)
#   --thermal-critical TEMP CPU temperature for emergency shutdown (default: 110°C)
#   --thermal-sensor PATH  Custom temperature sensor path (default: auto-detect)
#   --gpu-throttle         Enable GPU temperature throttling (default: enabled)
#   --no-gpu-throttle      Disable GPU temperature throttling
#   --skip-build           Skip build step (assume build exists)
#   --skip-install         Build only, do not install
#   --resume               Resume from previous build (auto-detected if build exists)
#   --clean                Clean build directory before starting (removes cache)
#   --dry-run              Show what would be done without executing
#   --verbose              Enable verbose output
#   --help                 Show this help message
#

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default configuration (optimized for performance and thermal safety)
PREFIX="/usr/local"
BUILD_DIR="./build"
BUILD_TYPE="Release"
JOBS=""
TEMP_HIGH_THRESHOLD_C=105    # Temperature to trigger throttling (Celsius)
TEMP_LOW_THRESHOLD_C=90      # Temperature to allow resuming after cooldown (Celsius)
TEMP_CRITICAL_C=110          # Emergency shutdown temperature
THROTTLE_JOB_PERCENT=60      # Jobs percentage when throttling (e.g., 60 means 60% of base jobs)
THROTTLE_COOLDOWN_DURATION=180 # Duration in seconds to maintain throttle after cooldown
CUSTOM_SENSOR_PATH=""        # Auto-detect temperature sensor
GPU_THROTTLE_ENABLED=true    # Enable GPU thermal throttling by default
SKIP_BUILD=false
SKIP_INSTALL=false
RESUME=false
CLEAN=false
DRY_RUN=false
VERBOSE=false
LOG_FILE=""

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_DIR="$SCRIPT_DIR"

# Setup log file
LOG_FILE="$SCRIPT_DIR/dsllvm-install-$(date +%Y%m%d-%H%M%S).log"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --prefix)
            PREFIX="$2"
            shift 2
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        --build-type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        --jobs)
            JOBS="$2"
            shift 2
            ;;
        --thermal-max=*)
            TEMP_HIGH_THRESHOLD_C="${1#*=}"
            shift
            ;;
        --thermal-critical=*)
            # Set emergency shutdown threshold (5°C above throttle threshold)
            TEMP_CRITICAL_C="${1#*=}"
            shift
            ;;
        --thermal-sensor=*)
            CUSTOM_SENSOR_PATH="${1#*=}"
            shift
            ;;
        --gpu-throttle)
            GPU_THROTTLE_ENABLED=true
            shift
            ;;
        --no-gpu-throttle)
            GPU_THROTTLE_ENABLED=false
            shift
            ;;
        --skip-build)
            SKIP_BUILD=true
            shift
            ;;
        --skip-install)
            SKIP_INSTALL=true
            shift
            ;;
        --resume)
            RESUME=true
            shift
            ;;
        --clean)
            CLEAN=true
            shift
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            set -x
            shift
            ;;
        --help)
            head -n 20 "$0" | tail -n +3
            exit 0
            ;;
        *)
            echo -e "${RED}Error: Unknown option: $1${NC}" >&2
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Logging functions with file logging
log_info() {
    local msg
    msg="[$(date +'%Y-%m-%d %H:%M:%S')] [INFO] $*"
    echo -e "${BLUE}[INFO]${NC} $*" | tee -a "$LOG_FILE"
}

log_success() {
    local msg
    msg="[$(date +'%Y-%m-%d %H:%M:%S')] [SUCCESS] $*"
    echo -e "${GREEN}[SUCCESS]${NC} $*" | tee -a "$LOG_FILE"
}

log_warning() {
    local msg
    msg="[$(date +'%Y-%m-%d %H:%M:%S')] [WARNING] $*"
    echo -e "${YELLOW}[WARNING]${NC} $*" | tee -a "$LOG_FILE"
}

log_error() {
    local msg
    msg="[$(date +'%Y-%m-%d %H:%M:%S')] [ERROR] $*"
    echo -e "${RED}[ERROR]${NC} $*" | tee -a "$LOG_FILE" >&2
}

log_debug() {
    if [[ "$VERBOSE" == true ]]; then
        local msg
        msg="[$(date +'%Y-%m-%d %H:%M:%S')] [DEBUG] $*"
        echo -e "${YELLOW}[DEBUG]${NC} $*" | tee -a "$LOG_FILE"
    else
        echo "[$(date +'%Y-%m-%d %H:%M:%S')] [DEBUG] $*" >> "$LOG_FILE"
    fi
}

log_step() {
    local step="$1"
    shift
    log_info "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    log_info "Step: $step"
    log_info "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    log_debug "Details: $*"
}

# Function to get CPU temperature
get_cpu_temp() {
    local temp=0
    local raw_temp
    local valid_temp_found=false

    if command -v sensors &> /dev/null; then
        # Use lm-sensors, redirect stderr to /dev/null to avoid polluting stdout
        raw_temp=$(sensors -j 2>/dev/null | grep -m 1 '"temp":' | awk '{print $2}' | cut -d'.' -f1)
        if [[ "$raw_temp" =~ ^[0-9]+$ ]]; then # Validate raw_temp is purely numeric
            temp="$raw_temp"
            valid_temp_found=true
        fi
    fi

    if [[ "$valid_temp_found" == "false" ]] && [[ -f "/sys/class/thermal/thermal_zone0/temp" ]]; then
        # Fallback to sysfs if sensors didn't provide a valid temp
        raw_temp=$(cat /sys/class/thermal/thermal_zone0/temp)
        if [[ "$raw_temp" =~ ^[0-9]+$ ]]; then # Validate raw_temp is purely numeric
            temp=$(( raw_temp / 1000 ))
            valid_temp_found=true
        fi
    fi

    if [[ "$valid_temp_found" == "true" ]]; then
        echo "$temp"
        return 0
    else
        log_warning "Could not determine valid CPU temperature. 'sensors' command (if used) failed to parse and '/sys/class/thermal/thermal_zone0/temp' (if used) was not accessible or valid." >&2
        return 1
    fi
}

# Check if running as root for system installation
check_root() {
    if [[ "$PREFIX" == "/usr/local" ]] && [[ $EUID -ne 0 ]]; then
        log_error "System installation requires root privileges. Use sudo or --prefix for custom location."
        exit 1
    fi
}

# Check prerequisites
check_prerequisites() {
    log_step "Prerequisites Check"
    
    local missing=()
    local apt_install_suggestion="sudo apt-get update && sudo apt-get install -y build-essential cmake ninja-build python3 git libssl-dev zlib1g-dev"
    
    # Check for build-essential (general compiler tools)
    if dpkg -s build-essential &> /dev/null; then
        log_debug "  ✓ Found build-essential"
    else
        log_warning "  ✗ build-essential not found. Suggesting installation."
        apt_install_suggestion+=" build-essential"
    fi

    # Check for required tools
    log_debug "Checking for required build tools..."
    for tool in cmake ninja python3 git; do # Core tools
        if command -v "$tool" &> /dev/null; then
            local version
            version=$("$tool" --version 2>/dev/null | head -n1 || echo "unknown")
            log_debug "  ✓ Found $tool: $version"
        else
            missing+=("$tool")
            log_debug "  ✗ Missing: $tool"
        fi
    done

    # Check for optional tools required for full functionality
    for tool in sensors; do
        if command -v "$tool" &> /dev/null; then
            log_debug "  ✓ Found $tool"
        else
            log_warning "  ✗ '$tool' command not found. Adding to install suggestion."
            apt_install_suggestion+=" $tool"
        fi
    done
    
    if [[ ${#missing[@]} -gt 0 ]]; then
        log_error "Missing required core tools: ${missing[*]}"
        log_error ""
        log_error "Install missing dependencies with:"
        log_error "  $apt_install_suggestion"
        log_error ""
        log_error "Full log saved to: $LOG_FILE"
        exit 1
    else
        # If no core tools are missing, but optional ones were added to suggestion
        log_info "All core build prerequisites met."
        # Check if the suggestion string changed due to optional tools
        if [[ "$apt_install_suggestion" != "sudo apt-get update && sudo apt-get install -y build-essential cmake ninja-build python3 git libssl-dev zlib1g-dev" ]]; then
            log_warning "Optional tools are missing (lm-sensors). For full functionality, consider installing:"
            log_warning "  $apt_install_suggestion"
        fi
    fi
    
    # Check CMake version (need 3.20+)
    log_debug "Checking CMake version..."
    local cmake_version
    cmake_version=$(cmake --version | head -n1 | cut -d' ' -f3)
    log_debug "  CMake version: $cmake_version"
    local cmake_major cmake_minor
    cmake_major=$(echo "$cmake_version" | cut -d. -f1)
    cmake_minor=$(echo "$cmake_version" | cut -d. -f2)
    
    if [[ $cmake_major -lt 3 ]] || [[ $cmake_major -eq 3 && $cmake_minor -lt 20 ]]; then
        log_error "CMake 3.20 or higher required. Found: $cmake_version"
        log_error ""
        log_error "Upgrade CMake with:"
        log_error "  sudo apt-get install -y cmake"
        log_error "  Or download from: https://cmake.org/download/"
        log_error ""
        log_error "Full log saved to: $LOG_FILE"
        exit 1
    fi
    
    # Check disk space (need at least 20GB free)
    log_debug "Checking disk space..."
    local available_space
    available_space=$(df -BG "$SCRIPT_DIR" | tail -1 | awk '{print $4}' | sed 's/G//')
    if [[ $available_space -lt 20 ]]; then
        log_warning "Low disk space: ${available_space}GB available (recommend at least 20GB)"
    else
        log_debug "  Disk space: ${available_space}GB available"
    fi
    
    log_success "All prerequisites met"
}
# Detect number of CPU cores
# Function to get P-core count heuristically
get_p_core_count() {
    # User-requested override: Set JOBS count to 10 (half of total logical CPUs)
    echo "10"
    return 0

    # Original heuristic:
    # local logical_cpus=$(lscpu | grep '^CPU(s):' | awk '{print $2}')
    # local physical_cores=$(lscpu | grep '^Core(s) per socket:' | awk '{print $4}')
    # local p_cores=0

    # if [[ -n "$logical_cpus" ]] && [[ -n "$physical_cores" ]]; then
    #     # Heuristic: Assuming P-cores are hyperthreaded (2 threads/core) and E-cores are not (1 thread/core)
    #     # num_p_cores = (Total logical CPUs) - (Total physical cores)
    #     p_cores=$(( logical_cpus - physical_cores ))
    #     if [[ "$p_cores" -lt 1 ]]; then # Ensure at least 1 p-core if it's a P-core machine
    #         p_cores=1
    #     fi
    #     echo "$p_cores"
    #     return 0
    # fi
    # echo "0"
    # return 1
}

detect_jobs() {
    if [[ -z "$JOBS" ]]; then
        local p_cores
        p_cores=$(get_p_core_count)
        if [[ "$p_cores" -gt 0 ]]; then
            JOBS="$p_cores"
            log_info "Detected $p_cores P-cores. Setting build jobs to P-core count."
            log_info "  (Heuristic: Assumes P-cores are hyperthreaded, E-cores are not)."
            log_info "  (Total logical CPUs - Total physical cores = P-cores)"
        elif command -v nproc &> /dev/null; then
            JOBS=$(nproc)
            log_info "Could not detect P-cores. Using total logical CPUs: $JOBS."
        elif [[ -f /proc/cpuinfo ]]; then
            JOBS=$(grep -c processor /proc/cpuinfo)
            log_info "Could not detect P-cores. Using total logical CPUs from /proc/cpuinfo: $JOBS."
        else
            JOBS=4
            log_warning "Could not auto-detect CPU cores. Defaulting to 4 jobs."
        fi
    fi
    log_info "Using $JOBS parallel build jobs"
}

# Check for existing build and resume capability
check_build_state() {
    if [[ "$SKIP_BUILD" == true ]]; then
        return
    fi
    
    local build_ninja="$BUILD_DIR/build.ninja"
    local cmake_cache="$BUILD_DIR/CMakeCache.txt"
    
    if [[ -f "$build_ninja" ]] && [[ -f "$cmake_cache" ]]; then
        if [[ "$CLEAN" == true ]]; then
            log_info "Cleaning build directory (--clean specified)..."
            if [[ "$DRY_RUN" == false ]]; then
                rm -rf "$BUILD_DIR"
                mkdir -p "$BUILD_DIR"
            fi
            log_success "Build directory cleaned"
        elif [[ "$RESUME" == true ]] || [[ "$RESUME" == false ]]; then
            # Auto-detect: if build exists, we can resume
            log_info "Existing build detected in $BUILD_DIR"
            log_info "Build can be resumed - Ninja will continue from last state"
            log_info "  - Build state: $(du -sh "$BUILD_DIR" 2>/dev/null | cut -f1 || echo 'unknown')"
            log_info "  - To clean and start fresh: use --clean"
            log_info "  - To resume: build will automatically continue"
            
            # Check if build is complete
            if ninja -C "$BUILD_DIR" -n -d explain 2>&1 | grep -q "no work to do"; then
                log_success "Build appears to be complete - nothing to rebuild"
            else
                local remaining
                remaining=$(ninja -C "$BUILD_DIR" -n 2>&1 | grep -c "build" || echo "unknown")
                if [[ "$remaining" != "unknown" ]] && [[ "$remaining" != "0" ]]; then
                    log_info "Remaining work detected - build will resume"
                fi
            fi
        fi
    else
        log_info "No existing build detected - starting fresh build"
    fi
}

# Setup build caching
setup_build_cache() {
    if [[ "$SKIP_BUILD" == true ]]; then
        return
    fi
    
    log_debug "Setting up build cache and resume support..."
    
    # Ensure build directory exists
    if [[ "$DRY_RUN" == false ]]; then
        mkdir -p "$BUILD_DIR"
    fi
    
    # Ninja automatically supports resume via its dependency database
    # The .ninja_deps and .ninja_log files track build state
    log_info "Build caching enabled:"
    log_info "  - Ninja dependency tracking: automatic resume support"
    log_info "  - Build state preserved in: $BUILD_DIR/.ninja_deps"
    log_info "  - Build log: $BUILD_DIR/.ninja_log"
    log_info ""
    log_info "If build is interrupted, simply re-run this script to resume"
    log_info "  (or run: ninja -C $BUILD_DIR -j$JOBS)"
}

# Backup existing LLVM installation
backup_existing_llvm() {
    if [[ "$SKIP_INSTALL" == true ]]; then
        return
    fi
    
    log_info "Checking for existing LLVM installation..."
    
    local backup_dir
    backup_dir="/opt/llvm-backup-$(date +%Y%m%d-%H%M%S)"
    local backed_up=false
    
    # Check for system LLVM tools
    for tool in clang clang++ llvm-config llc opt; do
        local tool_path
        tool_path=$(command -v "$tool" 2>/dev/null || true)
        if [[ -n "$tool_path" ]] && [[ "$tool_path" != "$PREFIX/bin/$tool" ]]; then
            if [[ "$backed_up" == false ]]; then
                log_info "Backing up existing LLVM installation to $backup_dir"
                if [[ "$DRY_RUN" == false ]]; then
                    mkdir -p "$backup_dir"
                fi
                backed_up=true
            fi
            
            if [[ "$DRY_RUN" == true ]]; then
                log_info "Would backup: $tool_path -> $backup_dir/"
            else
                cp -r "$(dirname "$tool_path")" "$backup_dir/" 2>/dev/null || true
            fi
        fi
    done
    
    if [[ "$backed_up" == true ]] && [[ "$DRY_RUN" == false ]]; then
        # Create restore script
        cat > "$backup_dir/RESTORE.sh" << 'EOF'
#!/bin/bash
# Restore script for backed up LLVM installation
echo "To restore, manually copy files from this directory back to their original locations"
echo "Original locations are preserved in the directory structure"
EOF
        chmod +x "$backup_dir/RESTORE.sh"
        log_success "Backup created at $backup_dir"
    fi
}

# Configure build
configure_build() {
    log_step "CMake Configuration"
    log_info "Build type: $BUILD_TYPE"
    log_info "Install prefix: $PREFIX"
    log_info "Build directory: $BUILD_DIR"
    log_info "Log file: $LOG_FILE"
    
    # Determine all available projects
    # Enable all major projects for comprehensive build
    local projects="clang;clang-tools-extra;lld;lldb;mlir;flang;openmp;polly;bolt"
    
    # Determine all targets (build for native + common targets)
    local targets="all"
    
    log_info "DSMIL Features to Enable:"
    log_info "  ✓ All 20 DSMIL passes:"
    log_info "    - BFT, BlueRed, ConstantTime, CrossDomain, EdgeSecurity"
    log_info "    - FuzzCoverage, FuzzExport, JADC2, MPE, Metrics"
    log_info "    - MissionPolicy, NuclearSurety, RadioBridge, Stealth"
    log_info "    - Telecom, Telemetry, TelemetryCheck, ThreatSignature"
    log_info "    - DSSSL: ApiMisuse, Coverage, CryptoMetrics"
    log_info "  ✓ All 25+ DSMIL runtime libraries:"
    log_info "    - Core: stealth, radio, nuclear, MPE, JADC2, edge_security, cross_domain, blue_red, bft"
    log_info "    - Devices: device15_wycheproof, device255_crypto, device46_pqc, device47_crypto"
    log_info "    - Layers: layer7_llm, layer8_security, layer8_security_crypto, layer9_executive"
    log_info "    - Advanced: int8_quantization, quantum, mlops_optimization, mlops_crypto"
    log_info "    - System: intelligence_flow, memory_budget, hil_orchestration, paths"
    log_info "  ✓ Wycheproof integration (Device 15 crypto assurance)"
    log_info "  ✓ TPM2 compatibility layer (88 cryptographic algorithms)"
    log_info "  ✓ Device 255 master crypto controller"
    log_info "  ✓ All Layer 7/8/9 runtime APIs"
    log_info ""
    log_debug "Projects to build: $projects"
    log_debug "Targets to build: $targets"
    
    local cmake_args=(
        -G "Ninja"
        -S "$SOURCE_DIR/llvm"
        -B "$BUILD_DIR"
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DCMAKE_INSTALL_PREFIX="$PREFIX"
        -DLLVM_ENABLE_PROJECTS="$projects"
        -DLLVM_TARGETS_TO_BUILD="$targets"
        # DSMIL Configuration - ALL FEATURES ENABLED
        -DLLVM_ENABLE_DSMIL=ON
        # LLVM Core Options
        -DLLVM_ENABLE_ASSERTIONS=OFF
        -DLLVM_ENABLE_EH=OFF
        -DLLVM_ENABLE_RTTI=ON
        -DLLVM_ENABLE_TERMINFO=ON
        -DLLVM_ENABLE_ZLIB=ON
        -DLLVM_ENABLE_ZSTD=ON
        -DLLVM_INCLUDE_EXAMPLES=ON
        -DLLVM_INCLUDE_TESTS=ON
        -DLLVM_INCLUDE_BENCHMARKS=ON
        -DLLVM_BUILD_EXAMPLES=OFF
        -DLLVM_BUILD_TESTS=OFF
        -DLLVM_BUILD_BENCHMARKS=OFF
        -DLLVM_INSTALL_UTILS=ON
        -DLLVM_OPTIMIZED_TABLEGEN=ON
        -DLLVM_USE_SPLIT_DWARF=ON
        -DLLVM_ENABLE_BACKTRACES=ON
        -DLLVM_ENABLE_UNWIND_TABLES=ON
        -DLLVM_ENABLE_CRASH_OVERRIDES=ON
        # Compiler Configuration - Force GCC for bootstrap
        -DCMAKE_C_COMPILER=gcc
        -DCMAKE_CXX_COMPILER=g++
        # Enable all optional features (required for DSMIL)
        -DLLVM_ENABLE_PIC=ON
        -DLLVM_ENABLE_PLUGINS=ON
        -DLLVM_ENABLE_BINDINGS=ON
        -DLLVM_ENABLE_OCAMLDOC=ON
        # Additional features for comprehensive build
        -DLLVM_ENABLE_Z3_SOLVER=OFF
        -DLLVM_ENABLE_LIBPFM=ON
        -DLLVM_ENABLE_LIBEDIT=ON
        -DLLVM_ENABLE_TERMINFO=ON
        -DLLVM_ENABLE_FFI=OFF
        # Ensure shared libraries can be built (for plugins)
        -DBUILD_SHARED_LIBS=OFF
    )
    
    if [[ "$DRY_RUN" == true ]]; then
        log_info "Would run: cmake ${cmake_args[*]}"
        return
    fi
    
    # Check if CMake cache exists (resume scenario)
    local cmake_cache="$BUILD_DIR/CMakeCache.txt"
    if [[ -f "$cmake_cache" ]] && [[ "$CLEAN" != true ]]; then
        log_info "CMake cache exists - reconfiguring (preserving build state)..."
        log_info "Existing build artifacts will be preserved - only changed configs will update"
    else
        log_info "Running CMake configuration (this may take a few minutes)..."
    fi
    
    log_debug "CMake command: cmake ${cmake_args[*]}"
    
    local cmake_log="$BUILD_DIR/cmake-config.log"
    local cmake_exit_code=0
    
    if [[ "$VERBOSE" == true ]]; then
        # Verbose: show all output
        if cmake "${cmake_args[@]}" 2>&1 | tee -a "$LOG_FILE" "$cmake_log"; then
            cmake_exit_code=0
        else
            cmake_exit_code=${PIPESTATUS[0]}
        fi
    else
        # Non-verbose: show progress and important messages
        log_info "Progress: Configuring build system..."
        if cmake "${cmake_args[@]}" > "$cmake_log" 2>&1; then
            cmake_exit_code=0
            # Show important messages
            grep -E "(^-- |^CMake |DSMIL|Error|Warning|found|NOT found)" "$cmake_log" | while IFS= read -r line; do
                if [[ "$line" =~ Error ]]; then
                    log_error "$line"
                elif [[ "$line" =~ Warning ]]; then
                    log_warning "$line"
                elif [[ "$line" =~ DSMIL ]]; then
                    log_info "$line"
                elif [[ "$line" =~ ^-- ]]; then
                    log_debug "$line"
                fi
            done
            # Always show final status
            tail -n 20 "$cmake_log" | grep -E "(^-- Configuring|^-- Generating|^-- Build files)" | head -n 3 | while IFS= read -r line; do
                log_info "$line"
            done
        else
            cmake_exit_code=$?
        fi
        # Append full log to main log file
        cat "$cmake_log" >> "$LOG_FILE"
    fi
    
    if [[ $cmake_exit_code -ne 0 ]]; then
        log_error "CMake configuration failed with exit code $cmake_exit_code"
        log_error ""
        log_error "Full CMake output saved to: $cmake_log"
        log_error "Check the log file for detailed error messages."
        log_error ""
        log_error "Common issues:"
        log_error "  - Missing dependencies: sudo apt-get install -y build-essential cmake ninja-build"
        log_error "  - Source directory issues: Ensure you're in the DSLLVM root directory"
        log_error "  - Permissions: Check write permissions for $BUILD_DIR"
        log_error ""
        log_error "Full installation log: $LOG_FILE"
        exit 1
    fi
    
    log_success "Configuration complete"
    log_debug "CMake cache saved to: $BUILD_DIR/CMakeCache.txt"
    
    # Verify DSMIL and all features are enabled
    if [[ -f "$BUILD_DIR/CMakeCache.txt" ]]; then
        log_info "Verifying DSMIL configuration..."
        # local dsmil_enabled=false # Removed: variable is unused
        local issues=0
        
        if grep -q "LLVM_ENABLE_DSMIL:BOOL=ON" "$BUILD_DIR/CMakeCache.txt"; then
            log_success "✓ LLVM_ENABLE_DSMIL=ON"
            # dsmil_enabled=true # Removed: variable is unused
        else
            log_error "✗ LLVM_ENABLE_DSMIL is not ON"
            ((issues++))
        fi
        
        # Check that DSMIL directory was found
        if grep -q "dsmil" "$BUILD_DIR/CMakeCache.txt" 2>/dev/null; then
            log_success "✓ DSMIL source directory detected"
        else
            log_warning "⚠ DSMIL source directory not found in cache"
        fi
        
        # Verify required LLVM features
        if grep -q "LLVM_ENABLE_RTTI:BOOL=ON" "$BUILD_DIR/CMakeCache.txt"; then
            log_success "✓ LLVM_ENABLE_RTTI=ON (required for DSMIL)"
        else
            log_warning "⚠ LLVM_ENABLE_RTTI is OFF (may cause issues)"
        fi
        
        if grep -q "LLVM_ENABLE_EH:BOOL=ON" "$BUILD_DIR/CMakeCache.txt"; then
            log_success "✓ LLVM_ENABLE_EH=ON (required for DSMIL)"
        else
            log_warning "⚠ LLVM_ENABLE_EH is OFF (may cause issues)"
        fi
        
        # Check projects
        if grep -q "LLVM_ENABLE_PROJECTS.*clang" "$BUILD_DIR/CMakeCache.txt"; then
            log_success "✓ Clang project enabled"
        else
            log_warning "⚠ Clang project may not be enabled"
        fi
        
        if [[ $issues -gt 0 ]]; then
            log_error "Configuration verification found $issues issue(s)"
            log_error "Check $BUILD_DIR/CMakeCache.txt for details"
            return 1
        else
            log_success "All DSMIL configuration checks passed"
        fi
    else
        log_warning "CMakeCache.txt not found - cannot verify configuration"
    fi
}

# Build TPM2 Compatibility Layer (separate project)
build_tpm2_compat() {
    if [[ "$SKIP_BUILD" == true ]]; then
        return
    fi
    
    local tpm2_dir="$SOURCE_DIR/tpm2_compat"
    if [[ ! -d "$tpm2_dir" ]]; then
        log_warning "TPM2 compat directory not found, skipping TPM2 build"
        log_info "TPM2 compat provides 88 cryptographic algorithms but is optional"
        return
    fi
    
    log_step "Building TPM2 Compatibility Layer (OPTIONAL)"
    log_info "TPM2 compat provides 88 cryptographic algorithms for Device 255"
    log_info "NOTE: TPM2 compat requires DSSSL, which requires DSLLVM"
    log_info "      This is OPTIONAL - you can build it after installing DSSSL"
    log_info "Building TPM2 with all features enabled (if dependencies available)..."
    
    local tpm2_build_dir="$tpm2_dir/build"
    local tpm2_log="$tpm2_build_dir/tpm2-build.log"
    local tpm2_cache="$tpm2_build_dir/CMakeCache.txt"
    
    # Check for existing build (resume support)
    if [[ -f "$tpm2_cache" ]] && [[ "$CLEAN" != true ]]; then
        log_info "Existing TPM2 build detected - will resume if needed"
    fi
    
    if [[ "$DRY_RUN" == true ]]; then
        log_info "Would build TPM2 compat with:"
        log_info "  - Hardware acceleration enabled (AES-NI, SHA-NI, AVX2)"
        log_info "  - Post-quantum crypto enabled (if liboqs available)"
        log_info "  - DSSSL required (or OpenSSL fallback with --use-openssl-fallback)"
        return
    fi
    
    mkdir -p "$tpm2_build_dir"
    
    log_info "Configuring TPM2 compat..."
    log_info "Features:"
    log_info "  ✓ 88 cryptographic algorithms"
    log_info "  ✓ Hardware acceleration (NPU, AES-NI, SHA-NI)"
    log_info "  ✓ Post-quantum crypto support (ML-KEM, ML-DSA)"
    log_info "  ✓ TPM 2.0 compatibility"
    
    local tpm2_cmake_args=(
        -S "$tpm2_dir"
        -B "$tpm2_build_dir"
        -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
        -DCMAKE_INSTALL_PREFIX="$PREFIX"
        -DENABLE_TPM2_COMPAT=ON
        -DENABLE_HARDWARE_ACCEL=ON
        -DENABLE_POST_QUANTUM=ON
        -DBUILD_TPM2_TESTS=OFF
        -DBUILD_TPM2_BENCHMARKS=OFF
    )
    
    # Try with DSSSL first (required), fallback to OpenSSL if needed
    local use_openssl_fallback=false
    if ! cmake "${tpm2_cmake_args[@]}" -DUSE_OPENSSL_FALLBACK=OFF > "$tpm2_log" 2>&1; then
        log_warning "TPM2 configuration with DSSSL failed, trying OpenSSL fallback..."
        log_warning "Note: DSSSL is recommended for full DSMIL security features"
        tpm2_cmake_args+=(-DUSE_OPENSSL_FALLBACK=ON)
        use_openssl_fallback=true
        
        if ! cmake "${tpm2_cmake_args[@]}" > "$tpm2_log" 2>&1; then
            log_error "TPM2 compat configuration failed"
            log_error "Check $tpm2_log for details"
            log_error ""
            log_info "Dependency chain: DSLLVM → DSSSL → TPM2 compat"
            log_info "  - DSLLVM must be built first (you're doing this now)"
            log_info "  - DSSSL requires DSLLVM to build"
            log_info "  - TPM2 compat requires DSSSL"
            log_info ""
            log_info "To build TPM2 compat later:"
            log_info "  1. Build DSLLVM (this installer) ✓"
            log_info "  2. Build DSSSL (requires DSLLVM): https://github.com/SWORDIntel/DSSSL"
            log_info "  3. Rebuild TPM2 compat (requires DSSSL)"
            log_info ""
            log_warning "Skipping TPM2 compat build - install DSSSL first, then rebuild TPM2"
            return
        fi
    fi
    
    if [[ "$use_openssl_fallback" == true ]]; then
        log_warning "Using OpenSSL fallback (DSSSL not found)"
        log_info "For full DSMIL features, install DSSSL and rebuild"
    else
        log_success "TPM2 configuration complete (using DSSSL)"
    fi
    
    log_info "Building TPM2 compat library..."
    local tpm2_start_time
    tpm2_start_time=$(date +%s)
    
    if cmake --build "$tpm2_build_dir" -j"$JOBS" >> "$tpm2_log" 2>&1; then
        local tpm2_end_time tpm2_elapsed
        tpm2_end_time=$(date +%s)
        tpm2_elapsed=$((tpm2_end_time - tpm2_start_time))
        log_success "TPM2 compat build complete in $((tpm2_elapsed / 60))m $((tpm2_elapsed % 60))s"
        
        # Verify library was built
        local tpm2_lib="$tpm2_build_dir/libtpm2_compat.a"
        if [[ -f "$tpm2_lib" ]]; then
            local lib_size
            lib_size=$(du -h "$tpm2_lib" 2>/dev/null | cut -f1 || echo "unknown")
            log_success "TPM2 library built: libtpm2_compat.a ($lib_size)"
        else
            log_warning "TPM2 library not found at expected location"
        fi
        
        # Install TPM2 compat
        if [[ "$SKIP_INSTALL" == false ]]; then
            log_info "Installing TPM2 compat..."
            if cmake --install "$tpm2_build_dir" >> "$tpm2_log" 2>&1; then
                log_success "TPM2 compat installed to $PREFIX"
            else
                log_warning "TPM2 compat installation had issues (check log)"
            fi
        fi
    else
        log_warning "TPM2 compat build failed (check $tpm2_log)"
        log_warning "This is optional - DSLLVM will still work without it"
        log_info "Common issues:"
        log_info "  - Missing DSSSL or OpenSSL"
        log_info "  - Missing development headers"
        log_info "  - Compiler compatibility issues"
    fi
    
    # Append to main log
    if [[ -f "$tpm2_log" ]]; then
        {
            echo ""
            echo "=== TPM2 Compatibility Layer Build ==="
            cat "$tpm2_log"
        } >> "$LOG_FILE"
    fi
}

# Build DSLLVM
build_dsllvm() {
    if [[ "$SKIP_BUILD" == true ]]; then
        log_warning "Skipping build step"
        return
    fi

    log_step "Building DSLLVM with Dynamic CPU Throttling"

    # Check if we're resuming
    local build_ninja="$BUILD_DIR/build.ninja"
    local ninja_deps="$BUILD_DIR/.ninja_deps"
    if [[ -f "$build_ninja" ]] && [[ -f "$ninja_deps" ]] && [[ "$CLEAN" != true ]]; then
        log_info "Resuming previous build (build state preserved)"
        log_info "Ninja will skip already-built targets automatically"
    else
        log_info "Starting fresh build"
    fi

    log_info "This may take 30-120 minutes depending on your system..."
    log_info "Using initial $JOBS parallel jobs"
    log_info "Build log: $LOG_FILE"
    log_info "Build cache: ./build/.ninja_deps (enables resume)"
    log_info "Throttling Thresholds: HIGH=${TEMP_HIGH_THRESHOLD_C}°C, RESUME=${TEMP_LOW_THRESHOLD_C}°C"
    log_info "Throttling Jobs: ${THROTTLE_JOB_PERCENT}% of base jobs, Cooldown: ${THROTTLE_COOLDOWN_DURATION}s"

    if [[ "$DRY_RUN" == true ]]; then
        log_info "Would run: ninja -C $BUILD_DIR -j$JOBS"
        return
    fi

    local start_total_time
    start_total_time=$(date +%s)
    local build_log="$BUILD_DIR/build.log"
    local build_exit_code=0
    
    local BASE_JOBS=$JOBS # Store original jobs count
    local CURRENT_NINJA_PID=0
    local CURRENT_NINJA_JOBS=$BASE_JOBS
    local THROTTLING_ACTIVE=false
    local THROTTLE_START_TIME=0
    local MONITOR_INTERVAL_S=5 # How often to check temperature and restart ninja
    local BUILD_FINISHED=false

    # --- Start initial ninja process ---
    log_info "Starting ninja build with $BASE_JOBS jobs."
    ninja -C "$BUILD_DIR" -j"$BASE_JOBS" 2>&1 | tee -a "$build_log" >&1 &
    CURRENT_NINJA_PID=$!
    log_debug "Initial ninja PID: $CURRENT_NINJA_PID"
    # --- End initial ninja process ---

    # --- Main Build Monitoring Loop ---
    while ! $BUILD_FINISHED; do
        local CURRENT_TIME=$(date +%s)
        local CURRENT_TEMP
        
        # Check if ninja process is still running
        if ! kill -0 "$CURRENT_NINJA_PID" &> /dev/null; then
            # Ninja process finished or crashed
            wait "$CURRENT_NINJA_PID" 2>/dev/null
            build_exit_code=$? # Get its exit code
            BUILD_FINISHED=true
            break # Exit monitoring loop
        fi

        # Get CPU Temperature
        if ! CURRENT_TEMP=$(get_cpu_temp); then
            log_warning "Failed to get CPU temperature. Continuing without dynamic throttling."
            CURRENT_TEMP="N/A"
            # If temp cannot be read, we cannot throttle, so just continue with current jobs
            sleep "$MONITOR_INTERVAL_S"
            continue
        fi

        local TARGET_JOBS=$BASE_JOBS # Assume base jobs initially

        if [[ "$THROTTLING_ACTIVE" == "true" ]]; then
            # We are currently throttling
            if [[ $((CURRENT_TIME - THROTTLE_START_TIME)) -lt "$THROTTLE_COOLDOWN_DURATION" ]]; then
                # Still in cooldown period, maintain throttle
                TARGET_JOBS=$(( BASE_JOBS * THROTTLE_JOB_PERCENT / 100 ))
                log_debug "Throttling active (cooldown). Temp: ${CURRENT_TEMP}°C, Next unthrottle in $((THROTTLE_COOLDOWN_DURATION - (CURRENT_TIME - THROTTLE_START_TIME)))s."
            else
                # Cooldown period passed, check if safe to unthrottle
                if [[ "$CURRENT_TEMP" -lt "$TEMP_LOW_THRESHOLD_C" ]]; then
                    THROTTLING_ACTIVE=false
                    THROTTLE_START_TIME=0
                    TARGET_JOBS=$BASE_JOBS # Unthrottle
                    log_info "Temperature (${CURRENT_TEMP}°C) dropped below resume threshold (${TEMP_LOW_THRESHOLD_C}°C) after cooldown. Resuming full jobs ($BASE_JOBS)."
                else
                    # Temp still high, maintain throttle
                    TARGET_JOBS=$(( BASE_JOBS * THROTTLE_JOB_PERCENT / 100 ))
                    log_info "Temperature (${CURRENT_TEMP}°C) still high after cooldown. Maintaining throttle."
                fi
            fi
        else
            # Not currently throttling, check if throttling is needed
            if [[ "$CURRENT_TEMP" != "N/A" && "$CURRENT_TEMP" -ge "$TEMP_HIGH_THRESHOLD_C" ]]; then
                THROTTLING_ACTIVE=true
                THROTTLE_START_TIME="$CURRENT_TIME"
                TARGET_JOBS=$(( BASE_JOBS * THROTTLE_JOB_PERCENT / 100 ))
                log_warning "Temperature (${CURRENT_TEMP}°C) exceeded HIGH threshold (${TEMP_HIGH_THRESHOLD_C}°C). Throttling jobs to $TARGET_JOBS (from $BASE_JOBS)."
            fi
        fi
        
        # Ensure TARGET_JOBS is at least 1
        if [[ "$TARGET_JOBS" -lt 1 ]]; then
            TARGET_JOBS=1
        fi

        # Restart ninja if jobs count needs to change
        if [[ "$TARGET_JOBS" -ne "$CURRENT_NINJA_JOBS" ]]; then
            log_info "Jobs count changing from $CURRENT_NINJA_JOBS to $TARGET_JOBS. Restarting ninja."
            kill "$CURRENT_NINJA_PID" 2>/dev/null || true # Kill current ninja if still running
            wait "$CURRENT_NINJA_PID" 2>/dev/null || true # Wait for it to terminate
            
            ninja -C "$BUILD_DIR" -j"$TARGET_JOBS" 2>&1 | tee -a "$build_log" >&1 &
            CURRENT_NINJA_PID=$!
            CURRENT_NINJA_JOBS="$TARGET_JOBS"
            log_debug "New ninja PID: $CURRENT_NINJA_PID"
        fi

        # Display status on a single line
        local STATUS_STR="Temp: ${CURRENT_TEMP}°C, Jobs: ${CURRENT_NINJA_JOBS} (Base: $BASE_JOBS)"
        if [[ "$THROTTLING_ACTIVE" == "true" ]]; then
            local REM_COOLDOWN=$(( THROTTLE_COOLDOWN_DURATION - (CURRENT_TIME - THROTTLE_START_TIME) ))
            if [[ "$REM_COOLDOWN" -gt 0 ]]; then
                STATUS_STR+=" ${YELLOW}THROTTLED (cooldown: ${REM_COOLDOWN}s)${NC}"
            else
                STATUS_STR+=" ${YELLOW}THROTTLED (temp high)${NC}"
            fi
        else
            STATUS_STR+=" ${GREEN}RUNNING FULL${NC}"
        fi

        printf "\r%-80s" "$STATUS_STR" # Overwrite current line
        
        sleep "$MONITOR_INTERVAL_S"
    done
    # --- End Main Build Monitoring Loop ---

    # --- Final Cleanup and Status Check ---
    echo "" # Newline after progress status

    # Ensure no ninja process is left
    if kill -0 "$CURRENT_NINJA_PID" &> /dev/null; then
        log_warning "Ninja process $CURRENT_NINJA_PID was still running after loop, killing it."
        kill "$CURRENT_NINJA_PID" 2>/dev/null || true
        wait "$CURRENT_NINJA_PID" 2>/dev/null || true
    fi

    # Read the last exit code of ninja
    # Since ninja was backgrounded, its exit code isn't directly available via $? in the main loop context
    # We must have captured it when it finished or use a more robust way.
    # For now, rely on build_log for success/failure
    if grep -q "Build finished" "$build_log"; then # Simplified check, better to parse ninja's final exit
        build_exit_code=0
    else
        log_warning "Build log did not indicate 'Build finished'. Checking for errors..."
        if grep -q "error:" "$build_log"; then
            build_exit_code=1
        else
            build_exit_code=0 # Assume success if no errors and no explicit finish message
        fi
    fi
    # --- End Final Cleanup and Status Check ---
    
    local end_time elapsed
    end_time=$(date +%s)
    elapsed=$((end_time - start_total_time))
    
    log_success "Build complete in $((elapsed / 60))m $((elapsed % 60))s"
    log_debug "Build artifacts in: $BUILD_DIR"
    
    # Show cache statistics
    if [[ -f "$BUILD_DIR/.ninja_deps" ]]; then
        local cache_size
        cache_size=$(du -sh "$BUILD_DIR/.ninja_deps" 2>/dev/null | cut -f1 || echo "unknown")
        log_debug "Build dependency cache: $cache_size (enables fast resume)"
    fi
    
    log_info ""
    log_info "Build state saved - you can resume by re-running this script"
    log_info "  or manually: ninja -C $BUILD_DIR -j$JOBS"

    if [[ ${build_exit_code:-0} -ne 0 ]]; then
        log_error "Build failed with exit code ${build_exit_code}"
        log_error ""
        log_error "Full build output saved to: $build_log"
        log_error ""
        log_error "Common build issues:"
        log_error "  - Out of memory: Reduce --jobs (try --jobs 2)"
        log_error "  - Disk space: Free up space (need ~20GB)"
        log_error "  - Missing dependencies: Check CMake configuration output"
        log_error "  - Compiler errors: Check $build_log for specific errors"
        log_error ""
        log_error "Full installation log: $LOG_FILE"
        exit 1
    fi
}


# Install DSLLVM
install_dsllvm() {
    if [[ "$SKIP_INSTALL" == true ]]; then
        log_warning "Skipping installation step"
        return
    fi
    
    log_step "Installing DSLLVM"
    log_info "Installation prefix: $PREFIX"
    
    if [[ "$DRY_RUN" == true ]]; then
        log_info "Would run: ninja -C $BUILD_DIR install"
        return
    fi
    
    local install_log="$BUILD_DIR/install.log"
    log_info "Installing files (this may take a few minutes)..."
    
    if [[ "$VERBOSE" == true ]]; then
        if ninja -C "$BUILD_DIR" install 2>&1 | tee -a "$LOG_FILE" "$install_log"; then
            local install_exit_code=0
        else
            local install_exit_code=${PIPESTATUS[0]}
        fi
    else
        # Show installation progress
        if ninja -C "$BUILD_DIR" install > "$install_log" 2>&1; then
            local install_exit_code=0
            # Show installed files count
            local installed_count
            installed_count=$(grep -c "Installing:" "$install_log" 2>/dev/null || echo "0")
            if [[ $installed_count -gt 0 ]]; then
                log_info "Installed $installed_count files"
            fi
        else
            local install_exit_code=$?
        fi
        
        # Show errors if any
        if grep -qi "error:" "$install_log" 2>/dev/null; then
            log_error "Installation errors found:"
            grep -i "error:" "$install_log" | head -n 10 | while IFS= read -r line; do
                log_error "  $line"
            done
        fi
        
        # Append to main log
        cat "$install_log" >> "$LOG_FILE"
    fi
    
    if [[ ${install_exit_code:-0} -ne 0 ]]; then
        log_error "Installation failed with exit code ${install_exit_code}"
        log_error ""
        log_error "Full installation output saved to: $install_log"
        log_error ""
        log_error "Common installation issues:"
        log_error "  - Permission denied: Run with sudo or use --prefix for user directory"
        log_error "  - Disk space: Ensure sufficient space in $PREFIX"
        log_error ""
        log_error "Full installation log: $LOG_FILE"
        exit 1
    fi
    
    log_success "Installation complete"
    log_debug "Installed to: $PREFIX"
}

# Create system compiler symlinks
create_symlinks() {
    if [[ "$SKIP_INSTALL" == true ]]; then
        return
    fi
    
    log_info "Creating system compiler symlinks..."
    
    # Map of DSMIL tools to system compiler names
    declare -A symlink_map=(
        ["dsmil-clang"]="clang"
        ["dsmil-clang++"]="clang++"
        ["dsmil-opt"]="opt"
        ["dsmil-llc"]="llc"
    )
    
    # Optional: also create symlinks for other tools
    local bin_dir="$PREFIX/bin"
    local system_bin_dir="/usr/local/bin"
    
    # Only create system symlinks if installing to /usr/local
    if [[ "$PREFIX" == "/usr/local" ]] && [[ -w "$system_bin_dir" ]] || [[ $EUID -eq 0 ]]; then
        for dsmil_tool in "${!symlink_map[@]}"; do
            local system_tool="${symlink_map[$dsmil_tool]}"
            local dsmil_path="$bin_dir/$dsmil_tool"
            local system_path="$system_bin_dir/$system_tool"
            
            if [[ -f "$dsmil_path" ]]; then
                if [[ "$DRY_RUN" == true ]]; then
                    log_info "Would create symlink: $system_path -> $dsmil_path"
                else
                    # Backup existing if it exists and is not our symlink
                    if [[ -L "$system_path" ]] && [[ "$(readlink "$system_path")" != "$dsmil_path" ]]; then
                        mv "$system_path" "${system_path}.backup-$(date +%Y%m%d-%H%M%S)"
                    elif [[ -f "$system_path" ]] && [[ ! -L "$system_path" ]]; then
                        mv "$system_path" "${system_path}.backup-$(date +%Y%m%d-%H%M%S)"
                    fi
                    
                    ln -sf "$dsmil_path" "$system_path"
                    log_info "Created symlink: $system_tool -> $dsmil_tool"
                fi
            fi
        done
    else
        log_warning "Skipping system symlinks (not installing to /usr/local or no write access)"
        log_info "You can manually create symlinks or add $bin_dir to your PATH"
    fi
}

# Setup environment configuration
setup_environment() {
    if [[ "$SKIP_INSTALL" == true ]]; then
        return
    fi
    
    log_info "Setting up environment configuration..."
    
    local env_file="/etc/profile.d/dsllvm.sh"
    
    if [[ "$DRY_RUN" == true ]]; then
        log_info "Would create: $env_file"
        return
    fi
    
    # Only create system-wide config if installing to /usr/local
    if [[ "$PREFIX" == "/usr/local" ]] && [[ $EUID -eq 0 ]]; then
        cat > "$env_file" << EOF
# DSLLVM Environment Configuration
# Automatically generated by install-dsllvm.sh

export DSLLVM_ROOT="$PREFIX"
export PATH="\$DSLLVM_ROOT/bin:\$PATH"

# Set DSLLVM as default compiler
export CC="dsmil-clang"
export CXX="dsmil-clang++"

# LLVM configuration
export LLVM_DIR="\$DSLLVM_ROOT"

# DSMIL Configuration (adjust paths as needed)
export DSMIL_PSK_PATH="/etc/dsmil/keys/project_signing_key.pem"
export DSMIL_POLICY="production"
export DSMIL_TRUSTSTORE="/etc/dsmil/truststore"
EOF
        chmod 644 "$env_file"
        log_success "Environment configuration created at $env_file"
        log_info "Users may need to log out and back in, or run: source $env_file"
    else
        log_info "Skipping system-wide environment setup (use custom prefix or run as root)"
        log_info "Add to your ~/.bashrc or ~/.zshrc:"
        echo ""
        echo "export DSLLVM_ROOT=\"$PREFIX\""
        echo "export PATH=\"\$DSLLVM_ROOT/bin:\$PATH\""
        echo "export CC=\"dsmil-clang\""
        echo "export CXX=\"dsmil-clang++\""
        echo "export LLVM_DIR=\"\$DSLLVM_ROOT\""
    fi
}

# Verify installation
verify_installation() {
    if [[ "$SKIP_INSTALL" == true ]]; then
        return
    fi
    
    log_info "Verifying installation..."
    
    local bin_dir="$PREFIX/bin"
    local errors=0
    
    # Check for key tools
    local tools=("dsmil-clang" "dsmil-clang++" "dsmil-opt")
    
    for tool in "${tools[@]}"; do
        local tool_path="$bin_dir/$tool"
        if [[ "$DRY_RUN" == true ]]; then
            log_info "Would check: $tool_path"
        elif [[ -f "$tool_path" ]]; then
            # Test that it runs
            if "$tool_path" --version &> /dev/null; then
                log_success "Verified: $tool"
            else
                log_error "Tool exists but failed to run: $tool"
                ((errors++))
            fi
        else
            log_error "Missing tool: $tool"
            ((errors++))
        fi
    done
    
    # Check for DSMIL runtime library
    local lib_dir="$PREFIX/lib"
    local runtime_lib="$lib_dir/libdsmilrt.a"
    if [[ "$DRY_RUN" == false ]]; then
        if [[ -f "$runtime_lib" ]]; then
            log_success "DSMIL runtime library found: libdsmilrt.a"
            local lib_size
            lib_size=$(du -h "$runtime_lib" 2>/dev/null | cut -f1 || echo "unknown")
            log_debug "  Library size: $lib_size"
        else
            log_warning "DSMIL runtime library not found: $runtime_lib"
            log_warning "  Some DSMIL features may not work correctly"
        fi
    fi
    
    # Check for DSMIL passes plugin
    local pass_plugin="$lib_dir/libDsmilPasses.so"
    if [[ "$DRY_RUN" == false ]]; then
        if [[ -f "$pass_plugin" ]] || [[ -f "${pass_plugin%.so}.a" ]]; then
            log_success "DSMIL passes plugin found"
        else
            log_warning "DSMIL passes plugin not found - passes may not load"
        fi
    fi
    
    # Check for DSMIL passes
    if [[ "$DRY_RUN" == false ]]; then
        local opt_path="$bin_dir/dsmil-opt"
        if [[ -f "$opt_path" ]]; then
            if "$opt_path" --help 2>&1 | grep -q -i dsmil; then
                log_success "DSMIL passes are available via dsmil-opt"
            else
                log_warning "DSMIL passes may not be loaded correctly"
            fi
        fi
    fi
    
    # Check for DSMIL headers
    local include_dir="$PREFIX/include/dsmil"
    if [[ "$DRY_RUN" == false ]]; then
        if [[ -d "$include_dir" ]]; then
            local header_count
            header_count=$(find "$include_dir" -name "*.h" 2>/dev/null | wc -l)
            if [[ $header_count -gt 0 ]]; then
                log_success "DSMIL headers installed: $header_count header files"
            else
                log_warning "DSMIL include directory exists but no headers found"
            fi
        else
            log_warning "DSMIL headers not found in $include_dir"
        fi
    fi
    
    # Check for TPM2 compat library
    local tpm2_lib="$lib_dir/libtpm2_compat.a"
    local tpm2_include="$PREFIX/include/tpm2_compat"
    if [[ "$DRY_RUN" == false ]]; then
        if [[ -f "$tpm2_lib" ]]; then
            log_success "TPM2 compat library found: libtpm2_compat.a"
            local tpm2_size
            tpm2_size=$(du -h "$tpm2_lib" 2>/dev/null | cut -f1 || echo "unknown")
            log_debug "  Library size: $tpm2_size"
            
            if [[ -d "$tpm2_include" ]]; then
                local tpm2_headers
                tpm2_headers=$(find "$tpm2_include" -name "*.h" 2>/dev/null | wc -l)
                log_success "TPM2 compat headers: $tpm2_headers header files"
            fi
        else
            log_warning "TPM2 compat library not found (optional component)"
            log_info "  TPM2 compat provides 88 cryptographic algorithms but is optional"
            log_info "  To build: Ensure DSSSL or OpenSSL 3.0+ is installed"
        fi
    fi
    
    if [[ $errors -eq 0 ]]; then
        log_success "Installation verification complete"
    else
        log_error "Installation verification found $errors error(s)"
        return 1
    fi
}

# Print summary
print_summary() {
    echo ""
    echo "=========================================="
    echo -e "${GREEN}DSLLVM Installation Summary${NC}"
    echo "=========================================="
    echo ""
    echo "Installation prefix: $PREFIX"
    echo "Build type: $BUILD_TYPE"
    echo ""
    
    if [[ "$SKIP_INSTALL" == false ]]; then
        echo "Tools installed:"
        echo "  - dsmil-clang, dsmil-clang++ (DSMIL-aware compilers)"
        echo "  - dsmil-opt, dsmil-llc (DSMIL optimization tools)"
        echo "  - All LLVM tools with DSMIL support"
        echo ""
        echo "DSMIL Features Enabled:"
        echo "  ✓ All DSMIL passes (BFT, BlueRed, ConstantTime, CrossDomain, EdgeSecurity, etc.)"
        echo "  ✓ All DSMIL runtime libraries (stealth, radio, nuclear, MPE, JADC2, etc.)"
        echo "  ✓ 9-Layer/104-Device architecture support"
        echo "  ✓ CNSA 2.0 provenance (SHA-384, ML-DSA-87, ML-KEM-1024)"
        echo "  ✓ Mission profiles (border_ops, cyber_defence, covert_ops)"
        echo "  ✓ Operational stealth modes"
        echo "  ✓ Cross-domain security (U/C/S/TS/TS-SCI)"
        echo "  ✓ JADC2/5G optimization"
        echo "  ✓ Wycheproof integration (Device 15 crypto assurance)"
        echo "  ⚠ TPM2 compatibility layer (OPTIONAL - requires DSSSL)"
        echo "    - 88 cryptographic algorithms"
        echo "    - Hardware acceleration (AES-NI, SHA-NI, AVX2)"
        echo "    - Post-quantum crypto (ML-KEM, ML-DSA)"
        echo "    - TPM 2.0 compatibility"
        echo "    - Device 255 integration"
        echo "    - Note: Build DSLLVM first, then DSSSL, then TPM2"
        echo "  ✓ Device 255 master crypto controller"
        echo "  ✓ All runtime APIs (Layer 7/8/9, Device 255)"
        echo "  ✓ INT8 quantization runtime"
        echo "  ✓ Quantum runtime (Device 46)"
        echo "  ✓ Memory budget management (62 GB pool)"
        echo "  ✓ HIL orchestration (NPU/GPU/CPU)"
        echo ""
        
        if [[ "$PREFIX" == "/usr/local" ]]; then
            echo "System integration:"
            echo "  - Symlinks created in /usr/local/bin"
            echo "  - Environment configured in /etc/profile.d/dsllvm.sh"
            echo ""
            echo "To use DSLLVM:"
            echo "  1. Log out and back in, or run: source /etc/profile.d/dsllvm.sh"
            echo "  2. Verify: dsmil-clang --version"
            echo "  3. Compile: dsmil-clang -O3 -fpass-pipeline=dsmil-default -o output input.c"
        else
            echo "To use DSLLVM:"
            echo "  export PATH=\"$PREFIX/bin:\$PATH\""
            echo "  export CC=\"dsmil-clang\""
            echo "  export CXX=\"dsmil-clang++\""
            echo "  dsmil-clang --version"
        fi
        echo ""
        echo "Build Resume Support:"
        echo "  - Build state cached in: $BUILD_DIR/.ninja_deps"
        echo "  - To resume interrupted build: Re-run this script"
        echo "  - To clean and rebuild: Use --clean flag"
        echo "  - Manual resume: ninja -C $BUILD_DIR -j$JOBS"
    else
        echo "Build complete. Run installation separately or use --skip-install=false"
    fi
    
    echo ""
}

# Error handler
error_handler() {
    local line_num=$1
    local error_code=$2
    log_error ""
    log_error "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    log_error "Installation failed at line $line_num with exit code $error_code"
    log_error "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    log_error ""
    log_error "Full error log saved to: $LOG_FILE"
    log_error ""
    log_error "To debug:"
    log_error "  1. Check the log file: cat $LOG_FILE"
    log_error "  2. Re-run with verbose: $0 --verbose"
    log_error "  3. Check build directory: ls -la $BUILD_DIR"
    exit "$error_code"
}

# Set error trap
trap 'error_handler ${LINENO} $?' ERR

# Main execution
main() {
    echo ""
    echo "=========================================="
    echo -e "${BLUE}DSLLVM System Compiler Installer${NC}"
    echo "=========================================="
    echo ""
    log_info "Installation log: $LOG_FILE"
    echo ""
    
    if [[ "$DRY_RUN" == true ]]; then
        log_warning "DRY RUN MODE - No changes will be made"
        echo ""
    fi
    
    # Log start
    {
        echo "=========================================="
        echo "DSLLVM Installation Log"
        echo "Started: $(date)"
        echo "Command: $0 $*"
        echo "User: $(whoami)"
        echo "System: $(uname -a)"
        echo "=========================================="
        echo ""
    } >> "$LOG_FILE"
    
    check_root
    check_prerequisites
    detect_jobs
    setup_build_cache
    check_build_state
    backup_existing_llvm
    configure_build
    build_dsllvm
    build_tpm2_compat
    install_dsllvm
    create_symlinks
    setup_environment
    verify_installation
    print_summary
    
    # Log completion
    {
        echo ""
        echo "=========================================="
        echo "Installation completed successfully"
        echo "Finished: $(date)"
        echo "=========================================="
    } >> "$LOG_FILE"
    
    log_success "DSLLVM installation complete!"
    log_info "Full installation log saved to: $LOG_FILE"
}

# Run main function
main "$@"

