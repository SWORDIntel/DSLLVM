add_subdirectory(Passes)
add_subdirectory(Runtime)

# Additional DSMIL components
# Config validator
add_library(dsmil_config STATIC
  Config/dsmil_config_validator.c
)
target_include_directories(dsmil_config PUBLIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/include
)
install(TARGETS dsmil_config
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Metrics
add_library(dsmil_metrics_lib STATIC
  Metrics/dsmil_metrics.c
)
target_include_directories(dsmil_metrics_lib PUBLIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/include
)
install(TARGETS dsmil_metrics_lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Setup
add_library(dsmil_setup_lib STATIC
  Setup/dsmil_setup.c
)
target_include_directories(dsmil_setup_lib PUBLIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/include
)
install(TARGETS dsmil_setup_lib
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Telemetry export
add_library(dsmil_telemetry_export STATIC
  Telemetry/dsmil_telemetry_export.c
)
target_include_directories(dsmil_telemetry_export PUBLIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/include
)
install(TARGETS dsmil_telemetry_export
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Additional runtime telemetry libraries (from dsmil/runtime/)
# These provide fuzzing and OpenTelemetry support
add_library(dsmil_fuzz_telemetry STATIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/runtime/dsmil_fuzz_telemetry.c
)
target_include_directories(dsmil_fuzz_telemetry PUBLIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/include
)
install(TARGETS dsmil_fuzz_telemetry
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_library(dsmil_fuzz_telemetry_advanced STATIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/runtime/dsmil_fuzz_telemetry_advanced.c
)
target_include_directories(dsmil_fuzz_telemetry_advanced PUBLIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/include
)
install(TARGETS dsmil_fuzz_telemetry_advanced
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_library(dsmil_ot_telemetry STATIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/runtime/dsmil_ot_telemetry.c
)
target_include_directories(dsmil_ot_telemetry PUBLIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/include
)
install(TARGETS dsmil_ot_telemetry
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_library(dsssl_fuzz_telemetry STATIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/runtime/dsssl_fuzz_telemetry.c
)
target_include_directories(dsssl_fuzz_telemetry PUBLIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/include
)
install(TARGETS dsssl_fuzz_telemetry
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

add_library(dsssl_fuzz_telemetry_advanced STATIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/runtime/dsssl_fuzz_telemetry_advanced.c
)
target_include_directories(dsssl_fuzz_telemetry_advanced PUBLIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/include
)
install(TARGETS dsssl_fuzz_telemetry_advanced
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
