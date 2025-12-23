# DSLLVM Remote Build System

This directory contains scripts for offloading DSLLVM compilation to remote hosts with real-time monitoring.

## 🚀 Quick Start

### Basic Remote Build
```bash
# Build on default remote host (38.102.85.235) with dependency installation
./remote-build.sh --install-deps
```

### Custom Configuration
```bash
# Build on custom host with SSH key authentication
./remote-build.sh --host your-server.com --user youruser --key ~/.ssh/id_rsa --install-deps
```

### Monitor Existing Build
```bash
# Monitor remote build status
./remote-monitor.sh

# Quick status check (no continuous monitoring)
./remote-monitor.sh --once
```

## 📋 Scripts Overview

### `remote-build.sh` - Main Build Offloader
**Purpose**: Complete remote build orchestration with dependency management

**Key Features**:
- SSH connection management with key support
- Automatic dependency installation
- GitHub repository cloning
- Remote build execution with thermal management
- Real-time monitoring and progress reporting
- Automatic cleanup (configurable)

**Usage Examples**:
```bash
# Full setup and build with all defaults
./remote-build.sh

# Custom host and branch
./remote-build.sh --host 192.168.1.100 --branch development

# Use SSH key and keep build directory
./remote-build.sh --key ~/.ssh/build_key --skip-cleanup

# Fast monitoring interval
./remote-build.sh --monitor-interval 10
```

### `remote-monitor.sh` - Build Monitor
**Purpose**: Real-time monitoring of remote DSLLVM builds

**Key Features**:
- Live system resource monitoring (CPU, memory, disk)
- Temperature tracking
- Build log streaming
- Progress indicators
- Build completion detection

**Usage Examples**:
```bash
# Continuous monitoring with defaults
./remote-monitor.sh

# Monitor custom host
./remote-monitor.sh --host your-server.com --user admin

# Quick status check
./remote-monitor.sh --once --host 192.168.1.100

# Custom monitoring interval
./remote-monitor.sh --interval 60
```

## ⚙️ Configuration Options

### Remote Host Configuration
```bash
--host HOST          # Remote hostname/IP (default: 38.102.85.235)
--user USER          # SSH username (default: dsmil)
--port PORT          # SSH port (default: 22)
--key KEY_FILE       # SSH private key file (optional)
```

### Build Configuration
```bash
--repo-url URL       # GitHub repository URL
--branch BRANCH      # Git branch to checkout (default: main)
--build-dir DIR      # Remote build directory (default: ~/dsllvm-build)
```

### Build Options
```bash
--install-deps       # Install system dependencies on remote host
--skip-cleanup       # Keep remote build directory after completion
--monitor-interval N # Status update interval in seconds (default: 30)
```

## 🔧 System Dependencies Installed

When `--install-deps` is used, the following packages are installed:

### Build Tools
- `build-essential` - GCC and basic build tools
- `cmake` - Cross-platform build system
- `ninja-build` - Fast build system
- `git` - Version control

### LLVM/Clang Dependencies
- `libssl-dev` - SSL/TLS development libraries
- `libffi-dev` - Foreign Function Interface
- `python3-dev` - Python development headers
- `libedit-dev` - BSD editline library
- `libncurses-dev` - NCurses development
- `swig` - Simplified Wrapper and Interface Generator
- `libxml2-dev` - XML parsing library
- `liblzma-dev` - XZ compression library

### Monitoring Tools
- `htop` - Interactive process viewer
- `lm-sensors` - Hardware monitoring
- `sysstat` - System performance tools
- `iotop` - I/O monitoring

## 📊 Monitoring Output

The monitoring system provides real-time information:

