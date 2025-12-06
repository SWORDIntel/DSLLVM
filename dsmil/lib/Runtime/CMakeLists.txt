set(DSMIL_RUNTIME_SOURCES
  # Core runtime libraries
  dsmil_stealth_runtime.c
  dsmil_radio_runtime.c
  dsmil_nuclear_surety_runtime.c
  dsmil_mpe_runtime.c
  dsmil_jadc2_runtime.c
  dsmil_edge_security_runtime.c
  dsmil_cross_domain_runtime.c
  dsmil_blue_red_runtime.c
  dsmil_bft_runtime.c
  # Device-specific runtimes
  dsmil_device15_wycheproof_runtime.c
  dsmil_device255_crypto_runtime.c
  dsmil_device46_pqc_runtime.c
  dsmil_device47_crypto_runtime.c
  # Layer-specific runtimes
  dsmil_layer7_llm_runtime.c
  dsmil_layer8_security_runtime.c
  dsmil_layer8_security_crypto_runtime.c
  dsmil_layer9_executive_runtime.c
  # Advanced runtime features
  dsmil_int8_quantization_runtime.c
  dsmil_quantum_runtime.c
  dsmil_mlops_optimization_runtime.c
  dsmil_mlops_crypto_runtime.c
  dsmil_intelligence_flow_runtime.c
  dsmil_memory_budget_runtime.c
  dsmil_hil_orchestration_runtime.c
  dsmil_paths_runtime.c
  # Additional runtime files from dsmil/runtime/
  # Note: These are separate from lib/Runtime/ but provide telemetry support
  # They are included here to ensure they're built with the main runtime
)

add_library(dsmilrt STATIC ${DSMIL_RUNTIME_SOURCES})
set_target_properties(dsmilrt PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(dsmilrt PUBLIC
  ${LLVM_MAIN_SRC_DIR}/../dsmil/include
)

install(TARGETS dsmilrt
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
