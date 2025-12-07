// DsmilBandwidthEstimate.cpp - Bandwidth estimation implementation
//
// Part of the DSLLVM Project
//
//===----------------------------------------------------------------------===//

#include "DsmilBandwidthEstimate.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace llvm::dsmil;

PreservedAnalyses BandwidthEstimatePass::run(Module &M, ModuleAnalysisManager &AM) {
  // Load CPU features from module metadata
  CPUFeatures Features(M);
  
  errs() << "DSLLVM BandwidthEstimate: Analyzing module with CPU profile '"
         << Features.getProfileName() << "'\n";
  
  bool Modified = false;
  
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    
    // Analyze function bandwidth
    FunctionBandwidth BW = analyzeFunctionBandwidth(F, Features);
    
    // Attach metadata
    attachBandwidthMetadata(F, BW);
    
    // Report
    if (BW.bytes_read > 0 || BW.bytes_written > 0) {
      errs() << "  Function '" << F.getName() << "': "
             << "read=" << BW.bytes_read << "B, "
             << "write=" << BW.bytes_written << "B, "
             << "est=" << BW.gbps_estimate << " GB/s, "
             << "class=" << BW.memory_class << ", "
             << "pattern=" << BW.access_pattern << "\n";
    }
    
    Modified = true;
  }
  
  return Modified ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

BandwidthEstimatePass::FunctionBandwidth 
BandwidthEstimatePass::analyzeFunctionBandwidth(Function &F, const CPUFeatures &Features) {
  FunctionBandwidth BW;
  
  uint64_t load_count = 0;
  uint64_t store_count = 0;
  uint64_t vector_loads = 0;
  uint64_t vector_stores = 0;
  
  // Count loads and stores
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (auto *LI = dyn_cast<LoadInst>(&I)) {
        Type *Ty = LI->getType();
        uint64_t size = Ty->getPrimitiveSizeInBits() / 8;
        BW.bytes_read += size;
        load_count++;
        
        if (Ty->isVectorTy())
          vector_loads++;
      } else if (auto *SI = dyn_cast<StoreInst>(&I)) {
        Type *Ty = SI->getValueOperand()->getType();
        uint64_t size = Ty->getPrimitiveSizeInBits() / 8;
        BW.bytes_written += size;
        store_count++;
        
        if (Ty->isVectorTy())
          vector_stores++;
      } else if (auto *MI = dyn_cast<MemIntrinsic>(&I)) {
        // Handle memcpy, memmove, memset
        if (auto *Len = dyn_cast<ConstantInt>(MI->getLength())) {
          uint64_t size = Len->getZExtValue();
          
          if (isa<MemTransferInst>(MI)) {
            BW.bytes_read += size;
            BW.bytes_written += size;
          } else if (isa<MemSetInst>(MI)) {
            BW.bytes_written += size;
          }
          
          // Account for FSRM/ERMS optimization
          double cost = estimateMemopyCost(size, Features);
          BW.gbps_estimate += size / cost;  // Simplified
        }
      }
    }
  }
  
  // Classify access pattern
  BW.access_pattern = classifyAccessPattern(F);
  
  // Determine memory class
  uint64_t total_bytes = BW.bytes_read + BW.bytes_written;
  BW.memory_class = determineMemoryClass(F, total_bytes);
  
  // Estimate bandwidth (simplified model)
  // Real implementation would use cycle-accurate modeling
  if (Features.hasAVXVNNI() && vector_loads > 0) {
    // AVX-VNNI can achieve higher throughput
    BW.gbps_estimate = (total_bytes * 2.0) / 1e9;  // Rough estimate
  } else {
    BW.gbps_estimate = total_bytes / 1e9;
  }
  
  return BW;
}

double BandwidthEstimatePass::estimateMemopyCost(uint64_t size, const CPUFeatures &Features) {
  // Cost in nanoseconds (simplified model)
  
  if (size < 256 && Features.hasFSRM()) {
    // FSRM: Fast Short REP MOVSB (optimized for < 256 bytes)
    return size * 0.5;  // ~2 bytes/ns
  } else if (Features.hasERMS()) {
    // ERMS: Enhanced REP MOVSB/STOSB (optimized for large copies)
    return size * 0.25;  // ~4 bytes/ns
  } else if (Features.hasRepGood()) {
    // REP is fast, but not enhanced
    return size * 1.0;  // ~1 byte/ns
  } else {
    // Generic memcpy (slower)
    return size * 2.0;  // ~0.5 bytes/ns
  }
}

std::string BandwidthEstimatePass::classifyAccessPattern(Function &F) {
  // Simplified pattern classification
  // Real implementation would analyze strides and access sequences
  
  bool has_gather = false;
  bool has_scatter = false;
  bool is_sequential = true;
  
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      // Check for gather/scatter patterns
      if (auto *CI = dyn_cast<CallInst>(&I)) {
        StringRef Name = CI->getCalledFunction() ? 
                         CI->getCalledFunction()->getName() : "";
        if (Name.contains("gather")) has_gather = true;
        if (Name.contains("scatter")) has_scatter = true;
      }
      
      // Check for indexed loads (non-sequential)
      if (auto *GEP = dyn_cast<GetElementPtrInst>(&I)) {
        // Simplified: if GEP has non-constant index, assume non-sequential
        for (Use &U : GEP->indices()) {
          if (!isa<ConstantInt>(U)) {
            is_sequential = false;
            break;
          }
        }
      }
    }
  }
  
  if (has_gather || has_scatter)
    return "gather-scatter";
  else if (!is_sequential)
    return "strided";
  else
    return "contiguous";
}

std::string BandwidthEstimatePass::determineMemoryClass(Function &F, uint64_t total_bytes) {
  // Simplified classification based on function name and size
  StringRef Name = F.getName();
  
  if (Name.contains("kv_cache") || Name.contains("attention"))
    return "kv_cache";
  else if (Name.contains("weights") || Name.contains("model"))
    return "model_weights";
  else if (total_bytes > 1024 * 1024)  // > 1MB
    return "large_buffer";
  else if (total_bytes > 64 * 1024)     // > 64KB
    return "hot_ram";
  else
    return "cache_resident";
}

void BandwidthEstimatePass::attachBandwidthMetadata(Function &F, const FunctionBandwidth &BW) {
  LLVMContext &Ctx = F.getContext();
  
  // Attach !dsmil.bw_bytes_read
  F.setMetadata("dsmil.bw_bytes_read",
                MDNode::get(Ctx, ConstantAsMetadata::get(
                  ConstantInt::get(Type::getInt64Ty(Ctx), BW.bytes_read))));
  
  // Attach !dsmil.bw_bytes_written
  F.setMetadata("dsmil.bw_bytes_written",
                MDNode::get(Ctx, ConstantAsMetadata::get(
                  ConstantInt::get(Type::getInt64Ty(Ctx), BW.bytes_written))));
  
  // Attach !dsmil.bw_gbps_estimate
  F.setMetadata("dsmil.bw_gbps_estimate",
                MDNode::get(Ctx, ConstantAsMetadata::get(
                  ConstantFP::get(Type::getDoubleTy(Ctx), BW.gbps_estimate))));
  
  // Attach !dsmil.memory_class
  F.setMetadata("dsmil.memory_class",
                MDNode::get(Ctx, MDString::get(Ctx, BW.memory_class)));
  
  // Attach !dsmil.access_pattern
  F.setMetadata("dsmil.access_pattern",
                MDNode::get(Ctx, MDString::get(Ctx, BW.access_pattern)));
}