```
=== REMOTE BUILD STATUS Thu Dec 26 10:30:15 UTC 2024 ===

--- SYSTEM RESOURCES ---
Load:  2.45, 2.12, 1.98
Memory: 16GB total, 8GB used, 8GB free
Disk: /home/dsmil 500GB total, 200GB used, 300GB free

--- TEMPERATURE ---
Package id 0: +45.0°C
Core 0: +42.0°C
Core 1: +43.0°C

--- BUILD LOG (last 10 lines) ---
[2045/6810] Building CXX object lib/CodeGen/CMakeFiles/LLVMCodeGen.dir/BranchFolding.cpp.o
[2046/6810] Building CXX object lib/CodeGen/CMakeFiles/LLVMCodeGen.dir/BasicBlockSections.cpp.o
[2047/6810] Building CXX object lib/CodeGen/CMakeFiles/LLVMCodeGen.dir/BasicBlockPathCloning.cpp.o

--- BUILD PROGRESS ---
Build steps completed: 2047
```

## 🔐 SSH Authentication

### Password Authentication (Default)
- Uses standard SSH password authentication
- Requires interactive password entry

### Key-Based Authentication (Recommended)
```bash
# Generate SSH key pair (if needed)
ssh-keygen -t ed25519 -C "dsllvm-build"

# Copy public key to remote host
ssh-copy-id -p 22 dsmil@38.102.85.235

# Use in build script
./remote-build.sh --key ~/.ssh/id_ed25519
```

## 🚨 Troubleshooting

### SSH Connection Issues
```bash
# Test SSH connection manually
ssh -p 22 dsmil@38.102.85.235 "echo 'Connection successful'"

# With SSH key
ssh -i ~/.ssh/id_rsa -p 22 dsmil@38.102.85.235 "echo 'Connection successful'"
```

### Build Failures
- Check remote system resources with `./remote-monitor.sh --once`
- Review build logs: `ssh dsmil@38.102.85.235 tail -50 ~/dsllvm-build/build.log`
- Verify dependencies: `ssh dsmil@38.102.85.235 dpkg -l | grep -E "(cmake|ninja|llvm)"`

### Permission Issues
```bash
# On remote host, ensure user has sudo access
ssh dsmil@38.102.85.235 "sudo -l"

# Check disk space
ssh dsmil@38.102.85.235 "df -h /home/dsmil"
```

## 📈 Performance Optimization

### Remote Host Selection
- Choose hosts with high CPU core count
- Ensure adequate RAM (16GB+ recommended)
- Fast storage (SSD preferred)
- Good network connectivity

### Build Configuration
- Use `--thermal-max 85` for aggressive thermal management
- Adjust `--monitor-interval` based on network latency
- Consider `--cpu-governor` options for CPU frequency control

### Network Optimization
- Use SSH connection multiplexing for faster reconnections
- Choose geographically close hosts to reduce latency
- Use VPN if security requires it

## 🔄 Workflow Examples

### Development Workflow
```bash
# 1. Start build with monitoring
./remote-build.sh --install-deps --monitor-interval 15

# 2. In another terminal, monitor progress
./remote-monitor.sh --interval 20

# 3. Check completion
./remote-monitor.sh --once
```

### CI/CD Integration
```bash
#!/bin/bash
# ci-build.sh
./remote-build.sh --host build-server.company.com \
                  --user builder \
                  --key /path/to/ci-key \
                  --branch "$GIT_BRANCH" \
                  --install-deps \
                  --monitor-interval 60
```

## 🛡️ Security Considerations

- Use SSH key authentication instead of passwords
- Limit SSH access to specific users/IPs
- Regularly rotate SSH keys
- Monitor remote host access logs
- Use VPN for additional security layer

## 📞 Support

For issues with remote building:

1. Check SSH connectivity: `ssh dsmil@38.102.85.235`
2. Verify dependencies: `./remote-monitor.sh --once`
3. Review build logs on remote host
4. Check system resources and temperatures

## 🤝 Contributing

To add new remote build features:

1. Modify `remote-build.sh` for build logic
2. Update `remote-monitor.sh` for monitoring features
3. Test on multiple remote host configurations
4. Update this documentation

---

**The remote build system enables distributed compilation for improved performance, resource utilization, and build reliability across multiple machines.**
