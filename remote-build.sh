#!/bin/bash
#
# DSLLVM Remote Build Offloader
# Offloads DSLLVM compilation to remote host with monitoring
#
# Usage:
#   ./remote-build.sh [options]
#
# Options:
#   --host HOST          Remote host (default: 38.102.85.235)
#   --user USER          SSH user (default: dsmil)
#   --port PORT          SSH port (default: 22)
#   --key KEY_FILE       SSH private key file
#   --repo-url URL       GitHub repo URL (default: https://github.com/SWORDIntel/DSLLVM.git)
#   --branch BRANCH      Git branch to build (default: main)
#   --build-dir DIR      Remote build directory (default: ~/dsllvm-build)
#   --install-deps       Install system dependencies
#   --skip-cleanup       Keep remote build directory after completion
#   --monitor-interval N Monitor interval in seconds (default: 30)
#   --help               Show this help
#

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Default configuration
REMOTE_HOST="38.102.85.235"
REMOTE_USER="dsmil"
REMOTE_PORT="22"
SSH_KEY=""
REPO_URL="https://github.com/SWORDIntel/DSLLVM.git"
BRANCH="main"
REMOTE_BUILD_DIR="~/dsllvm-build"
INSTALL_DEPS=false
SKIP_CLEANUP=false
MONITOR_INTERVAL=30

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Detect local system architecture and generate build flags
detect_local_system() {
    # Detect architecture
    local arch
    case "$(uname -m)" in
        x86_64)
            arch="x86_64"
            ;;
        aarch64|arm64)
            arch="aarch64"
            ;;
        armv7l|armv7)
            arch="armv7"
            ;;
        riscv64)
            arch="riscv64"
            ;;
        ppc64le)
            arch="ppc64le"
            ;;
        s390x)
            arch="s390x"
            ;;
        *)
            arch="$(uname -m)"
            ;;
    esac

    # Detect OS
    local os
    if [[ -f /etc/os-release ]]; then
        os="$(grep '^ID=' /etc/os-release | cut -d'=' -f2 | tr -d '"')"
    else
        os="$(uname -s | tr '[:upper:]' '[:lower:]')"
    fi

    # Generate target triple
    local triple="${arch}-pc-${os}"

    # Generate CMake flags optimized for local system
    local cmake_flags=""

    # Architecture-specific optimizations
    case "$arch" in
        x86_64)
            cmake_flags="$cmake_flags -DLLVM_TARGETS_TO_BUILD=X86"
            cmake_flags="$cmake_flags -DLLVM_ENABLE_X86_TARGETS=ON"
            cmake_flags="$cmake_flags -DCMAKE_CXX_FLAGS=-march=native"
            cmake_flags="$cmake_flags -DCMAKE_C_FLAGS=-march=native"
            ;;
        aarch64)
            cmake_flags="$cmake_flags -DLLVM_TARGETS_TO_BUILD=AArch64"
            cmake_flags="$cmake_flags -DLLVM_ENABLE_AARCH64_TARGETS=ON"
            cmake_flags="$cmake_flags -DCMAKE_CXX_FLAGS=-march=armv8-a"
            cmake_flags="$cmake_flags -DCMAKE_C_FLAGS=-march=armv8-a"
            ;;
        armv7)
            cmake_flags="$cmake_flags -DLLVM_TARGETS_TO_BUILD=ARM"
            cmake_flags="$cmake_flags -DLLVM_ENABLE_ARM_TARGETS=ON"
            cmake_flags="$cmake_flags -DCMAKE_CXX_FLAGS=-march=armv7-a"
            cmake_flags="$cmake_flags -DCMAKE_C_FLAGS=-march=armv7-a"
            ;;
        riscv64)
            cmake_flags="$cmake_flags -DLLVM_TARGETS_TO_BUILD=RISCV"
            cmake_flags="$cmake_flags -DLLVM_ENABLE_RISCV_TARGETS=ON"
            ;;
        ppc64le)
            cmake_flags="$cmake_flags -DLLVM_TARGETS_TO_BUILD=PowerPC"
            cmake_flags="$cmake_flags -DLLVM_ENABLE_POWERPC_TARGETS=ON"
            ;;
        s390x)
            cmake_flags="$cmake_flags -DLLVM_TARGETS_TO_BUILD=SystemZ"
            cmake_flags="$cmake_flags -DLLVM_ENABLE_SYSTEMZ_TARGETS=ON"
            ;;
    esac

    # OS-specific flags
    case "$os" in
        ubuntu|debian)
            cmake_flags="$cmake_flags -DCMAKE_INSTALL_PREFIX=/usr/local"
            ;;
        centos|rhel|fedora)
            cmake_flags="$cmake_flags -DCMAKE_INSTALL_PREFIX=/usr/local"
            ;;
        alpine)
            cmake_flags="$cmake_flags -DCMAKE_INSTALL_PREFIX=/usr/local"
            ;;
    esac

    # Common optimization flags
    cmake_flags="$cmake_flags -DCMAKE_BUILD_TYPE=Release"
    cmake_flags="$cmake_flags -DLLVM_ENABLE_ASSERTIONS=OFF"
    cmake_flags="$cmake_flags -DLLVM_ENABLE_EXPENSIVE_CHECKS=OFF"
    cmake_flags="$cmake_flags -DLLVM_ENABLE_BACKTRACES=OFF"
    cmake_flags="$cmake_flags -DLLVM_INCLUDE_TESTS=OFF"
    cmake_flags="$cmake_flags -DLLVM_INCLUDE_EXAMPLES=OFF"
    cmake_flags="$cmake_flags -DLLVM_INCLUDE_BENCHMARKS=OFF"
    cmake_flags="$cmake_flags -DLLVM_BUILD_DOCS=OFF"
    cmake_flags="$cmake_flags -DLLVM_ENABLE_DOXYGEN=OFF"
    cmake_flags="$cmake_flags -DLLVM_ENABLE_SPHINX=OFF"

    # Detect CPU cores for parallel build
    local cpu_cores
    cpu_cores=$(nproc 2>/dev/null || echo "4")

    echo "LOCAL_SYSTEM_INFO=\"$arch:$os:$triple\""
    echo "CMAKE_FLAGS=\"$cmake_flags\""
    echo "CPU_CORES=\"$cpu_cores\""
}

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $*" >&2
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $*" >&2
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $*" >&2
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $*" >&2
}

