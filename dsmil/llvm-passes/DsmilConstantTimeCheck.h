// DsmilConstantTimeCheck.h - Constant-time enforcement for crypto
//
// Part of the DSLLVM Project
//
// This pass enforces constant-time execution for functions marked with
// dsmil_secret attribute, using CPU features (CET, CLFLUSHOPT) for cleanup.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_DSMIL_CONSTANT_TIME_CHECK_H
#define LLVM_TRANSFORMS_DSMIL_CONSTANT_TIME_CHECK_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"
#include "DsmilCPUFeatures.h"

namespace llvm {
namespace dsmil {

class ConstantTimeCheckPass : public PassInfoMixin<ConstantTimeCheckPass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
  
private:
  /// Check if function is marked dsmil_secret
  bool isCryptoFunction(Function &F);
  
  /// Identify violations
  struct CTViolation {
    Instruction *I;
    enum Type {
      SecretDependentBranch,    // if/switch on secret data
      SecretDependentMemAccess, // array[secret_index]
      VariableTimeInstruction,  // div/mod with secret operands
      CacheTimingLeak           // Missing cache flush after secret use
    } Type;
    std::string Message;
  };
  
  std::vector<CTViolation> checkFunction(Function &F, const CPUFeatures &Features);
  
  /// Check for secret-dependent branches
  std::vector<CTViolation> checkBranches(Function &F);
  
  /// Check for secret-dependent memory accesses
  std::vector<CTViolation> checkMemoryAccesses(Function &F);
  
  /// Check for variable-time instructions
  std::vector<CTViolation> checkVariableTimeOps(Function &F);
  
  /// Insert cache flushes after secret operations
  bool insertCacheFlushes(Function &F, const CPUFeatures &Features);
  
  /// Track secret data through SSA graph
  bool isSecretTainted(Value *V);
  
  /// Attach metadata about constant-time verification
  void attachCTMetadata(Function &F, bool Verified, unsigned ViolationCount);
};

} // namespace dsmil
} // namespace llvm

#endif // LLVM_TRANSFORMS_DSMIL_CONSTANT_TIME_CHECK_H
