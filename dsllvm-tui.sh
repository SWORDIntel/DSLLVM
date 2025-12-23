#!/bin/bash
#
# DSLLVM Integrated TUI (Text User Interface)
# Complete remote build management system with monitoring
#
# Features:
# - Interactive menu system
# - Remote host management
# - Dependency installation
# - GitHub repository operations
# - Build monitoring and control
# - System resource monitoring
#

set -euo pipefail

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_FILE="$SCRIPT_DIR/.dsllvm-config"

# Load configuration
load_config() {
    if [[ -f "$CONFIG_FILE" ]]; then
        source "$CONFIG_FILE"
    else
        echo "Warning: Configuration file $CONFIG_FILE not found."
        echo "Please create it with your settings."
        # Set defaults
        REMOTE_HOST="${REMOTE_HOST:-38.102.85.235}"
        REMOTE_USER="${REMOTE_USER:-dsmil}"
        REMOTE_PORT="${REMOTE_PORT:-22}"
        SSH_PASS="${SSH_PASS:-}"
        GITHUB_PAT="${GITHUB_PAT:-}"
        REPO_URL="${REPO_URL:-https://github.com/SWORDIntel/DSLLVM.git}"
        BRANCH="${BRANCH:-main}"
        REMOTE_BUILD_DIR="${REMOTE_BUILD_DIR:-~/dsllvm-build}"
    fi
}

# Initialize configuration
load_config

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
NC='\033[0m' # No Color

# UI Functions
show_banner() {
    clear
    echo -e "${CYAN}"
    cat << 'EOF'
╔══════════════════════════════════════════════════════════════╗
║                     DSLLVM BUILD TUI                        ║
║              Distributed Systems Machine Intelligence       ║
║                                                              ║
║  🚀 Remote Build Management    🔧 Dependency Management      ║
║  📊 Real-time Monitoring       🔄 GitHub Integration         ║
║  🔥 Thermal Control           ⚡ Performance Optimization     ║
║                                                              ║
║  Remote Host: Auto-detected    Branch: main                  ║
║  Status: Ready                  Auth: SSH Password           ║
╚══════════════════════════════════════════════════════════════╝
EOF
    echo -e "${NC}"
}

check_dependencies() {
    local missing_deps=()

    # Check for required commands
    for cmd in dialog ssh curl wget git; do
        if ! command -v "$cmd" &> /dev/null; then
            missing_deps+=("$cmd")
        fi
    done

    # Check for Python (optional but recommended)
    if ! command -v python3 &> /dev/null && ! command -v python &> /dev/null; then
        echo -e "${YELLOW}Warning: Python not found. Some features may be limited.${NC}"
    fi

    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        echo -e "${RED}Missing required dependencies: ${missing_deps[*]}${NC}"
        echo -e "${YELLOW}Install with: sudo apt-get install dialog openssh-client curl wget git${NC}"
        return 1
    fi

    return 0
}

# SSH Functions
build_ssh_cmd() {
    local host="$1"
    local user="$2"
    local port="$3"

    local ssh_cmd="sshpass -p '$SSH_PASS' ssh"
    ssh_cmd="$ssh_cmd -p $port -o ConnectTimeout=10 -o StrictHostKeyChecking=no ${user}@${host}"

    echo "$ssh_cmd"
}

test_ssh_connection() {
    local host="$1"
    local user="$2"
    local port="$3"

    if [[ -z "$SSH_PASS" ]]; then
        echo -e "${RED}SSH password not configured. Please set SSH_PASS in $CONFIG_FILE${NC}"
        return 1
    fi

    local ssh_cmd
    ssh_cmd=$(build_ssh_cmd "$host" "$user" "$port")

    if eval "$ssh_cmd 'echo \"SSH connection successful\"'" &>/dev/null; then
        return 0
    else
        return 1
    fi
}

