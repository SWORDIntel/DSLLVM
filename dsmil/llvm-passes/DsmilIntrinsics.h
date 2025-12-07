// DsmilIntrinsics.h - DSLLVM intrinsic helpers
//
// Part of the DSLLVM Project
//
// This file provides helpers for inserting x86 intrinsics used by
// DSLLVM passes (LFENCE, CLFLUSHOPT, VNNI, etc.)
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_DSMIL_INTRINSICS_H
#define LLVM_TRANSFORMS_DSMIL_INTRINSICS_H

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicsX86.h"

namespace llvm {
namespace dsmil {

class DsmilIntrinsics {
public:
  /// Insert LFENCE (load fence) for speculation mitigation
  static void insertLFENCE(IRBuilder<> &Builder) {
    Module *M = Builder.GetInsertBlock()->getModule();
    Function *LFence = Intrinsic::getDeclaration(M, Intrinsic::x86_sse2_lfence);
    Builder.CreateCall(LFence);
  }
  
  /// Insert MFENCE (memory fence) for ordering
  static void insertMFENCE(IRBuilder<> &Builder) {
    Module *M = Builder.GetInsertBlock()->getModule();
    Function *MFence = Intrinsic::getDeclaration(M, Intrinsic::x86_sse2_mfence);
    Builder.CreateCall(MFence);
  }
  
  /// Insert SFENCE (store fence)
  static void insertSFENCE(IRBuilder<> &Builder) {
    Module *M = Builder.GetInsertBlock()->getModule();
    Function *SFence = Intrinsic::getDeclaration(M, Intrinsic::x86_sse_sfence);
    Builder.CreateCall(SFence);
  }
  
  /// Insert CLFLUSH (cache line flush)
  static void insertCLFLUSH(IRBuilder<> &Builder, Value *Addr) {
    Module *M = Builder.GetInsertBlock()->getModule();
    Function *CLFlush = Intrinsic::getDeclaration(M, Intrinsic::x86_sse2_clflush);
    
    // CLFLUSH takes i8* pointer
    Type *I8PtrTy = Builder.getInt8PtrTy();
    Value *I8Ptr = Builder.CreateBitCast(Addr, I8PtrTy);
    
    Builder.CreateCall(CLFlush, {I8Ptr});
  }
  
  /// Insert CLFLUSHOPT (optimized cache line flush)
  static void insertCLFLUSHOPT(IRBuilder<> &Builder, Value *Addr) {
    // CLFLUSHOPT via inline assembly (LLVM may not have intrinsic)
    Module *M = Builder.GetInsertBlock()->getModule();
    LLVMContext &Ctx = M->getContext();
    
    Type *I8PtrTy = Builder.getInt8PtrTy();
    Value *I8Ptr = Builder.CreateBitCast(Addr, I8PtrTy);
    
    // Inline asm: clflushopt (%rdi)
    FunctionType *AsmFnTy = FunctionType::get(
        Builder.getVoidTy(), {I8PtrTy}, false);
    
    InlineAsm *Asm = InlineAsm::get(
        AsmFnTy,
        "clflushopt ($0)",
        "r,~{memory}",
        /*hasSideEffects=*/true);
    
    Builder.CreateCall(Asm, {I8Ptr});
  }
  
  /// Insert CLWB (cache line write-back)
  static void insertCLWB(IRBuilder<> &Builder, Value *Addr) {
    // CLWB via inline assembly
    Module *M = Builder.GetInsertBlock()->getModule();
    
    Type *I8PtrTy = Builder.getInt8PtrTy();
    Value *I8Ptr = Builder.CreateBitCast(Addr, I8PtrTy);
    
    // Inline asm: clwb (%rdi)
    FunctionType *AsmFnTy = FunctionType::get(
        Builder.getVoidTy(), {I8PtrTy}, false);
    
    InlineAsm *Asm = InlineAsm::get(
        AsmFnTy,
        "clwb ($0)",
        "r,~{memory}",
        /*hasSideEffects=*/true);
    
    Builder.CreateCall(Asm, {I8Ptr});
  }
  
