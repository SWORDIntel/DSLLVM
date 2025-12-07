// DsmilCPUFeatures.h - CPU feature model integration for DSLLVM
//
// Part of the DSLLVM Project
//
// This file defines utilities for querying CPU features from module metadata
// and using them to drive optimization decisions.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_DSMIL_CPU_FEATURES_H
#define LLVM_TRANSFORMS_DSMIL_CPU_FEATURES_H

#include "llvm/IR/Module.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/ADT/SmallVector.h"
#include <string>
#include <set>

namespace llvm {
namespace dsmil {

/// CPU feature query interface
/// Reads !dsllvm.cpu.features metadata from module and provides feature queries
class CPUFeatures {
public:
  explicit CPUFeatures(const Module &M);
  
  /// Check if a specific feature is available
  bool hasFeature(StringRef Feature) const;
  
  /// Check if all features in a list are available
  bool hasAllFeatures(ArrayRef<StringRef> Features) const;
  
  /// Check if any feature in a list is available
  bool hasAnyFeature(ArrayRef<StringRef> Features) const;
  
  /// Get all features in a category
  /// Categories: ai_acceleration, security, profiling, virtualization,
  ///             crypto, memory, cache, timing, tlb, power
  std::set<std::string> getFeaturesInCategory(StringRef Category) const;
  
  /// Get the CPU profile name (e.g., "mtr-mtl-dsmil")
  StringRef getProfileName() const { return ProfileName; }
  
  /// AI/Vector features
  bool hasAVXVNNI() const { return hasFeature("avx_vnni"); }
  bool hasFSRM() const { return hasFeature("fsrm"); }
  bool hasERMS() const { return hasFeature("erms"); }
  bool hasBMI1() const { return hasFeature("bmi1"); }
  bool hasBMI2() const { return hasFeature("bmi2"); }
  bool hasABM() const { return hasFeature("abm"); }
  bool hasRepGood() const { return hasFeature("rep_good"); }
  bool hasAVX2() const { return hasFeature("avx2"); }
  bool hasFMA() const { return hasFeature("fma"); }
  
  /// Security features
  bool hasSMEP() const { return hasFeature("smep"); }
  bool hasSMAP() const { return hasFeature("smap"); }
  bool hasUMIP() const { return hasFeature("umip"); }
  bool hasUserShadowStack() const { return hasFeature("user_shstk"); }
  bool hasTME() const { return hasFeature("tme"); }
  bool hasSMX() const { return hasFeature("smx"); }
  bool hasMDClear() const { return hasFeature("md_clear"); }
  bool hasFlushL1D() const { return hasFeature("flush_l1d"); }
  bool hasSSBD() const { return hasFeature("ssbd"); }
  bool hasIBRSEnhanced() const { return hasFeature("ibrs_enhanced"); }
  bool hasSTIBP() const { return hasFeature("stibp"); }
  bool hasIBPB() const { return hasFeature("ibpb"); }
  bool hasNX() const { return hasFeature("nx"); }
  bool hasPKU() const { return hasFeature("pku"); }
  bool hasArchCapabilities() const { return hasFeature("arch_capabilities"); }
  
  /// Profiling features
  bool hasIntelPT() const { return hasFeature("intel_pt"); }
  bool hasArchLBR() const { return hasFeature("arch_lbr"); }
  bool hasPEBS() const { return hasFeature("pebs"); }
  bool hasHFI() const { return hasFeature("hfi"); }
  bool hasConstantTSC() const { return hasFeature("constant_tsc"); }
  bool hasNonstopTSC() const { return hasFeature("nonstop_tsc"); }
  
  /// Virtualization features
  bool hasVMX() const { return hasFeature("vmx"); }
  bool hasEPT() const { return hasFeature("ept"); }
  bool hasEPTAD() const { return hasFeature("ept_ad"); }
  bool hasVPID() const { return hasFeature("vpid"); }
  bool has1GBPages() const { return hasFeature("pdpe1gb"); }
  
  /// Crypto features
  bool hasAESNI() const { return hasFeature("aes"); }
  bool hasVAES() const { return hasFeature("vaes"); }
  bool hasSHANI() const { return hasFeature("sha_ni"); }
  bool hasPCLMULQDQ() const { return hasFeature("pclmulqdq"); }
  bool hasRDRAND() const { return hasFeature("rdrand"); }
  bool hasRDSEED() const { return hasFeature("rdseed"); }
  bool hasGFNI() const { return hasFeature("gfni"); }
  
  /// Cache features
  bool hasCLFLUSHOPT() const { return hasFeature("clflushopt"); }
  bool hasCLWB() const { return hasFeature("clwb"); }
  
  /// TLB features
  bool hasPCID() const { return hasFeature("pcid"); }
  bool hasINVPCID() const { return hasFeature("invpcid"); }
  
  /// Check if hardware RNG is available (rdseed or rdrand)
  bool hasHardwareRNG() const { return hasRDSEED() || hasRDRAND(); }
  
  /// Check if speculation mitigations are hardware-supported
  bool hasHardwareSpeculationMitigations() const {
    return hasIBRSEnhanced() && hasSSBD() && hasMDClear();
  }
  
  /// Check if constant-time execution support is available
  bool hasConstantTimeSupport() const {
    return hasUserShadowStack() && (hasCLFLUSHOPT() || hasCLWB());
  }
  
  /// Check if profiling prerequisites are met
  bool canEnableProfiling() const {
    return hasConstantTSC() && (hasIntelPT() || hasArchLBR() || hasPEBS());
  }
  
private:
  std::set<std::string> Features;
  std::string ProfileName;
  
  void parseMetadata(const Module &M);
};

} // namespace dsmil
} // namespace llvm

#endif // LLVM_TRANSFORMS_DSMIL_CPU_FEATURES_H