get_remote_info() {
    local host="$1"
    local user="$2"
    local port="$3"

    local ssh_cmd
    ssh_cmd=$(build_ssh_cmd "$host" "$user" "$port")

    echo "=== REMOTE SYSTEM INFO ==="
    eval "$ssh_cmd 'uname -a'" 2>/dev/null || echo "Failed to get system info"
    eval "$ssh_cmd 'lsb_release -d 2>/dev/null | cut -f2'" 2>/dev/null || echo "Ubuntu/Debian system"
    eval "$ssh_cmd 'nproc && echo \"CPU cores\"'" 2>/dev/null || echo "CPU info unavailable"
    eval "$ssh_cmd 'free -h | grep \"^Mem:\"'" 2>/dev/null || echo "Memory info unavailable"
    eval "$ssh_cmd 'df -h / | tail -1'" 2>/dev/null || echo "Disk info unavailable"
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

# GitHub Functions
github_api_call() {
    local endpoint="$1"
    local method="${2:-GET}"
    local data="${3:-}"

    local url="https://api.github.com${endpoint}"
    local auth_header="Authorization: token ${GITHUB_PAT}"

    if [[ "$method" == "GET" ]]; then
        curl -s -H "$auth_header" "$url"
    elif [[ "$method" == "POST" ]]; then
        curl -s -X POST -H "$auth_header" -H "Content-Type: application/json" -d "$data" "$url"
    fi
}

check_repo_access() {
    local repo_info
    repo_info=$(github_api_call "/repos/SWORDIntel/DSLLVM")

    if [[ $(echo "$repo_info" | jq -r '.name') == "DSLLVM" ]]; then
        echo -e "${GREEN}✓ GitHub repository access confirmed${NC}"
        return 0
    else
        echo -e "${RED}✗ GitHub repository access failed${NC}"
        return 1
    fi
}

get_repo_branches() {
    github_api_call "/repos/SWORDIntel/DSLLVM/branches" | jq -r '.[].name' 2>/dev/null || echo "main"
}

# Build Functions
start_remote_build() {
    local host="$1"
    local user="$2"
    local port="$3"
    local key="$4"
    local branch="$5"

    local ssh_cmd="ssh"
    if [[ -n "$key" ]]; then
        ssh_cmd="$ssh_cmd -i $key"
    fi
    ssh_cmd="$ssh_cmd -p $port ${user}@${host}"

    echo "Starting remote build on ${user}@${host}..."

    # Create build directory
    $ssh_cmd "mkdir -p $REMOTE_BUILD_DIR" || {
        echo -e "${RED}Failed to create build directory${NC}"
        return 1
    }

    # Clone/update repository
    if $ssh_cmd "test -d $REMOTE_BUILD_DIR/dsllvm-src"; then
        echo "Updating existing repository..."
        $ssh_cmd "cd $REMOTE_BUILD_DIR/dsllvm-src && git fetch origin && git checkout $branch && git pull origin $branch" || {
            echo -e "${RED}Failed to update repository${NC}"
            return 1
        }
    else
        echo "Cloning repository..."
        $ssh_cmd "cd $REMOTE_BUILD_DIR && git clone --branch $branch $REPO_URL dsllvm-src" || {
            echo -e "${RED}Failed to clone repository${NC}"
            return 1
        }
    fi

    # Start build in background
    $ssh_cmd "cd $REMOTE_BUILD_DIR/dsllvm-src && nohup ./install-dsllvm.sh --prefix $REMOTE_BUILD_DIR/install --thermal-max 85 --thermal-critical 95 --clean --resume --skip-install > $REMOTE_BUILD_DIR/build.log 2>&1 & echo \$! > $REMOTE_BUILD_DIR/build.pid" || {
        echo -e "${RED}Failed to start build${NC}"
        return 1
    }

    echo -e "${GREEN}✓ Build started successfully${NC}"
    return 0
}

# Monitoring Functions
monitor_build() {
    local host="$1"
    local user="$2"
    local port="$3"

    while true; do
        clear
        show_banner

        echo -e "${WHITE}=== REAL-TIME BUILD MONITORING ===${NC}"
        echo "Host: ${user}@${host}:${port}"
        echo "Time: $(date)"
        echo

        # Check if build is running
        if remote_exec "$host" "$user" "$port" "ps aux | grep -v grep | grep -q install-dsllvm.sh" >/dev/null; then
            echo -e "${GREEN}✓ Build Status: RUNNING${NC}"
        else
            echo -e "${YELLOW}⚠ Build Status: NOT RUNNING${NC}"
            if remote_exec "$host" "$user" "$port" "test -f $REMOTE_BUILD_DIR/install/bin/clang" >/dev/null; then
                echo -e "${GREEN}✓ Build appears COMPLETED successfully${NC}"
                echo
                echo "Installed binaries:"
                remote_exec "$host" "$user" "$port" "ls -la $REMOTE_BUILD_DIR/install/bin/" 2>/dev/null | head -5
                break
            else
                echo -e "${RED}✗ Build appears FAILED or not started${NC}"
                echo
                echo "Last build log entries:"
                remote_exec "$host" "$user" "$port" "tail -10 $REMOTE_BUILD_DIR/build.log" 2>/dev/null || echo "No build log available"
                break
            fi
        fi

        echo
        echo -e "${WHITE}--- SYSTEM RESOURCES ---${NC}"
        remote_exec "$host" "$user" "$port" "uptime" 2>/dev/null | sed 's/^/Load: /' || echo "Load info unavailable"
        remote_exec "$host" "$user" "$port" "free -h | grep '^Mem:' | awk '{print \"Memory: \" \$2 \" total, \" \$3 \" used, \" \$4 \" free\"}'" 2>/dev/null || echo "Memory info unavailable"
        remote_exec "$host" "$user" "$port" "df -h $REMOTE_BUILD_DIR | tail -1 | awk '{print \"Disk: \" \$4 \" free / \" \$2 \" total\"}'" 2>/dev/null || echo "Disk info unavailable"

        echo
        echo -e "${WHITE}--- TEMPERATURE ---${NC}"
        remote_exec "$host" "$user" "$port" "sensors 2>/dev/null | grep -E '(Package|Core|temp1)' | head -3" 2>/dev/null || echo "Temperature monitoring not available"

        echo
        echo -e "${WHITE}--- BUILD PROGRESS ---${NC}"
        remote_exec "$host" "$user" "$port" "tail -5 $REMOTE_BUILD_DIR/dsllvm-src/build.log 2>/dev/null | grep -E '\\[.*\\].*\\.o\\|\\[.*\\].*Building\\|FAILED\\|error'" 2>/dev/null | tail -3 || echo "Build log not available yet"

        local progress
        progress=$(remote_exec "$host" "$user" "$port" "test -f $REMOTE_BUILD_DIR/dsllvm-src/.ninja_log && wc -l < $REMOTE_BUILD_DIR/dsllvm-src/.ninja_log || echo '0'" 2>/dev/null)
        if [[ "$progress" != "0" ]]; then
            echo "Build steps completed: $progress"
        fi

        echo
        echo -e "${YELLOW}Press 'q' to quit monitoring, any other key to refresh...${NC}"
        read -t 10 -n 1 key
        if [[ "$key" == "q" ]]; then
            break
        fi
    done
}

# Dependency Installation
install_remote_deps() {
    local host="$1"
    local user="$2"
    local port="$3"

    echo "Installing dependencies on remote host..."

    # Update package list
    if ! remote_exec "$host" "$user" "$port" "sudo apt-get update -qq"; then
        echo -e "${YELLOW}Warning: Failed to update package list${NC}"
    fi

    # Install build tools
    echo "Installing build tools..."
    if ! remote_exec "$host" "$user" "$port" "sudo apt-get install -y -qq build-essential cmake ninja-build git curl wget"; then
        echo -e "${RED}Failed to install build tools${NC}"
        return 1
    fi

    # Install LLVM dependencies
    echo "Installing LLVM dependencies..."
    if ! remote_exec "$host" "$user" "$port" "sudo apt-get install -y -qq libssl-dev libffi-dev python3-dev libedit-dev libncurses-dev swig libxml2-dev liblzma-dev"; then
        echo -e "${RED}Failed to install LLVM dependencies${NC}"
        return 1
    fi

    # Install monitoring tools
    echo "Installing monitoring tools..."
    if ! remote_exec "$host" "$user" "$port" "sudo apt-get install -y -qq htop lm-sensors sysstat iotop jq"; then
        echo -e "${YELLOW}Warning: Failed to install monitoring tools${NC}"
    fi

    echo -e "${GREEN}✓ Dependencies installed successfully${NC}"
}

# Main Menu
show_main_menu() {
    local choice
    while true; do
        show_banner

        choice=$(dialog --clear --title "DSLLVM Build Management TUI" \
            --menu "Select an option:" 20 60 12 \
            1 "Connect to Remote Host" \
            2 "Install Dependencies" \
            3 "Download/Update Repository" \
            4 "Start Build" \
            5 "Monitor Build" \
            6 "View System Info" \
            7 "Check GitHub Access" \
            8 "Configuration" \
            9 "Exit" \
            2>&1 >/dev/tty)

        case $choice in
            1)
                connect_to_host
                ;;
            2)
                install_dependencies_menu
                ;;
            3)
                download_repository
                ;;
            4)
                start_build_menu
                ;;
            5)
                monitor_build_menu
                ;;
            6)
                view_system_info
                ;;
            7)
                check_github_access
                ;;
            8)
                show_configuration
                ;;
            9)
                echo -e "${GREEN}Goodbye!${NC}"
                exit 0
                ;;
            *)
                echo -e "${YELLOW}Invalid option. Please try again.${NC}"
                sleep 2
                ;;
        esac
    done
}

