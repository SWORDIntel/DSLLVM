// DsmilSpecHardening.cpp - Speculation mitigation implementation
//
// Part of the DSLLVM Project
//
//===----------------------------------------------------------------------===//

#include "DsmilSpecHardening.h"
#include "DsmilIntrinsics.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace llvm::dsmil;

PreservedAnalyses SpecHardeningPass::run(Module &M, ModuleAnalysisManager &AM) {
  CPUFeatures Features(M);
  
  errs() << "DSLLVM SpecHardening: Mode=";
  switch (Mode) {
    case Off: errs() << "Off"; break;
    case Hardware: errs() << "Hardware"; break;
    case Hybrid: errs() << "Hybrid"; break;
    case Paranoid: errs() << "Paranoid"; break;
  }
  errs() << "\n";
  
  if (Mode == Off)
    return PreservedAnalyses::all();
  
  // Check hardware mitigation support
  bool has_hw_mitigations = hardwareMitigationsSufficient(Features);
  
  if (Mode == Hardware && !has_hw_mitigations) {
    errs() << "  WARNING: Hardware mitigations insufficient, falling back to fences\n";
  }
  
  bool Modified = false;
  unsigned TotalHazards = 0;
  unsigned TotalMitigated = 0;
  
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    
    // Identify hazard sites
    std::vector<HazardSite> Hazards = identifyHazards(F);
    
    if (Hazards.empty())
      continue;
    
    TotalHazards += Hazards.size();
    
    errs() << "  Function '" << F.getName() << "': " << Hazards.size() << " hazards\n";
    
    // Mitigate each hazard
    unsigned Mitigated = 0;
    for (HazardSite &Site : Hazards) {
      if (mitigateHazard(Site, Features)) {
        Mitigated++;
        Modified = true;
      }
    }
    
    TotalMitigated += Mitigated;
    
    // Attach metadata
    attachMitigationMetadata(F, Hazards.size(), Mitigated);
  }
  
  errs() << "  Summary: " << TotalMitigated << "/" << TotalHazards 
         << " hazards mitigated\n";
  
  return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

std::vector<SpecHardeningPass::HazardSite> 
SpecHardeningPass::identifyHazards(Function &F) {
  std::vector<HazardSite> Hazards;
  
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      // Indirect branch (Spectre v2)
      if (auto *BI = dyn_cast<BranchInst>(&I)) {
        if (!BI->isConditional())
          continue;
        
        // Check if condition depends on potentially speculative data
        // TODO: More sophisticated taint analysis
        Hazards.push_back({&I, HazardSite::IndirectBranch});
      }
      
      // Bounds check (Spectre v1)
      if (auto *CI = dyn_cast<ICmpInst>(&I)) {
        // Look for array bound comparisons
        if (CI->getPredicate() == ICmpInst::ICMP_ULT ||
            CI->getPredicate() == ICmpInst::ICMP_ULE) {
          // Check if result is used in branch
          for (User *U : CI->users()) {
            if (isa<BranchInst>(U)) {
              Hazards.push_back({&I, HazardSite::BoundsCheck});
              break;
            }
          }
        }
      }
      
      // Speculative load (Spectre v4 / SSB)
      if (auto *LI = dyn_cast<LoadInst>(&I)) {
        // Check if load depends on prior stores
        // Simplified: flag all loads in loops
        if (BB.getParent()->getName().contains("loop")) {
          Hazards.push_back({&I, HazardSite::SpeculativeLoad});
        }
      }
      
      // MDS-vulnerable operations
      if (auto *CI = dyn_cast<CallInst>(&I)) {
        Function *Callee = CI->getCalledFunction();
        if (Callee && Callee->getName().contains("secret")) {
          // Operations on secret data may be MDS-vulnerable
          Hazards.push_back({&I, HazardSite::MDSVulnerable});
        }
      }
    }
  }
  
  return Hazards;
}

bool SpecHardeningPass::mitigateHazard(HazardSite &Site, const CPUFeatures &Features) {
  bool hw_mitigations = hardwareMitigationsSufficient(Features);
  
  switch (Site.Type) {
    case HazardSite::IndirectBranch:
      // Spectre v2: IBRS or retpoline
      if (Mode == Paranoid || (!hw_mitigations && Mode != Hardware)) {
        errs() << "    Inserting LFENCE for indirect branch\n";
        insertLFENCE(Site.I);
        return true;
      } else if (Features.hasIBRSEnhanced()) {
        errs() << "    Relying on IBRS for indirect branch\n";
        return false;  // Hardware handles it
      }
      break;
      
    case HazardSite::BoundsCheck:
      // Spectre v1: Always fence after bounds check
      errs() << "    Inserting LFENCE after bounds check\n";
      insertLFENCE(Site.I);
      return true;
      
    case HazardSite::SpeculativeLoad:
      // Spectre v4 (SSB): SSBD or fence
      if (Mode == Paranoid || !Features.hasSSBD()) {
        errs() << "    Inserting LFENCE for speculative load\n";
        insertLFENCE(Site.I);
        return true;
      } else {
        errs() << "    Relying on SSBD for speculative load\n";
        return false;
      }
      break;
      
    case HazardSite::MDSVulnerable:
      // MDS: MD_CLEAR
      if (Features.hasMDClear()) {
        errs() << "    Inserting MD_CLEAR for MDS mitigation\n";
        insertMDClear(Site.I);
        return true;
      } else {
        errs() << "    WARNING: MDS mitigation unavailable\n";
        return false;
      }
      break;
  }
  
  return false;
}

void SpecHardeningPass::insertLFENCE(Instruction *I) {
  IRBuilder<> Builder(I->getNextNode());
  DsmilIntrinsics::insertLFENCE(Builder);
}

void SpecHardeningPass::insertMDClear(Instruction *I) {
  // MD_CLEAR sequence: VERW instruction
  // This clears microarchitectural buffers
  
  IRBuilder<> Builder(I->getNextNode());
  DsmilIntrinsics::insertVERW(Builder);
}

bool SpecHardeningPass::hardwareMitigationsSufficient(const CPUFeatures &Features) {
  // Hardware mitigations are sufficient if:
  // - IBRS enhanced (Spectre v2)
  // - SSBD (Spectre v4 / SSB)
  // - MD_CLEAR (MDS/RIDL)
  
  return Features.hasIBRSEnhanced() &&
         Features.hasSSBD() &&
         Features.hasMDClear();
}

void SpecHardeningPass::attachMitigationMetadata(Function &F, unsigned HazardCount,
                                                   unsigned MitigatedCount) {
  LLVMContext &Ctx = F.getContext();
  
  F.setMetadata("dsmil.spec.hazard_count",
                MDNode::get(Ctx, ConstantAsMetadata::get(
                  ConstantInt::get(Type::getInt32Ty(Ctx), HazardCount))));
  
  F.setMetadata("dsmil.spec.mitigated_count",
                MDNode::get(Ctx, ConstantAsMetadata::get(
                  ConstantInt::get(Type::getInt32Ty(Ctx), MitigatedCount))));
}