  /// Insert VERW (for MD_CLEAR / MDS mitigation)
  static void insertVERW(IRBuilder<> &Builder) {
    // VERW via inline assembly
    Module *M = Builder.GetInsertBlock()->getModule();
    
    // Inline asm: verw (%rsp)
    FunctionType *AsmFnTy = FunctionType::get(Builder.getVoidTy(), {}, false);
    
    InlineAsm *Asm = InlineAsm::get(
        AsmFnTy,
        "subq $$8, %rsp; movw $$0, (%rsp); verw (%rsp); addq $$8, %rsp",
        "~{memory},~{rsp}",
        /*hasSideEffects=*/true);
    
    Builder.CreateCall(Asm);
  }
  
  /// Insert VPDPBUSD (AVX-VNNI multiply-accumulate)
  /// dst[i] += src1[i] * src2[i] (INT8 x INT8 -> INT32)
  static Value *insertVPDPBUSD256(IRBuilder<> &Builder, Value *Dst, 
                                   Value *Src1, Value *Src2) {
    Module *M = Builder.GetInsertBlock()->getModule();
    
    // x86_avx512_vpdpbusd_256
    Function *VPDPBUSD = Intrinsic::getDeclaration(
        M, Intrinsic::x86_avx512_vpdpbusd_256);
    
    return Builder.CreateCall(VPDPBUSD, {Dst, Src1, Src2});
  }
  
  /// Insert RDRAND (hardware RNG)
  static Value *insertRDRAND16(IRBuilder<> &Builder) {
    Module *M = Builder.GetInsertBlock()->getModule();
    Function *RdRand = Intrinsic::getDeclaration(M, Intrinsic::x86_rdrand_16);
    return Builder.CreateCall(RdRand);
  }
  
  static Value *insertRDRAND32(IRBuilder<> &Builder) {
    Module *M = Builder.GetInsertBlock()->getModule();
    Function *RdRand = Intrinsic::getDeclaration(M, Intrinsic::x86_rdrand_32);
    return Builder.CreateCall(RdRand);
  }
  
  static Value *insertRDRAND64(IRBuilder<> &Builder) {
    Module *M = Builder.GetInsertBlock()->getModule();
    Function *RdRand = Intrinsic::getDeclaration(M, Intrinsic::x86_rdrand_64);
    return Builder.CreateCall(RdRand);
  }
  
  /// Insert RDSEED (hardware entropy)
  static Value *insertRDSEED16(IRBuilder<> &Builder) {
    Module *M = Builder.GetInsertBlock()->getModule();
    Function *RdSeed = Intrinsic::getDeclaration(M, Intrinsic::x86_rdseed_16);
    return Builder.CreateCall(RdSeed);
  }
  
  static Value *insertRDSEED32(IRBuilder<> &Builder) {
    Module *M = Builder.GetInsertBlock()->getModule();
    Function *RdSeed = Intrinsic::getDeclaration(M, Intrinsic::x86_rdseed_32);
    return Builder.CreateCall(RdSeed);
  }
  
  static Value *insertRDSEED64(IRBuilder<> &Builder) {
    Module *M = Builder.GetInsertBlock()->getModule();
    Function *RdSeed = Intrinsic::getDeclaration(M, Intrinsic::x86_rdseed_64);
    return Builder.CreateCall(RdSeed);
  }
  
  /// Insert AES-NI encrypt
  static Value *insertAESENC(IRBuilder<> &Builder, Value *State, Value *RoundKey) {
    Module *M = Builder.GetInsertBlock()->getModule();
    Function *AesEnc = Intrinsic::getDeclaration(M, Intrinsic::x86_aesni_aesenc);
    return Builder.CreateCall(AesEnc, {State, RoundKey});
  }
  
  /// Insert SHA-256 message schedule update
  static Value *insertSHA256MSG1(IRBuilder<> &Builder, Value *State, Value *Data) {
    Module *M = Builder.GetInsertBlock()->getModule();
    Function *Sha256Msg1 = Intrinsic::getDeclaration(M, Intrinsic::x86_sha256msg1);
    return Builder.CreateCall(Sha256Msg1, {State, Data});
  }
};

} // namespace dsmil
} // namespace llvm

#endif // LLVM_TRANSFORMS_DSMIL_INTRINSICS_H