# Show usage
show_help() {
    cat << EOF
DSLLVM Remote Build Offloader

Offloads DSLLVM compilation to remote host with real-time monitoring.

USAGE:
    $0 [OPTIONS]

OPTIONS:
    --host HOST          Remote host (default: $REMOTE_HOST)
    --user USER          SSH user (default: $REMOTE_USER)
    --port PORT          SSH port (default: $REMOTE_PORT)
    --key KEY_FILE       SSH private key file
    --repo-url URL       GitHub repo URL (default: $REPO_URL)
    --branch BRANCH      Git branch to build (default: $BRANCH)
    --build-dir DIR      Remote build directory (default: $REMOTE_BUILD_DIR)
    --install-deps       Install system dependencies on remote host
    --skip-cleanup       Keep remote build directory after completion
    --monitor-interval N Monitor interval in seconds (default: $MONITOR_INTERVAL)
    --help               Show this help

EXAMPLES:
    # Basic usage with defaults
    $0

    # Custom host and branch
    $0 --host my-server.com --branch development

    # Use SSH key and install dependencies
    $0 --key ~/.ssh/id_rsa --install-deps

    # Custom build directory
    $0 --build-dir ~/custom-build-dir

DEPENDENCIES INSTALLED (when --install-deps is used):
    - Build tools: build-essential, cmake, ninja-build
    - Development libraries: libssl-dev, libffi-dev, python3-dev
    - LLVM dependencies: libedit-dev, libncurses-dev, swig
    - System tools: git, curl, wget, htop, lm-sensors
    - Python packages: pip, virtualenv

MONITORING:
    The script provides real-time monitoring of:
    - Build progress and completion percentage
    - CPU usage, memory usage, disk usage
    - Temperature monitoring
    - Build log tail
    - Estimated time remaining

REMOTE BUILD PROCESS:
    1. SSH connection establishment
    2. System dependency installation (if requested)
    3. Repository cloning/checkout
    4. Build execution with thermal management
    5. Real-time monitoring and status reporting
    6. Cleanup (unless --skip-cleanup is specified)

EOF
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --host)
            REMOTE_HOST="$2"
            shift 2
            ;;
        --user)
            REMOTE_USER="$2"
            shift 2
            ;;
        --port)
            REMOTE_PORT="$2"
            shift 2
            ;;
        --key)
            SSH_KEY="$2"
            shift 2
            ;;
        --repo-url)
            REPO_URL="$2"
            shift 2
            ;;
        --branch)
            BRANCH="$2"
            shift 2
            ;;
        --build-dir)
            REMOTE_BUILD_DIR="$2"
            shift 2
            ;;
        --install-deps)
            INSTALL_DEPS=true
            shift
            ;;
        --skip-cleanup)
            SKIP_CLEANUP=true
            shift
            ;;
        --monitor-interval)
            MONITOR_INTERVAL="$2"
            shift 2
            ;;
        --help)
            show_help
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            echo "Use --help for usage information" >&2
            exit 1
            ;;
    esac
