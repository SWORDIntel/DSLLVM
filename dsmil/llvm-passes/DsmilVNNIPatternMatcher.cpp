// DsmilVNNIPatternMatcher.cpp - VNNI pattern matching implementation
//
// Part of the DSLLVM Project
//
//===----------------------------------------------------------------------===//

#include "DsmilVNNIPatternMatcher.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;
using namespace llvm::dsmil;

std::vector<MACPattern> VNNIPatternMatcher::findMACPatterns(Function &F) {
  std::vector<MACPattern> Patterns;
  
  // Iterate through all loops
  for (Loop *L : LI) {
    // Check if this is a 3-level loop nest
    Loop *Outer, *Middle, *Inner;
    if (is3LevelNest(L, Outer, Middle, Inner)) {
      // Analyze for MAC pattern
      auto Pattern = analyzeMACLoop(Outer);
      if (Pattern && isSuitableForVNNI(*Pattern)) {
        Patterns.push_back(*Pattern);
      }
    }
  }
  
  return Patterns;
}

bool VNNIPatternMatcher::is3LevelNest(Loop *L, Loop *&Outer, Loop *&Middle, Loop *&Inner) {
  Outer = L;
  
  // Check for middle loop
  auto &SubLoops = L->getSubLoops();
  if (SubLoops.size() != 1)
    return false;
  
  Middle = SubLoops[0];
  
  // Check for inner loop
  auto &InnerLoops = Middle->getSubLoops();
  if (InnerLoops.size() != 1)
    return false;
  
  Inner = InnerLoops[0];
  
  // Verify inner loop has no further nesting
  if (!Inner->getSubLoops().empty())
    return false;
  
  return true;
}

std::optional<MACPattern> VNNIPatternMatcher::analyzeMACLoop(Loop *OuterLoop) {
  MACPattern Pattern;
  
  Loop *Outer, *Middle, *Inner;
  if (!is3LevelNest(OuterLoop, Outer, Middle, Inner))
    return std::nullopt;
  
  Pattern.OuterLoop = Outer;
  Pattern.MiddleLoop = Middle;
  Pattern.InnerLoop = Inner;
  
  // Look for MAC pattern in inner loop
  BinaryOperator *MulOp = nullptr, *AddOp = nullptr;
  if (!containsMACPattern(Inner, MulOp, AddOp))
    return std::nullopt;
  
  Pattern.MulOp = MulOp;
  Pattern.AddOp = AddOp;
  
  // Extract operands
  if (!extractMACOperands(MulOp, AddOp, Pattern.LeftOperand, 
                          Pattern.RightOperand, Pattern.AccumulatorPhi))
    return std::nullopt;
  
  // Check element type
  Pattern.isINT8 = isINT8MAC(MulOp);
  Pattern.ElementType = MulOp->getOperand(0)->getType();
  
  // Analyze memory access patterns
  analyzeMemoryAccess(Pattern.LeftOperand, Pattern.LeftStride);
  analyzeMemoryAccess(Pattern.RightOperand, Pattern.RightStride);
  
  Pattern.isContiguous = (Pattern.LeftStride == 1 || Pattern.RightStride == 1);
  
  // Estimate vector width (AVX-VNNI processes 32 bytes at a time for i8)
  if (Pattern.isINT8) {
    Pattern.VectorWidth = 32; // 32 x i8 per vector
  } else {
    Pattern.VectorWidth = 8;  // 8 x i32 per vector
  }
  
  return Pattern;
}

bool VNNIPatternMatcher::containsMACPattern(Loop *L, BinaryOperator *&MulOp, 
                                             BinaryOperator *&AddOp) {
  // Look for pattern: acc = acc + (a * b)
  for (BasicBlock *BB : L->getBlocks()) {
    for (Instruction &I : *BB) {
      if (auto *Add = dyn_cast<BinaryOperator>(&I)) {
        if (Add->getOpcode() == Instruction::Add) {
          // Check if one operand is a multiply
          for (unsigned i = 0; i < 2; i++) {
            if (auto *Mul = dyn_cast<BinaryOperator>(Add->getOperand(i))) {
              if (Mul->getOpcode() == Instruction::Mul) {
                AddOp = Add;
                MulOp = Mul;
                return true;
              }
            }
          }
        }
      }
    }
  }
  
  return false;
}

