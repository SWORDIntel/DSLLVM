// DsmilVNNIPatternMatcher.h - Pattern matching for AVX-VNNI lowering
//
// Part of the DSLLVM Project
//
// This file implements pattern matching to detect multiply-accumulate loops
// that can be optimized with AVX-VNNI (VPDPBUSD) intrinsics.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_DSMIL_VNNI_PATTERN_MATCHER_H
#define LLVM_TRANSFORMS_DSMIL_VNNI_PATTERN_MATCHER_H

#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include <vector>

namespace llvm {
namespace dsmil {

/// Represents a multiply-accumulate pattern suitable for VNNI
struct MACPattern {
  Loop *OuterLoop;          // Outer loop (i)
  Loop *MiddleLoop;         // Middle loop (j)
  Loop *InnerLoop;          // Inner loop (k)
  
  PHINode *AccumulatorPhi;  // Accumulator: sum = 0; sum += ...
  BinaryOperator *MulOp;    // Multiply: a * b
  BinaryOperator *AddOp;    // Add: sum += (a * b)
  
  Value *LeftOperand;       // Left matrix element: A[i][k]
  Value *RightOperand;      // Right matrix element: B[k][j]
  Value *OutputPtr;         // Output pointer: C[i][j]
  
  Type *ElementType;        // Element type (i8, i16, i32)
  unsigned VectorWidth;     // Number of elements to vectorize
  
  bool isINT8;              // true if i8 x i8 -> i32
  bool isContiguous;        // true if memory accesses are sequential
  
  // Strides for memory access
  int64_t LeftStride;
  int64_t RightStride;
  int64_t OutputStride;
};

/// Detects and analyzes MAC patterns for VNNI optimization
class VNNIPatternMatcher {
public:
  VNNIPatternMatcher(LoopInfo &LI, ScalarEvolution &SE) : LI(LI), SE(SE) {}
  
  /// Find all MAC patterns in a function
  std::vector<MACPattern> findMACPatterns(Function &F);
  
  /// Check if a loop nest is a GEMM pattern
  bool isGEMMPattern(Loop *OuterLoop);
  
  /// Check if a loop nest is a Conv2D pattern
  bool isConv2DPattern(Loop *OuterLoop);
  
  /// Analyze a loop nest to extract MAC pattern
  std::optional<MACPattern> analyzeMACLoop(Loop *OuterLoop);
  
  /// Check if pattern is suitable for VNNI (INT8 x INT8 -> INT32)
  bool isSuitableForVNNI(const MACPattern &Pattern);
  
  /// Estimate speedup from VNNI optimization
  float estimateSpeedup(const MACPattern &Pattern);
  
private:
  LoopInfo &LI;
  ScalarEvolution &SE;
  
  /// Check if a loop contains a multiply-accumulate pattern
  bool containsMACPattern(Loop *L, BinaryOperator *&MulOp, BinaryOperator *&AddOp);
  
  /// Extract operands from MAC pattern
  bool extractMACOperands(BinaryOperator *MulOp, BinaryOperator *AddOp,
                          Value *&LeftOp, Value *&RightOp, PHINode *&Acc);
  
  /// Analyze memory access pattern
  void analyzeMemoryAccess(Value *Ptr, int64_t &Stride);
  
  /// Check if all operations are on i8 type
  bool isINT8MAC(BinaryOperator *MulOp);
  
  /// Check if loop nest is 3-level (for GEMM)
  bool is3LevelNest(Loop *L, Loop *&Outer, Loop *&Middle, Loop *&Inner);
};

} // namespace dsmil
} // namespace llvm

#endif // LLVM_TRANSFORMS_DSMIL_VNNI_PATTERN_MATCHER_H