done

# Validate SSH key if provided
if [[ -n "$SSH_KEY" ]] && [[ ! -f "$SSH_KEY" ]]; then
    log_error "SSH key file not found: $SSH_KEY"
    exit 1
fi

# Build SSH command
SSH_CMD="ssh"
if [[ -n "$SSH_KEY" ]]; then
    SSH_CMD="$SSH_CMD -i $SSH_KEY"
fi
if [[ "$REMOTE_PORT" != "22" ]]; then
    SSH_CMD="$SSH_CMD -p $REMOTE_PORT"
fi
SSH_CMD="$SSH_CMD ${REMOTE_USER}@${REMOTE_HOST}"

# Test SSH connection
log_info "Testing SSH connection to ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PORT}..."
if ! $SSH_CMD "echo 'SSH connection successful'" >/dev/null 2>&1; then
    log_error "Failed to establish SSH connection to ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PORT}"
    log_error "Please check:"
    log_error "  - SSH key authentication (if using key)"
    log_error "  - Network connectivity"
    log_error "  - Firewall settings"
    log_error "  - SSH server running on remote host"
    exit 1
fi
log_success "SSH connection established"

# Function to run command on remote host
remote_exec() {
    local cmd="$1"
    log_info "Executing on remote: $cmd"
    if ! $SSH_CMD "$cmd"; then
        log_error "Remote command failed: $cmd"
        return 1
    fi
    return 0
}

# Function to install system dependencies on remote host
install_remote_deps() {
    log_info "Installing system dependencies on remote host..."

    # Update package list
    remote_exec "sudo apt-get update -qq" || {
        log_warning "Failed to update package list, continuing..."
    }

    # Install essential build tools
    remote_exec "sudo apt-get install -y -qq build-essential cmake ninja-build git curl wget" || {
        log_error "Failed to install basic build tools"
        return 1
    }

    # Install LLVM/Clang build dependencies
    remote_exec "sudo apt-get install -y -qq libssl-dev libffi-dev python3-dev libedit-dev libncurses-dev swig libxml2-dev liblzma-dev" || {
        log_error "Failed to install LLVM dependencies"
        return 1
    }

    # Install system monitoring tools
    remote_exec "sudo apt-get install -y -qq htop lm-sensors sysstat iotop" || {
        log_error "Failed to install monitoring tools"
        return 1
    }

    # Install Python pip if not available
    remote_exec "command -v pip3 >/dev/null 2>&1 || sudo apt-get install -y -qq python3-pip" || {
        log_warning "Failed to install pip3, some features may not work"
    }

    log_success "System dependencies installed on remote host"
}

