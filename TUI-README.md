# DSLLVM Integrated TUI (Text User Interface)

A comprehensive, menu-driven interface for managing remote DSLLVM builds with real-time monitoring, dependency management, and GitHub integration.

## 🚀 Quick Start

### Launch the TUI
```bash
# Interactive mode (recommended)
./dsllvm-tui.sh

# Direct monitoring
./dsllvm-tui.sh --monitor

# Install dependencies only
./dsllvm-tui.sh --install-deps
```

### Prerequisites
```bash
# Install required packages (including sshpass for password auth)
sudo apt-get install dialog sshpass openssh-client curl wget git jq
```

### Local System Optimization
The remote build automatically detects your local system's architecture and applies optimization flags:

- **Architecture Detection**: Automatically detects x86_64, aarch64, armv7, riscv64, ppc64le, s390x
- **OS Detection**: Supports Ubuntu, Debian, CentOS, RHEL, Fedora, Alpine
- **Compiler Flags**: Adds `-march=native` for x86_64, appropriate architecture flags for others
- **Target Selection**: Enables only relevant LLVM targets for your architecture
- **Parallel Jobs**: Uses all CPU cores detected on local system

## 🎯 Features

### Core Functionality
- ✅ **Interactive Menu System** - Easy navigation through all features
- ✅ **Remote Host Management** - Connect and configure remote build servers
- ✅ **Dependency Installation** - Automated system package installation
- ✅ **GitHub Integration** - Repository cloning, branch management, API access
- ✅ **Build Orchestration** - Start and manage remote builds
- ✅ **Real-time Monitoring** - Live build progress and system metrics
- ✅ **Thermal Management** - CPU governor and temperature monitoring

### User Interface
- 📋 **Menu-driven navigation** with clear options
- 📊 **Real-time status displays** with color coding
- 🔄 **Progress indicators** and completion tracking
- ⚠️ **Error handling** with informative messages
- 🎨 **Color-coded output** for better readability

## 📋 Menu Options

### 1. Connect to Remote Host
- Configure SSH connection parameters
- Test connectivity before proceeding
- Support for password and key-based authentication

### 2. Install Dependencies
- Automated installation of build tools
- LLVM/Clang development packages
- System monitoring utilities
- One-click dependency resolution

### 3. Download/Update Repository
- GitHub repository cloning
- Branch selection and switching
- Automatic updates for existing repositories

### 4. Start Build
- Launch DSLLVM compilation on remote host
- Thermal management integration
- Background process execution

### 5. Monitor Build
- Real-time build progress tracking
- System resource monitoring (CPU, memory, disk)
- Temperature monitoring
- Live build log streaming

### 6. View System Info
- Remote host system information
- Hardware specifications
- Available resources

### 7. Check GitHub Access
- Verify GitHub API connectivity
- Test repository access permissions
- PAT authentication validation

### 8. Configuration
- View current settings
- Connection parameters
- Build configuration

## 🔧 Configuration

### Default Settings
```bash
Remote Host: 38.102.85.235
SSH User: dsmil
SSH Port: 22
Repository: https://github.com/SWORDIntel/DSLLVM.git
Branch: main
Build Directory: ~/dsllvm-build
```

### GitHub Integration
- **Personal Access Token**: Pre-configured for SWORDIntel organization
- **API Access**: Repository management, branch operations
- **Authentication**: Token-based for private repository access

## 📊 Monitoring Display

The monitoring interface provides comprehensive real-time information:

```
╔══════════════════════════════════════════════════════════════╗
║                     DSLLVM BUILD TUI                        ║
║              Distributed Systems Machine Intelligence       ║
╚══════════════════════════════════════════════════════════════╝

=== REAL-TIME BUILD MONITORING ===
Host: dsmil@38.102.85.235:22
Time: Mon Dec 23 15:30:45 UTC 2024

✓ Build Status: RUNNING

--- SYSTEM RESOURCES ---
Load: 2.45, 2.12, 1.98
Memory: 16GB total, 8GB used, 8GB free
Disk: 300GB free / 500GB total

--- TEMPERATURE ---
Package id 0: +45.0°C
Core 0: +42.0°C
Core 1: +43.0°C

--- BUILD PROGRESS ---
[2045/6810] Building CXX object lib/CodeGen/CMakeFiles/LLVMCodeGen.dir/BranchFolding.cpp.o
[2046/6810] Building CXX object lib/CodeGen/CMakeFiles/LLVMCodeGen.dir/BasicBlockSections.cpp.o

Build steps completed: 2046
```

## 🚀 Usage Examples

### Complete Build Workflow
```bash
# 1. Launch TUI
./dsllvm-tui.sh

# 2. Connect to remote host (Menu 1)
# Enter connection details, test connectivity

# 3. Install dependencies (Menu 2)
# Automated package installation

# 4. Download repository (Menu 3)
# Select branch and clone/update

# 5. Start build (Menu 4)
# Launch compilation with thermal management

# 6. Monitor progress (Menu 5)
# Real-time monitoring with system metrics
```

