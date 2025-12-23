#!/bin/bash
#
# DSLLVM Remote Build Offloader
# Offloads DSLLVM compilation to remote host with real-time monitoring
#
# Usage:
#   ./remote-build.sh [options]
#
# Options:
#   --host HOST          Remote host (default: 38.102.85.235)
#   --user USER          SSH user (default: dsmil)
#   --port PORT          SSH port (default: 22)
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
SSH_PASS="261505"
REPO_URL="https://github.com/SWORDIntel/DSLLVM.git"
BRANCH="main"
REMOTE_BUILD_DIR="~/dsllvm-build"
INSTALL_DEPS=false
SKIP_CLEANUP=false
MONITOR_INTERVAL=30

# Script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_FILE="$SCRIPT_DIR/.dsllvm-config"

# Load configuration from file if it exists
if [[ -f "$CONFIG_FILE" ]]; then
    source "$CONFIG_FILE"
fi

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
            sed -n '2,20p' "$0" | sed 's/^#//'
            exit 0
            ;;
        *)
            log_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# SSH command builder
build_ssh_cmd() {
    local host="$1"
    local user="$2"
    local port="$3"

    echo "sshpass -p '$SSH_PASS' ssh -p $port -o ConnectTimeout=10 -o StrictHostKeyChecking=no ${user}@${host}"
}

# Remote execution function
remote_exec() {
    local host="$1"
    local user="$2"
    local port="$3"
    local cmd="$4"

    local ssh_cmd
    ssh_cmd=$(build_ssh_cmd "$host" "$user" "$port")

    eval "$ssh_cmd '$cmd'"
}

# Function to install dependencies
install_dependencies() {
    log_info "Installing dependencies on remote host..."

    # Update package list
    if ! remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "sudo apt-get update -qq"; then
        log_warning "Failed to update package list"
    fi

    # Install build tools
    log_info "Installing build tools..."
    if ! remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "sudo apt-get install -y -qq build-essential cmake ninja-build git curl wget"; then
        log_error "Failed to install build tools"
        return 1
    fi

    # Install LLVM dependencies
    log_info "Installing LLVM dependencies..."
    if ! remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "sudo apt-get install -y -qq libssl-dev libffi-dev python3-dev libedit-dev libncurses-dev swig libxml2-dev liblzma-dev"; then
        log_error "Failed to install LLVM dependencies"
        return 1
    fi

    # Install monitoring tools
    log_info "Installing monitoring tools..."
    if ! remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "sudo apt-get install -y -qq htop lm-sensors sysstat iotop jq"; then
        log_warning "Failed to install monitoring tools"
    fi

    log_success "Dependencies installed successfully"
}

# Function to get remote system info
get_system_info() {
    log_info "=== REMOTE SYSTEM INFO ==="
    remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "uname -a"
    remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "lsb_release -a 2>/dev/null || cat /etc/os-release"
    remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "nproc && echo 'CPU cores'"
    remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "free -h | head -2"
    remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "df -h / | tail -1"
    remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "sensors 2>/dev/null | head -10 || echo 'No sensors available'"
}

# Function to start remote build
start_remote_build() {
    log_info "Starting remote DSLLVM build..."

    # Create build directory
    remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "mkdir -p $REMOTE_BUILD_DIR" || {
        log_error "Failed to create remote build directory"
        return 1
    }

    # Change to build directory and clone repo
    log_info "Cloning DSLLVM repository (this may take a few minutes)..."
    remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "cd $REMOTE_BUILD_DIR && git clone $REPO_URL dsllvm-src" || {
        log_error "Failed to clone DSLLVM repository"
        return 1
    }

    # Checkout specified branch
    remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "cd $REMOTE_BUILD_DIR/dsllvm-src && git checkout $BRANCH" || {
        log_error "Failed to checkout branch: $BRANCH"
        return 1
    }

    # Create build directory
    remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "cd $REMOTE_BUILD_DIR && mkdir -p build" || {
        log_error "Failed to create build directory"
        return 1
    }

    # Start build in background on remote host
    local build_cmd="cd $REMOTE_BUILD_DIR/dsllvm-src && ./install-dsllvm.sh --prefix $REMOTE_BUILD_DIR/install --thermal-max 85 --thermal-critical 95 --clean --resume --skip-install"

    # Execute build command in background
    remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "nohup bash -c '$build_cmd > $REMOTE_BUILD_DIR/build.log 2>&1 &' && echo \$! > $REMOTE_BUILD_DIR/build.pid" || {
        log_error "Failed to start remote build"
        return 1
    }

    # Wait a moment for build to start
    sleep 5

    # Verify build is running
    remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "ps aux | grep -v grep | grep install-dsllvm.sh" || {
        log_error "Build process not found on remote host"
        return 1
    }

    log_success "Remote DSLLVM build started"
}

