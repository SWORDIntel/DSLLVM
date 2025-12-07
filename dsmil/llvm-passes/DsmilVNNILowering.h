// DsmilVNNILowering.h - Lower MAC patterns to AVX-VNNI intrinsics
//
// Part of the DSLLVM Project
//
// This file implements the transformation that lowers multiply-accumulate
// patterns to AVX-VNNI (VPDPBUSD) intrinsics for INT8 AI kernels.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_DSMIL_VNNI_LOWERING_H
#define LLVM_TRANSFORMS_DSMIL_VNNI_LOWERING_H

#include "DsmilVNNIPatternMatcher.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

namespace llvm {
namespace dsmil {

/// Transforms MAC patterns into AVX-VNNI intrinsics
class VNNILowering {
public:
  VNNILowering(Module &M) : M(M), Ctx(M.getContext()) {}
  
  /// Lower a MAC pattern to VNNI intrinsics
  bool lowerPattern(MACPattern &Pattern, LoopInfo &LI, DominatorTree &DT);
  
  /// Vectorize the inner loop using VNNI
  bool vectorizeWithVNNI(MACPattern &Pattern, LoopInfo &LI, DominatorTree &DT);
  
  /// Create vectorized version of the loop body
  void createVectorizedBody(MACPattern &Pattern, IRBuilder<> &Builder,
                            Value *VectorizedAcc, unsigned UnrollFactor);
  
  /// Insert VPDPBUSD intrinsic for INT8 MAC
  Value *insertVPDPBUSD(IRBuilder<> &Builder, Value *Acc, Value *Left, Value *Right);
  
  /// Generate vector load for INT8 array
  Value *generateVectorLoad(IRBuilder<> &Builder, Value *Ptr, unsigned Width);
  
  /// Generate vector store for INT32 accumulator
  void generateVectorStore(IRBuilder<> &Builder, Value *Vec, Value *Ptr);
  
  /// Estimate benefit of vectorization
  struct VectorizationBenefit {
    float ExpectedSpeedup;
    unsigned VectorWidth;
    unsigned UnrollFactor;
    bool WorthVectorizing;
  };
  
  VectorizationBenefit analyzeBenefit(const MACPattern &Pattern);
  
private:
  Module &M;
  LLVMContext &Ctx;
  
  /// Get or create VPDPBUSD intrinsic declaration
  Function *getVPDPBUSDIntrinsic();
  
  /// Create vectorized loop structure
  void createVectorLoop(MACPattern &Pattern, BasicBlock *Preheader,
                        BasicBlock *Header, BasicBlock *Latch, BasicBlock *Exit,
                        unsigned VectorWidth);
  
  /// Handle remainder iterations (for loop counts not divisible by vector width)
  void createScalarRemainder(MACPattern &Pattern, BasicBlock *AfterVector);
};

} // namespace dsmil
} // namespace llvm

#endif // LLVM_TRANSFORMS_DSMIL_VNNI_LOWERING_H
