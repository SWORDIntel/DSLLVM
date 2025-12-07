// DsmilSpecHardening.h - Speculation mitigation using CPU features
//
// Part of the DSLLVM Project
//
// This pass implements speculation mitigations, preferring hardware features
// (IBRS, SSBD, MD_CLEAR) over unconditional fences when available.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_DSMIL_SPEC_HARDENING_H
#define LLVM_TRANSFORMS_DSMIL_SPEC_HARDENING_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"
#include "DsmilCPUFeatures.h"

namespace llvm {
namespace dsmil {

class SpecHardeningPass : public PassInfoMixin<SpecHardeningPass> {
public:
  enum HardeningMode {
    Off,          // No hardening
    Hardware,     // Rely on hardware mitigations (IBRS, SSBD)
    Hybrid,       // Hardware + selective fences
    Paranoid      // Always fence (ignore hardware)
  };
  
  explicit SpecHardeningPass(HardeningMode Mode = Hardware) : Mode(Mode) {}
  
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
  
private:
  HardeningMode Mode;
  
  /// Identify speculation hazard sites
  struct HazardSite {
    Instruction *I;
    enum Type {
      IndirectBranch,     // Spectre v2
      BoundsCheck,        // Spectre v1
      SpeculativeLoad,    // Spectre v4 (SSB)
      MDSVulnerable       // MDS/RIDL
    } Type;
  };
  
  std::vector<HazardSite> identifyHazards(Function &F);
  
  /// Mitigate hazard using appropriate strategy
  bool mitigateHazard(HazardSite &Site, const CPUFeatures &Features);
  
  /// Insert LFENCE at hazard site
  void insertLFENCE(Instruction *I);
  
  /// Insert MD_CLEAR sequence
  void insertMDClear(Instruction *I);
  
  /// Check if hardware mitigations are sufficient
  bool hardwareMitigationsSufficient(const CPUFeatures &Features);
  
  /// Attach metadata about mitigation strategy
  void attachMitigationMetadata(Function &F, unsigned HazardCount, 
                                 unsigned MitigatedCount);
};

} // namespace dsmil
} // namespace llvm

#endif // LLVM_TRANSFORMS_DSMIL_SPEC_HARDENING_H