### Direct Operations
```bash
# Start monitoring directly
./dsllvm-tui.sh --monitor

# Install dependencies only
./dsllvm-tui.sh --install-deps
```

## 🔐 Security Features

### SSH Authentication
- **Password Authentication**: Uses `sshpass` for SSH password authentication
- **Default Password**: 261505 (configured in `.dsllvm-config`)
- **Interactive Password Entry**: Secure password input via TUI dialogs
- **No SSH keys required**: Password authentication is used instead
- **Custom SSH ports** support
- **Connection timeout** protection

### GitHub Security
- **Personal Access Token** authentication
- **Repository access** validation
- **API rate limiting** awareness

## ⚡ Performance Features

### Build Optimization
- **Parallel compilation** with job control
- **Thermal throttling** to prevent overheating
- **CPU governor management** for optimal performance
- **Memory monitoring** and resource tracking

### Network Efficiency
- **SSH connection reuse** for monitoring
- **Compressed data transfer** for logs
- **Incremental updates** for repository changes

## 🛠️ Troubleshooting

### Common Issues

#### SSH Connection Problems
```bash
# Test connection manually
ssh -p 22 dsmil@38.102.85.235

# With SSH key
ssh -i ~/.ssh/id_rsa -p 22 dsmil@38.102.85.235
```

#### Dialog Not Available
```bash
# Install dialog package
sudo apt-get install dialog
```

#### GitHub Access Issues
```bash
# Check token permissions
curl -H "Authorization: token YOUR_TOKEN" https://api.github.com/user
```

#### Build Failures
- Check system resources with monitoring
- Review build logs on remote host
- Verify dependencies are installed
- Check thermal throttling status

### Log Files
```bash
# Local TUI logs
~/.dsllvm-tui.log

# Remote build logs
~/dsllvm-build/build.log
~/dsllvm-build/dsllvm-src/build.log
```

## 🎨 Interface Screenshots

### Main Menu
```
DSLLVM Build Management TUI
┌─────────────────────────────────────┐
│ Select an option:                   │
│                                     │
│ 1 Connect to Remote Host           │
│ 2 Install Dependencies             │
│ 3 Download/Update Repository       │
│ 4 Start Build                      │
│ 5 Monitor Build                    │
│ 6 View System Info                 │
│ 7 Check GitHub Access              │
│ 8 Configuration                    │
│ 9 Exit                             │
└─────────────────────────────────────┘
```

### Monitoring Interface
```
=== REAL-TIME BUILD MONITORING ===
✓ Build Status: RUNNING

--- SYSTEM RESOURCES ---
Load: 2.45, 2.12, 1.98
Memory: 8GB used / 16GB total
Disk: 300GB free / 500GB total

--- BUILD PROGRESS ---
[2045/6810] Building CXX object...
Build steps completed: 2045
```

## 🔧 Advanced Configuration

### Custom SSH Configuration
```bash
# Modify script variables at the top
REMOTE_HOST="your-server.com"
REMOTE_USER="builder"
REMOTE_PORT="2222"
SSH_KEY="/path/to/key"
```

### Environment Variables
```bash
# Override defaults
export DSLLVM_REMOTE_HOST="192.168.1.100"
export DSLLVM_REMOTE_USER="admin"
export DSLLVM_SSH_KEY="/home/user/.ssh/build_key"
```

## 🤝 Integration

### CI/CD Integration
```bash
#!/bin/bash
# ci-deploy.sh
./dsllvm-tui.sh --install-deps
./dsllvm-tui.sh --monitor &
BUILD_PID=$!
# ... other CI steps ...
kill $BUILD_PID
```

### Automated Workflows
```bash
#!/bin/bash
# nightly-build.sh
./dsllvm-tui.sh << EOF
1
38.102.85.235
dsmil
22
/path/to/key
EOF
# ... continue with automated workflow
```

## 📈 Performance Metrics

The TUI provides comprehensive performance tracking:

- **Build Progress**: Step-by-step completion tracking
- **Resource Usage**: CPU, memory, disk monitoring
- **Thermal Management**: Temperature and governor control
- **Network Efficiency**: Connection status and transfer rates
- **Time Tracking**: Elapsed time and ETA calculations

## 🔄 Update Process

### Keeping TUI Updated
```bash
# Update from repository
git pull origin main

# Restart TUI
./dsllvm-tui.sh
```

### Feature Requests
- Use GitHub issues for feature requests
- Check existing issues before submitting
- Provide detailed use case descriptions

---

**The DSLLVM TUI provides a complete, user-friendly interface for distributed build management with enterprise-grade monitoring and control capabilities.** 🚀

**Launch with: `./dsllvm-tui.sh`**