# Function to setup remote build environment
setup_remote_build() {
    log_info "Setting up remote build environment..."

    # Create build directory
    remote_exec "mkdir -p $REMOTE_BUILD_DIR" || {
        log_error "Failed to create remote build directory"
        return 1
    }

    # Change to build directory and clone repo
    remote_exec "cd $REMOTE_BUILD_DIR && git clone $REPO_URL dsllvm-src" || {
        log_error "Failed to clone DSLLVM repository"
        return 1
    }

    # Checkout specified branch
    remote_exec "cd $REMOTE_BUILD_DIR/dsllvm-src && git checkout $BRANCH" || {
        log_error "Failed to checkout branch: $BRANCH"
        return 1
    }

    # Create build directory
    remote_exec "cd $REMOTE_BUILD_DIR && mkdir -p build" || {
        log_error "Failed to create build directory"
        return 1
    }

    log_success "Remote build environment setup complete"
}

# Function to get remote system info
get_remote_info() {
    log_info "Gathering remote system information..."

    echo "=== REMOTE SYSTEM INFO ==="
    remote_exec "uname -a"
    remote_exec "lsb_release -a 2>/dev/null || cat /etc/os-release"
    remote_exec "nproc && echo 'CPU cores'"
    remote_exec "free -h | head -2"
    remote_exec "df -h / | tail -1"
    remote_exec "sensors 2>/dev/null | head -10 || echo 'No sensors available'"

    echo "=== BUILD ENVIRONMENT ==="
    remote_exec "cd $REMOTE_BUILD_DIR/dsllvm-src && pwd && ls -la"
    remote_exec "which cmake && cmake --version | head -1"
    remote_exec "which ninja && ninja --version"
    remote_exec "which clang && clang --version | head -1 || echo 'Clang not found'"
    remote_exec "python3 --version"
}

# Function to start remote build
start_remote_build() {
    log_info "Starting remote DSLLVM build..."

    # Detect local system for optimized build flags
    log_info "Detecting local system architecture for optimized build..."
    eval "$(detect_local_system)"

    # Extract detected values
    local_arch=$(echo "$LOCAL_SYSTEM_INFO" | cut -d: -f1)
    local_os=$(echo "$LOCAL_SYSTEM_INFO" | cut -d: -f2)
    local_triple=$(echo "$LOCAL_SYSTEM_INFO" | cut -d: -f3)

    log_info "Local system detected: $local_arch on $local_os"
    log_info "Target triple: $local_triple"
    log_info "Using $CPU_CORES CPU cores for build"

    # Create optimized build command with local system flags
    local build_cmd="cd $REMOTE_BUILD_DIR/dsllvm-src && CPU_CORES=\"$CPU_CORES\" LOCAL_ARCH=\"$local_arch\" LOCAL_OS=\"$local_os\" TARGET_TRIPLE=\"$local_triple\" ./install-dsllvm.sh --prefix $REMOTE_BUILD_DIR/install --thermal-max 85 --thermal-critical 95 --clean --resume --skip-install --jobs $CPU_CORES --cmake-extra-flags \"$CMAKE_FLAGS\""

    # Execute build command in background
    remote_exec "nohup bash -c '$build_cmd > $REMOTE_BUILD_DIR/build.log 2>&1 &' && echo \$! > $REMOTE_BUILD_DIR/build.pid" || {
        log_error "Failed to start remote build"
        return 1
    }

    # Wait a moment for build to start
    sleep 5

    # Verify build is running
    remote_exec "ps aux | grep -v grep | grep install-dsllvm.sh" || {
        log_error "Build process not found on remote host"
        return 1
    }

    log_success "Remote DSLLVM build started (optimized for $local_arch $local_os)"
    log_info "Build target triple: $local_triple"
    log_info "Using $CPU_CORES parallel jobs with optimized flags"
}

