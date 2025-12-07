// DsmilAIAccelerate.h - AI workload acceleration using CPU features
//
// Part of the DSLLVM Project
//
// This pass optimizes AI kernels (GEMM, convolution, attention) using
// hardware features like AVX-VNNI, FSRM, BMI1.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_DSMIL_AI_ACCELERATE_H
#define LLVM_TRANSFORMS_DSMIL_AI_ACCELERATE_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"
#include "DsmilCPUFeatures.h"

namespace llvm {
namespace dsmil {

class AIAcceleratePass : public PassInfoMixin<AIAcceleratePass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
  
private:
  /// Detect AI kernel patterns in function
  enum AIKernelType {
    GEMM,              // Matrix multiplication
    Conv2D,            // 2D convolution
    Attention,         // Attention mechanism
    Pooling,           // Pooling operation
    BatchNorm,         // Batch normalization
    Unknown
  };
  
  AIKernelType detectAIKernel(Function &F);
  
  /// Optimize GEMM kernel using AVX-VNNI
  bool optimizeGEMM_VNNI(Function &F, const CPUFeatures &Features);
  
  /// Optimize convolution using AVX-VNNI
  bool optimizeConv_VNNI(Function &F, const CPUFeatures &Features);
  
  /// Optimize attention mechanism
  bool optimizeAttention(Function &F, const CPUFeatures &Features);
  
  /// Insert VNNI intrinsics for INT8 operations
  bool lowerToVNNI(Function &F, const CPUFeatures &Features);
  
  /// Optimize bit manipulation for sparse operations
  bool optimizeBitOps(Function &F, const CPUFeatures &Features);
  
  /// Check if function operates on INT8 data
  bool isINT8Kernel(Function &F);
  
  /// Attach AI optimization metadata
  void attachAIMetadata(Function &F, AIKernelType Type, bool Optimized);
};

} // namespace dsmil
} // namespace llvm

#endif // LLVM_TRANSFORMS_DSMIL_AI_ACCELERATE_H