# Function to monitor remote build
monitor_remote_build() {
    log_info "Starting remote build monitoring (interval: ${MONITOR_INTERVAL}s)..."
    log_info "Press Ctrl+C to stop monitoring"

    local start_time=$(date +%s)
    local last_log_lines=0

    while true; do
        clear

        echo "=========================================="
        echo "DSLLVM Remote Build Monitor"
        echo "=========================================="
        echo "Host: $REMOTE_USER@$REMOTE_HOST:$REMOTE_PORT"
        echo "Build Dir: $REMOTE_BUILD_DIR"
        echo "Branch: $BRANCH"
        echo "Start Time: $(date -d "@$start_time")"
        echo "Elapsed: $(( $(date +%s) - start_time )) seconds"
        echo ""

        # Check if build is still running
        if remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "ps aux | grep -v grep | grep -q install-dsllvm.sh"; then
            echo -e "${GREEN}✓ Build Status: RUNNING${NC}"
        else
            echo -e "${YELLOW}⚠ Build Status: COMPLETED OR STOPPED${NC}"

            # Check if build completed successfully
            if remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "test -f $REMOTE_BUILD_DIR/install/bin/clang"; then
                echo -e "${GREEN}✓ Build appears to have completed successfully!${NC}"
                echo ""
                echo "Installed binaries:"
                remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "ls -la $REMOTE_BUILD_DIR/install/bin/"
                break
            else
                echo -e "${RED}✗ Build may have failed${NC}"
                echo ""
                echo "Last build log entries:"
                remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "tail -20 $REMOTE_BUILD_DIR/build.log" || echo "No build log found"
                break
            fi
        fi

        echo ""
        echo "=== SYSTEM RESOURCES ==="
        remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "uptime"
        remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "free -h | head -2"
        remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "df -h $REMOTE_BUILD_DIR | tail -1"

        echo ""
        echo "=== BUILD PROGRESS ==="
        # Get current build progress
        local current_log_lines
        current_log_lines=$(remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "wc -l < $REMOTE_BUILD_DIR/build.log 2>/dev/null || echo 0")

        if [[ "$current_log_lines" -gt "$last_log_lines" ]]; then
            # Show new log entries
            local new_lines=$((current_log_lines - last_log_lines))
            remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "tail -$new_lines $REMOTE_BUILD_DIR/build.log" | tail -10
            last_log_lines=$current_log_lines
        else
            # Show recent log entries
            remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "tail -10 $REMOTE_BUILD_DIR/build.log 2>/dev/null" || echo "Build log not yet available"
        fi

        # Show ninja build progress if available
        if remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "test -f $REMOTE_BUILD_DIR/dsllvm-src/.ninja_log"; then
            local ninja_progress
            ninja_progress=$(remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "wc -l < $REMOTE_BUILD_DIR/dsllvm-src/.ninja_log")
            echo "Ninja build steps completed: $ninja_progress"
        fi

        echo ""
        echo "Press 'q' to quit monitoring, any other key to refresh..."
        read -t "$MONITOR_INTERVAL" -n 1 key
        if [[ "$key" == "q" ]]; then
            break
        fi
    done
}

# Function to cleanup remote build
cleanup_remote_build() {
    if [[ "$SKIP_CLEANUP" == true ]]; then
        log_info "Skipping cleanup as requested (--skip-cleanup)"
        return
    fi

    log_info "Cleaning up remote build directory..."
    remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "rm -rf $REMOTE_BUILD_DIR" || {
        log_warning "Failed to cleanup remote build directory"
    }
}

# Main execution
main() {
    log_info "DSLLVM Remote Build Offloader"
    log_info "Host: $REMOTE_HOST:$REMOTE_PORT"
    log_info "User: $REMOTE_USER"
    log_info "Branch: $BRANCH"
    log_info ""

    # Test SSH connection
    log_info "Testing SSH connection..."
    if ! remote_exec "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "echo 'SSH connection successful'"; then
        log_error "SSH connection failed. Please check your credentials and network."
        exit 1
    fi
    log_success "SSH connection established"

    # Install dependencies if requested
    if [[ "$INSTALL_DEPS" == true ]]; then
        if ! install_dependencies; then
            log_error "Failed to install dependencies"
            exit 1
        fi
    fi

    # Get system information
    get_system_info

    # Start the build
    if ! start_remote_build; then
        log_error "Failed to start remote build"
        exit 1
    fi

    # Monitor the build
    monitor_remote_build

    # Cleanup
    cleanup_remote_build

    log_success "Remote build process completed"
}

# Run main function
main "$@"