# Function to monitor remote build
monitor_remote_build() {
    log_info "Starting remote build monitoring (interval: ${MONITOR_INTERVAL}s)..."
    log_info "Press Ctrl+C to stop monitoring"

    local start_time=$(date +%s)
    local last_log_lines=0

    while true; do
        echo
        echo "=== REMOTE BUILD STATUS $(date) ==="

        # Check if build is still running
        if ! remote_exec "ps aux | grep -v grep | grep install-dsllvm.sh >/dev/null"; then
            echo "=== BUILD COMPLETED OR STOPPED ==="
            remote_exec "cat $REMOTE_BUILD_DIR/build.log | tail -20"
            break
        fi

        # Get system stats
        echo "--- SYSTEM RESOURCES ---"
        remote_exec "echo 'CPU: ' && uptime && echo 'Memory:' && free -h | grep '^Mem:' && echo 'Disk:' && df -h $REMOTE_BUILD_DIR | tail -1"

        # Get temperature if available
        remote_exec "sensors 2>/dev/null | grep -E '(Package|Core|temp1)' | head -3 || echo 'Temperature monitoring not available'"

        # Get build progress
        echo "--- BUILD PROGRESS ---"
        remote_exec "tail -10 $REMOTE_BUILD_DIR/build.log 2>/dev/null || echo 'Build log not available yet'"

        # Calculate elapsed time
        local current_time=$(date +%s)
        local elapsed=$((current_time - start_time))
        echo "--- ELAPSED TIME: $(date -d "@$elapsed" -u +%H:%M:%S) ---"

        sleep "$MONITOR_INTERVAL"
    done
}

# Function to cleanup remote build
cleanup_remote_build() {
    if [[ "$SKIP_CLEANUP" == true ]]; then
        log_info "Skipping cleanup as requested (--skip-cleanup)"
        return 0
    fi

    log_info "Cleaning up remote build directory..."

    # Kill any remaining build processes
    remote_exec "pkill -f install-dsllvm.sh || true"

    # Remove build directory
    remote_exec "rm -rf $REMOTE_BUILD_DIR" || {
        log_warning "Failed to remove remote build directory: $REMOTE_BUILD_DIR"
    }

    log_success "Remote cleanup completed"
}

# Function to handle script interruption
cleanup_on_exit() {
    echo
    log_warning "Script interrupted, cleaning up..."
    cleanup_remote_build
    exit 1
}

# Set trap for cleanup on exit
trap cleanup_on_exit INT TERM

# Main execution
main() {
    echo
    echo "=========================================="
    echo -e "${BLUE}DSLLVM Remote Build Offloader${NC}"
    echo "=========================================="
    echo

    log_info "Remote Host: ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PORT}"
    log_info "Repository: $REPO_URL ($BRANCH)"
    log_info "Build Directory: $REMOTE_BUILD_DIR"
    echo

    # Install dependencies if requested
    if [[ "$INSTALL_DEPS" == true ]]; then
        install_remote_deps
    fi

    # Setup remote build environment
    setup_remote_build

    # Get remote system info
    get_remote_info

    # Start remote build
    start_remote_build

    # Monitor build progress
    monitor_remote_build

    # Check final build status
    echo
    echo "=== FINAL BUILD STATUS ==="
    if remote_exec "test -f $REMOTE_BUILD_DIR/install/bin/clang"; then
        log_success "DSLLVM build completed successfully!"
        remote_exec "ls -la $REMOTE_BUILD_DIR/install/bin/"
    else
        log_error "DSLLVM build appears to have failed"
        remote_exec "tail -50 $REMOTE_BUILD_DIR/build.log" || true
        exit 1
    fi

    # Cleanup
    cleanup_remote_build

    log_success "Remote build offloading completed"
}

# Run main function
main "$@"