# Menu Functions
connect_to_host() {
    local host user port pass

    host=$(dialog --inputbox "Remote Host:" 8 40 "$REMOTE_HOST" 2>&1 >/dev/tty)
    [[ -z "$host" ]] && return

    user=$(dialog --inputbox "SSH User:" 8 40 "$REMOTE_USER" 2>&1 >/dev/tty)
    [[ -z "$user" ]] && return

    port=$(dialog --inputbox "SSH Port:" 8 40 "$REMOTE_PORT" 2>&1 >/dev/tty)
    [[ -z "$port" ]] && return

    pass=$(dialog --passwordbox "SSH Password:" 8 40 2>&1 >/dev/tty)
    [[ -z "$pass" ]] && return

    REMOTE_HOST="$host"
    REMOTE_USER="$user"
    REMOTE_PORT="$port"
    SSH_PASS="$pass"

    if test_ssh_connection "$host" "$user" "$port"; then
        dialog --msgbox "SSH connection successful!" 6 30
    else
        dialog --msgbox "SSH connection failed!" 6 30
    fi
}

install_dependencies_menu() {
    if dialog --yesno "Install dependencies on remote host?\n\nThis will install:\n- Build tools (cmake, ninja, git)\n- LLVM dependencies\n- Monitoring tools" 12 50; then
        if install_remote_deps "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT"; then
            dialog --msgbox "Dependencies installed successfully!" 6 40
        else
            dialog --msgbox "Failed to install dependencies!" 6 40
        fi
    fi
}

