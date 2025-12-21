# DSLLVM Enhanced Build System

## 🚀 Enhanced Features

The DSLLVM build system has been significantly enhanced with the following features:

### ✅ Guaranteed Build Resume
- **State Persistence**: Build state is automatically saved to `$BUILD_DIR/.build_state`
- **Ninja Resume**: Leverages Ninja's dependency tracking for automatic resume
- **Emergency Recovery**: Handles emergency shutdowns due to thermal issues
- **Resume Command**: Simply re-run the installer with `--resume`

### ✅ Advanced Thermal Protection
- **Multi-Sensor Support**: CPU + GPU temperature monitoring
- **Configurable Thresholds**:
  - High threshold (default: 105°C) - throttles build jobs
  - Critical threshold (default: 110°C) - EMERGENCY SHUTDOWN
  - Resume threshold (default: 90°C) - resumes full speed
- **Dynamic Throttling**: Automatically adjusts parallel jobs based on temperature
- **Cooldown Periods**: Maintains reduced load after thermal events

### ✅ System-Wide Installation
- **Package Manager Integration**: Creates deb/rpm packages for system installation
- **Symlink Management**: Automatic creation of system compiler symlinks
- **Environment Setup**: System-wide PATH and compiler configuration
- **Systemd Integration**: Optional monitoring service

## 📋 Usage

### Basic Installation
```bash
sudo ./install-dsllvm.sh
```

### Advanced Installation with Thermal Control
```bash
sudo ./install-dsllvm.sh \
    --thermal-max 105 \
    --thermal-critical 110 \
    --gpu-throttle \
    --package-manager \
    --systemd-service
```

### Resume Interrupted Build
```bash
sudo ./install-dsllvm.sh --resume
```

### Custom Thermal Sensor
```bash
sudo ./install-dsllvm.sh --thermal-sensor /sys/class/thermal/thermal_zone0/temp
```

## 🔧 Command Line Options

| Option | Description | Default |
|--------|-------------|---------|
| `--thermal-max TEMP` | High temperature threshold (°C) | 105 |
| `--thermal-critical TEMP` | Critical temperature threshold (°C) | 110 |
| `--thermal-sensor PATH` | Custom thermal sensor path | auto-detect |
| `--gpu-throttle` | Enable GPU temperature monitoring | disabled |
| `--package-manager` | Install as system package (deb/rpm) | disabled |
| `--systemd-service` | Create monitoring systemd service | disabled |
| `--resume` | Resume from previous build state | auto-detect |
| `--jobs N` | Number of parallel build jobs | auto-detect |

## 🌡️ Thermal Management

### Temperature Thresholds
- **High (105°C)**: Reduces jobs to 60% of base capacity
- **Critical (110°C)**: Emergency shutdown to prevent hardware damage
- **Resume (90°C)**: Returns to full speed after cooldown period

### Thermal Sensors
The system automatically detects temperature sensors in this order:
1. Custom sensor path (if specified)
2. lm-sensors (`sensors` command)
3. sysfs thermal zones (`/sys/class/thermal/`)
4. GPU sensors (NVIDIA/AMD if `--gpu-throttle` enabled)

### Throttling Behavior
```
CPU Temp < 105°C: Full speed (100% jobs)
CPU Temp ≥ 105°C: Throttled (60% jobs) + cooldown timer
CPU Temp ≥ 110°C: EMERGENCY SHUTDOWN
CPU Temp ≤ 90°C: Resume full speed (after cooldown)
```

## 🔄 Build Resume System

### Resume Capabilities
- **Automatic Detection**: Detects existing build state on startup
- **State Persistence**: Saves build configuration and progress
- **Emergency Recovery**: Handles thermal emergency shutdowns
- **Dependency Tracking**: Ninja automatically skips completed tasks

### Resume Files
- `$BUILD_DIR/.ninja_deps`: Ninja dependency database
- `$BUILD_DIR/.build_state`: Build configuration state
- `$BUILD_DIR/.emergency_state`: Emergency shutdown information

### Resume Workflow
1. Run installer with `--resume` or let it auto-detect
2. System loads previous state and configuration
3. Ninja resumes from last completed task
4. Thermal monitoring continues with saved thresholds

## 📦 Package Manager Integration

### Supported Formats
- **DEB**: Debian/Ubuntu systems (`dpkg`)
- **RPM**: Red Hat/CentOS systems (`rpm`)

### Package Features
- System-wide installation to `/usr/local`
- Automatic symlinks and alternatives setup
- Post-install environment configuration
- Dependency management

### Installation Process
1. Builds DSLLVM normally
2. Creates package structure with all files
3. Generates package metadata and control files
4. Installs package using system package manager
5. Configures system integration

## 🔧 Systemd Service

### Service Features
- **Automatic Monitoring**: Runs in background monitoring builds
- **Thermal Protection**: Independent thermal monitoring
- **Emergency Shutdown**: Can terminate builds if temperature critical
- **Logging**: Comprehensive logging to `/var/log/dsllvm-build-monitor.log`

