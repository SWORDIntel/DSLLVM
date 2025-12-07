// DsmilCPUFeatures.cpp - CPU feature model implementation
//
// Part of the DSLLVM Project
//
//===----------------------------------------------------------------------===//

#include "DsmilCPUFeatures.h"
#include "llvm/IR/Metadata.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace llvm::dsmil;

CPUFeatures::CPUFeatures(const Module &M) {
  parseMetadata(M);
}

void CPUFeatures::parseMetadata(const Module &M) {
  // Parse !dsllvm.cpu.profile metadata
  if (auto *ProfileMD = M.getNamedMetadata("dsllvm.cpu.profile")) {
    if (ProfileMD->getNumOperands() > 0) {
      if (auto *MDStr = dyn_cast<MDString>(ProfileMD->getOperand(0)->getOperand(0))) {
        ProfileName = MDStr->getString().str();
      }
    }
  }
  
  // Parse !dsllvm.cpu.features metadata
  if (auto *FeaturesMD = M.getNamedMetadata("dsllvm.cpu.features")) {
    for (unsigned i = 0; i < FeaturesMD->getNumOperands(); ++i) {
      if (auto *Op = FeaturesMD->getOperand(i)) {
        if (auto *MDStr = dyn_cast<MDString>(Op->getOperand(0))) {
          Features.insert(MDStr->getString().str());
        }
      }
    }
  }
  
  // Debug: print loaded features
  if (!Features.empty()) {
    errs() << "DSLLVM: Loaded CPU profile '" << ProfileName << "' with "
           << Features.size() << " features\n";
  }
}

bool CPUFeatures::hasFeature(StringRef Feature) const {
  return Features.count(Feature.str()) > 0;
}

bool CPUFeatures::hasAllFeatures(ArrayRef<StringRef> FeatureList) const {
  for (auto Feature : FeatureList) {
    if (!hasFeature(Feature))
      return false;
  }
  return true;
}

bool CPUFeatures::hasAnyFeature(ArrayRef<StringRef> FeatureList) const {
  for (auto Feature : FeatureList) {
    if (hasFeature(Feature))
      return true;
  }
  return false;
}

std::set<std::string> CPUFeatures::getFeaturesInCategory(StringRef Category) const {
  std::set<std::string> CategoryFeatures;
  
  // Define category mappings (matches dsllvm-cpufeatures categorization)
  if (Category == "ai_acceleration") {
    std::vector<std::string> ai_features = {
      "avx_vnni", "fsrm", "erms", "bmi1", "bmi2", "abm", "rep_good",
      "avx", "avx2", "fma", "sse4_1", "sse4_2", "popcnt"
    };
    for (const auto &f : ai_features) {
      if (hasFeature(f))
        CategoryFeatures.insert(f);
    }
  } else if (Category == "security") {
    std::vector<std::string> security_features = {
      "smep", "smap", "umip", "user_shstk", "tme", "smx",
      "md_clear", "flush_l1d", "ssbd", "ibrs_enhanced", "stibp", "ibpb",
      "nx", "pku", "arch_capabilities"
    };
    for (const auto &f : security_features) {
      if (hasFeature(f))
        CategoryFeatures.insert(f);
    }
  } else if (Category == "profiling") {
    std::vector<std::string> profiling_features = {
      "intel_pt", "arch_lbr", "pebs", "bts", "arch_perfmon", "hfi",
      "constant_tsc", "nonstop_tsc"
    };
    for (const auto &f : profiling_features) {
      if (hasFeature(f))
        CategoryFeatures.insert(f);
    }
  } else if (Category == "crypto") {
    std::vector<std::string> crypto_features = {
      "aes", "vaes", "sha_ni", "pclmulqdq", "rdrand", "rdseed", "gfni"
    };
    for (const auto &f : crypto_features) {
      if (hasFeature(f))
        CategoryFeatures.insert(f);
    }
  }
  
  return CategoryFeatures;
}