bool VNNIPatternMatcher::extractMACOperands(BinaryOperator *MulOp, BinaryOperator *AddOp,
                                             Value *&LeftOp, Value *&RightOp, PHINode *&Acc) {
  // Extract multiply operands
  LeftOp = MulOp->getOperand(0);
  RightOp = MulOp->getOperand(1);
  
  // Find accumulator PHI
  // Pattern: phi = [0, preheader], [add_result, latch]
  for (unsigned i = 0; i < 2; i++) {
    Value *AddOperand = AddOp->getOperand(i);
    if (auto *Phi = dyn_cast<PHINode>(AddOperand)) {
      // Check if phi is updated by this add
      for (unsigned j = 0; j < Phi->getNumIncomingValues(); j++) {
        if (Phi->getIncomingValue(j) == AddOp) {
          Acc = Phi;
          return true;
        }
      }
    }
  }
  
  return false;
}

void VNNIPatternMatcher::analyzeMemoryAccess(Value *Ptr, int64_t &Stride) {
  // Simplified stride analysis
  // Real implementation would use ScalarEvolution
  
  if (auto *GEP = dyn_cast<GetElementPtrInst>(Ptr)) {
    // Check if GEP has constant stride
    if (GEP->getNumIndices() == 1) {
      if (auto *CI = dyn_cast<ConstantInt>(GEP->idx_begin()->get())) {
        Stride = CI->getSExtValue();
        return;
      }
    }
  }
  
  Stride = 1; // Assume unit stride
}

bool VNNIPatternMatcher::isINT8MAC(BinaryOperator *MulOp) {
  Type *Ty = MulOp->getOperand(0)->getType();
  return Ty->isIntegerTy(8);
}

bool VNNIPatternMatcher::isSuitableForVNNI(const MACPattern &Pattern) {
  // VNNI requirements:
  // 1. INT8 x INT8 -> INT32 multiply-accumulate
  // 2. Loop trip count large enough to amortize overhead
  // 3. Contiguous memory access (at least one operand)
  
  if (!Pattern.isINT8)
    return false;
  
  if (!Pattern.isContiguous)
    return false;
  
  // Check trip count (simplified - real implementation uses ScalarEvolution)
  // Require at least 32 iterations for vectorization
  return true;
}

float VNNIPatternMatcher::estimateSpeedup(const MACPattern &Pattern) {
  // Estimate speedup from VNNI optimization
  
  if (!Pattern.isINT8)
    return 1.0f;
  
  // VNNI can process 32 INT8 operations per instruction
  // vs 1 per scalar instruction
  float VectorSpeedup = Pattern.VectorWidth;
  
  // Account for memory bandwidth
  if (Pattern.isContiguous)
    VectorSpeedup *= 1.2f; // Better cache utilization
  
  // Account for overhead
  float Overhead = 0.9f; // ~10% overhead for setup
  
  return VectorSpeedup * Overhead;
}

bool VNNIPatternMatcher::isGEMMPattern(Loop *OuterLoop) {
  // GEMM pattern: C[i][j] += A[i][k] * B[k][j]
  // 3-level loop nest with specific access pattern
  
  auto Pattern = analyzeMACLoop(OuterLoop);
  if (!Pattern)
    return false;
  
  // Check access patterns match GEMM
  // Simplified: just check it's a 3-level nest with MAC
  return Pattern->isINT8;
}

bool VNNIPatternMatcher::isConv2DPattern(Loop *OuterLoop) {
  // Conv2D has 4+ level nesting typically
  // For now, just detect as GEMM-like (can be optimized similarly)
  return isGEMMPattern(OuterLoop);
}
