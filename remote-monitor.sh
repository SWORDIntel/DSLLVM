#!/bin/bash
#
# DSLLVM Remote Build Monitor
# Monitors remote DSLLVM build progress
#
# Usage:
#   ./remote-monitor.sh [options]
#
# Options:
#   --host HOST          Remote host (default: 38.102.85.235)
#   --user USER          SSH user (default: dsmil)
#   --port PORT          SSH port (default: 22)
#   --key KEY_FILE       SSH private key file
#   --build-dir DIR      Remote build directory (default: ~/dsllvm-build)
#   --interval N         Monitor interval in seconds (default: 30)
#   --once               Show status once and exit
#

set -euo pipefail

# Default configuration
REMOTE_HOST="38.102.85.235"
REMOTE_USER="dsmil"
REMOTE_PORT="22"
SSH_KEY=""
REMOTE_BUILD_DIR="~/dsllvm-build"
MONITOR_INTERVAL=30
SHOW_ONCE=false

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
        --build-dir)
            REMOTE_BUILD_DIR="$2"
            shift 2
            ;;
        --interval)
            MONITOR_INTERVAL="$2"
            shift 2
            ;;
        --once)
            SHOW_ONCE=true
            shift
            ;;
        --help)
            echo "DSLLVM Remote Build Monitor"
            echo
            echo "Usage: $0 [options]"
            echo
            echo "Options:"
            echo "  --host HOST          Remote host (default: $REMOTE_HOST)"
            echo "  --user USER          SSH user (default: $REMOTE_USER)"
            echo "  --port PORT          SSH port (default: $REMOTE_PORT)"
            echo "  --key KEY_FILE       SSH private key file"
            echo "  --build-dir DIR      Remote build directory (default: $REMOTE_BUILD_DIR)"
            echo "  --interval N         Monitor interval in seconds (default: $MONITOR_INTERVAL)"
            echo "  --once               Show status once and exit"
            echo "  --help               Show this help"
            exit 0
            ;;
        *)
            echo "Unknown option: $1" >&2
            exit 1
            ;;
    esac
done

# Build SSH command
SSH_CMD="ssh"
if [[ -n "$SSH_KEY" ]]; then
    SSH_CMD="$SSH_CMD -i $SSH_KEY"
fi
if [[ "$REMOTE_PORT" != "22" ]]; then
    SSH_CMD="$SSH_CMD -p $REMOTE_PORT"
fi
SSH_CMD="$SSH_CMD ${REMOTE_USER}@${REMOTE_HOST}"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Function to run command on remote host quietly
remote_exec() {
    $SSH_CMD "$1" 2>/dev/null
}

# Function to show build status
show_status() {
    echo
    echo "=== DSLLVM REMOTE BUILD STATUS $(date) ==="
    echo "Host: ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PORT}"
    echo "Build Dir: $REMOTE_BUILD_DIR"
    echo

    # Check if build is running
    if remote_exec "ps aux | grep -v grep | grep -q install-dsllvm.sh"; then
        echo -e "${GREEN}✓ Build is RUNNING${NC}"
    else
        echo -e "${RED}✗ Build is NOT RUNNING${NC}"

        # Check if build completed successfully
        if remote_exec "test -f $REMOTE_BUILD_DIR/install/bin/clang"; then
            echo -e "${GREEN}✓ Build appears to have COMPLETED successfully${NC}"
            echo "Installed binaries:"
            remote_exec "ls -la $REMOTE_BUILD_DIR/install/bin/ | head -10"
            exit 0
        else
            echo -e "${RED}✗ Build appears to have FAILED or not started${NC}"
        fi
    fi

    echo
    echo "--- SYSTEM RESOURCES ---"
    remote_exec "echo 'Load: ' && uptime && echo && echo 'Memory:' && free -h | grep '^Mem:' && echo && echo 'Disk:' && df -h $REMOTE_BUILD_DIR | tail -1"

    echo
    echo "--- TEMPERATURE ---"
    remote_exec "sensors 2>/dev/null | grep -E '(Package|Core|temp1)' | head -3" || echo "Temperature monitoring not available"

    echo
    echo "--- BUILD LOG (last 10 lines) ---"
    remote_exec "tail -10 $REMOTE_BUILD_DIR/dsllvm-src/build.log 2>/dev/null" || echo "Build log not available"

    echo
    echo "--- BUILD PROGRESS ---"
    remote_exec "test -f $REMOTE_BUILD_DIR/dsllvm-src/.ninja_log && echo 'Build steps completed: ' && wc -l < $REMOTE_BUILD_DIR/dsllvm-src/.ninja_log || echo 'Progress tracking not available'"
}

# Main monitoring loop
if [[ "$SHOW_ONCE" == true ]]; then
    show_status
    exit 0
fi

echo "DSLLVM Remote Build Monitor"
echo "Monitoring ${REMOTE_USER}@${REMOTE_HOST}:${REMOTE_PORT}"
echo "Press Ctrl+C to stop monitoring"
echo

while true; do
    show_status
    echo
    echo "Next update in ${MONITOR_INTERVAL}s... (Ctrl+C to stop)"
    sleep "$MONITOR_INTERVAL"
done