download_repository() {
    local branches
    branches=$(get_repo_branches)

    local branch
    branch=$(dialog --menu "Select branch:" 15 40 8 $(echo "$branches" | sed 's/^/ /g' | tr '\n' ' ') 2>&1 >/dev/tty)

    if [[ -n "$branch" ]]; then
        BRANCH="$branch"
        dialog --infobox "Downloading/updating repository (branch: $branch)..." 5 50
        sleep 2

        if start_remote_build "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "$SSH_KEY" "$branch" 2>/dev/null; then
            dialog --msgbox "Repository updated successfully!" 6 40
        else
            dialog --msgbox "Failed to update repository!" 6 40
        fi
    fi
}

start_build_menu() {
    if dialog --yesno "Start DSLLVM build on remote host?\n\nThis will:\n- Configure build system\n- Start compilation\n- Enable thermal management" 10 50; then
        dialog --infobox "Starting build..." 5 30
        sleep 2

        # Just trigger the build start, monitoring will happen separately
        if start_remote_build "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "$SSH_KEY" "$BRANCH" 2>/dev/null; then
            dialog --msgbox "Build started successfully!\n\nUse 'Monitor Build' to track progress." 8 50
        else
            dialog --msgbox "Failed to start build!" 6 40
        fi
    fi
}

monitor_build_menu() {
    if dialog --yesno "Open real-time build monitoring?\n\nThis will show:\n- Build progress\n- System resources\n- Temperature\n- Live logs" 10 50; then
        # Switch to console monitoring
        reset
        monitor_build "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT"
        echo -e "${YELLOW}Press Enter to return to menu...${NC}"
        read
    fi
}

view_system_info() {
    dialog --infobox "Gathering system information..." 5 40
    local info
    info=$(get_remote_info "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT" "$SSH_KEY")

    dialog --msgbox "$info" 20 70
}

check_github_access() {
    dialog --infobox "Checking GitHub access..." 5 30
    sleep 2

    if check_repo_access; then
        dialog --msgbox "GitHub access confirmed!\n\nRepository: SWORDIntel/DSLLVM\nAuthentication: PAT" 8 50
    else
        dialog --msgbox "GitHub access failed!\n\nCheck PAT and repository permissions." 8 50
    fi
}

show_configuration() {
    local config
    config="Current Configuration:\n\n"
    config+="Remote Host: $REMOTE_HOST\n"
    config+="SSH User: $REMOTE_USER\n"
    config+="SSH Port: $REMOTE_PORT\n"
    config+="SSH Password: ${SSH_PASS:+Configured (hidden)}\n"
    config+="Repository: $REPO_URL\n"
    config+="Branch: $BRANCH\n"
    config+="Build Dir: $REMOTE_BUILD_DIR\n"
    config+="GitHub PAT: ${GITHUB_PAT:+Configured}"

    dialog --msgbox "$config" 15 60
}

# Main execution
main() {
    # Check for required dependencies
    if ! check_dependencies; then
        echo -e "${RED}Missing required dependencies. Please install them first.${NC}"
        exit 1
    fi

    # Start the TUI
    show_main_menu
}

# Handle command line arguments
if [[ $# -gt 0 ]]; then
    case "$1" in
        --help|-h)
            echo "DSLLVM Integrated TUI"
            echo
            echo "Usage: $0 [options]"
            echo
            echo "Options:"
            echo "  --help, -h          Show this help"
            echo "  --monitor            Start monitoring directly"
            echo "  --install-deps       Install dependencies and exit"
            echo
            echo "Interactive mode (default):"
            echo "  Provides full menu-driven interface"
            exit 0
            ;;
        --monitor)
            monitor_build "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT"
            ;;
        --install-deps)
            install_remote_deps "$REMOTE_HOST" "$REMOTE_USER" "$REMOTE_PORT"
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
else
    main
fi