### Service Management
```bash
# Enable and start service
sudo systemctl enable dsllvm-build-monitor
sudo systemctl start dsllvm-build-monitor

# Check status
sudo systemctl status dsllvm-build-monitor

# View logs
sudo journalctl -u dsllvm-build-monitor
```

## 📊 Build Feedback

### Real-time Monitoring
```
🌡️  85°C | ⚙️  8/10 jobs ✅ RUNNING FULL
🌡️  108°C | ⚙️  6/10 jobs 🔥 THROTTLED (temp high)
🌡️  112°C | ⚙️  EMERGENCY SHUTDOWN TRIGGERED
```

### Build Phases
1. **Prerequisites Check**: Validates build environment
2. **Configuration**: CMake setup with DSMIL features
3. **Thermal Calibration**: Tests temperature monitoring
4. **Build Execution**: Ninja build with thermal throttling
5. **Installation**: System integration and packaging
6. **Verification**: Validates installation completeness

## 🛡️ Safety Features

### Emergency Shutdown
- Triggers at 110°C (configurable)
- Saves build state for resume
- Terminates build processes safely
- Logs emergency event details

### Build State Protection
- Atomic state file updates
- Backup of critical build files
- Recovery from interrupted saves
- Validation of resumed state

### Thermal Sensor Validation
- Multiple fallback temperature sources
- Sensor validation and error handling
- Graceful degradation without sensors
- Custom sensor path support

## 🔍 Troubleshooting

### Common Issues

**Thermal monitoring not working:**
```bash
# Install lm-sensors
sudo apt-get install lm-sensors
sudo sensors-detect

# Or use custom sensor
./install-dsllvm.sh --thermal-sensor /sys/class/thermal/thermal_zone0/temp
```

**Build resume fails:**
```bash
# Clean and restart
./install-dsllvm.sh --clean

# Or manual resume
cd build && ninja -j$(nproc)
```

**Package installation fails:**
```bash
# Check available space
df -h /tmp

# Clean temp files
sudo rm -rf /tmp/dsllvm-pkg*
```

### Debug Information
```bash
# Check thermal sensors
./dsllvm-build-monitor.sh status

# View build logs
cat dsllvm-install-*.log

# Check systemd service
sudo systemctl status dsllvm-build-monitor
```

## 📈 Performance Optimization

### Job Detection
- **P-core Detection**: Prioritizes performance cores for builds
- **Heuristic Fallback**: Uses logical CPU count if P-cores undetectable
- **Dynamic Adjustment**: Thermal throttling maintains optimal performance

### Memory Management
- **Build Caching**: Ninja dependency tracking minimizes rebuilds
- **Parallel Execution**: Optimal job count for system capabilities
- **Resource Monitoring**: Prevents memory exhaustion

### I/O Optimization
- **Ninja Build System**: Fast, parallel build execution
- **Dependency Tracking**: Only rebuilds changed components
- **State Persistence**: Fast resume from any interruption

## 🎯 Best Practices

### For Development
1. Use `--resume` for iterative development
2. Enable GPU throttling on multi-GPU systems
3. Set custom thermal limits for your hardware
4. Use `--systemd-service` for unattended builds

### For Production
1. Always use `--package-manager` for system integration
2. Configure appropriate thermal limits for your environment
3. Enable systemd service for 24/7 thermal monitoring
4. Use `--thermal-sensor` for custom cooling setups

### For CI/CD
1. Use `--dry-run` for validation
2. Set conservative thermal limits (`--thermal-max 95`)
3. Use `--jobs` to match CI resource limits
4. Enable `--gpu-throttle` for GPU-accelerated builds

## 📋 Feature Comparison

| Feature | Original | Enhanced |
|---------|----------|----------|
| Build Resume | Basic Ninja resume | State persistence + emergency recovery |
| Thermal Control | Fixed throttling | Dynamic multi-sensor monitoring |
| Installation | Prefix only | Package manager + system integration |
| Monitoring | None | Real-time thermal feedback |
| Error Recovery | Manual | Automatic state recovery |
| GPU Support | None | Full GPU temperature monitoring |
| Systemd | None | Complete service integration |

## 🔄 Migration Guide

### From Original Script
1. **Backup existing builds**: `cp -r build build.backup`
2. **Run enhanced installer**: `./install-dsllvm.sh --resume`
3. **System will detect and resume** existing build automatically

### Updating Existing Installation
1. **Re-run installer** with new options: `./install-dsllvm.sh --package-manager`
2. **System handles** upgrade automatically
3. **No data loss** during upgrade process

## 🤝 Contributing

### Enhancement Requests
- Thermal sensor support for additional hardware
- Package manager support for additional formats
- Advanced build analytics and reporting
- Integration with build farm systems

### Development
1. Test thermal monitoring on target hardware
2. Validate package creation on target distributions
3. Ensure backward compatibility with existing builds
4. Test systemd service integration thoroughly

---

**This enhanced build system provides enterprise-grade reliability and performance for DSLLVM compilation with comprehensive thermal protection and build resilience.**
