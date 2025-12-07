// DsmilBandwidthEstimate.h - Bandwidth estimation using CPU features
//
// Part of the DSLLVM Project
//
// This pass estimates memory bandwidth usage for functions, taking into
// account CPU-specific memory features (FSRM, ERMS, AVX-VNNI).
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_DSMIL_BANDWIDTH_ESTIMATE_H
#define LLVM_TRANSFORMS_DSMIL_BANDWIDTH_ESTIMATE_H

#include "llvm/IR/PassManager.h"
#include "llvm/IR/Function.h"
#include "DsmilCPUFeatures.h"

namespace llvm {
namespace dsmil {

class BandwidthEstimatePass : public PassInfoMixin<BandwidthEstimatePass> {
public:
  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM);
  
private:
  struct FunctionBandwidth {
    uint64_t bytes_read = 0;
    uint64_t bytes_written = 0;
    double gbps_estimate = 0.0;
    std::string memory_class;
    std::string access_pattern;
  };
  
  FunctionBandwidth analyzeFunctionBandwidth(Function &F, const CPUFeatures &Features);
  
  /// Estimate memcpy/memset cost based on size and CPU features
  double estimateMemopyCost(uint64_t size, const CPUFeatures &Features);
  
  /// Classify memory access pattern
  std::string classifyAccessPattern(Function &F);
  
  /// Determine memory class (kv_cache, model_weights, hot_ram, etc.)
  std::string determineMemoryClass(Function &F, uint64_t total_bytes);
  
  /// Attach bandwidth metadata to function
  void attachBandwidthMetadata(Function &F, const FunctionBandwidth &BW);
};

} // namespace dsmil
} // namespace llvm

#endif // LLVM_TRANSFORMS_DSMIL_BANDWIDTH_ESTIMATE_H
