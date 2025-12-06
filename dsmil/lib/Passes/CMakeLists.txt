set(DSMIL_PASS_SOURCES
  DsmilBFTPass.cpp
  DsmilBlueRedPass.cpp
  DsmilConstantTimePass.cpp
  DsmilCrossDomainPass.cpp
  DsmilEdgeSecurityPass.cpp
  DsmilFuzzCoveragePass.cpp
  DsmilFuzzExportPass.cpp
  DsmilJADC2Pass.cpp
  DsmilMPEPass.cpp
  DsmilMetricsPass.cpp
  DsmilMissionPolicyPass.cpp
  DsmilNuclearSuretyPass.cpp
  DsmilRadioBridgePass.cpp
  DsmilStealthPass.cpp
  DsmilTelecomPass.cpp
  DsmilTelemetryCheckPass.cpp
  DsmilTelemetryPass.cpp
  DsmilThreatSignaturePass.cpp
  DssslApiMisusePass.cpp
  DssslCoveragePass.cpp
  DssslCryptoMetricsPass.cpp
)

add_llvm_library(DsmilPasses MODULE
  ${DSMIL_PASS_SOURCES}
  PLUGIN_TOOL opt
)

target_include_directories(DsmilPasses PRIVATE
  ${LLVM_MAIN_SRC_DIR}/../dsmil/include
)

set_target_properties(DsmilPasses PROPERTIES
  OUTPUT_NAME "DsmilPasses"
)

install(TARGETS DsmilPasses
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
