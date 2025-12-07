# CMakeLists.txt for DSLLVM CPU Feature Integration Passes
#
# Part of the DSLLVM Project

cmake_minimum_required(VERSION 3.13)

project(DSLLVMPasses CXX)

# Find LLVM
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add LLVM definitions and include directories
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

# Library: DSLLVM CPU Features
add_library(DsmilCPUFeatures SHARED
  DsmilCPUFeatures.cpp
)

target_link_libraries(DsmilCPUFeatures
  LLVM
)

# Library: DSLLVM Bandwidth Estimate Pass
add_library(DsmilBandwidthEstimate SHARED
  DsmilBandwidthEstimate.cpp
)

target_link_libraries(DsmilBandwidthEstimate
  DsmilCPUFeatures
  LLVM
)

# Library: DSLLVM AI Accelerate Pass
add_library(DsmilAIAccelerate SHARED
  DsmilAIAccelerate.cpp
)

target_link_libraries(DsmilAIAccelerate
  DsmilCPUFeatures
  LLVM
)

# Library: DSLLVM Speculation Hardening Pass
add_library(DsmilSpecHardening SHARED
  DsmilSpecHardening.cpp
)

target_link_libraries(DsmilSpecHardening
  DsmilCPUFeatures
  LLVM
)

# Library: DSLLVM Constant-Time Check Pass
add_library(DsmilConstantTimeCheck SHARED
  DsmilConstantTimeCheck.cpp
)

target_link_libraries(DsmilConstantTimeCheck
  DsmilCPUFeatures
  LLVM
)

# Plugin: Combined DSLLVM Passes (for opt --load-pass-plugin)
add_library(DSLLVMPasses MODULE
  PassRegistry.cpp
  DsmilCPUFeatures.cpp
  DsmilBandwidthEstimate.cpp
  DsmilAIAccelerate.cpp
  DsmilSpecHardening.cpp
  DsmilConstantTimeCheck.cpp
  DsmilVNNIPatternMatcher.cpp
  DsmilVNNILowering.cpp
)

target_link_libraries(DSLLVMPasses
  LLVM
)

# Don't add 'lib' prefix to plugin
set_target_properties(DSLLVMPasses PROPERTIES PREFIX "")

# Installation
install(TARGETS 
  DsmilCPUFeatures
  DsmilBandwidthEstimate
  DsmilAIAccelerate
  DsmilSpecHardening
  DsmilConstantTimeCheck
  DSLLVMPasses
  LIBRARY DESTINATION lib/dsllvm
)

install(FILES
  DsmilCPUFeatures.h
  DsmilBandwidthEstimate.h
  DsmilAIAccelerate.h
  DsmilSpecHardening.h
  DsmilConstantTimeCheck.h
  DESTINATION include/dsllvm
)
