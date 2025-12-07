; ModuleID = '/workspace/dsmil/llvm-passes/test_ai_kernels.c'
source_filename = "/workspace/dsmil/llvm-passes/test_ai_kernels.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [37 x i8] c"Benchmark: GEMM INT8 (%d x %d x %d)\0A\00", align 1
@.str.1 = private unnamed_addr constant [29 x i8] c"  Result: C[0]=%d, C[%d]=%d\0A\00", align 1
@.str.2 = private unnamed_addr constant [52 x i8] c"Benchmark: Conv2D INT8 (%dx%d input, %dx%d kernel)\0A\00", align 1
@.str.3 = private unnamed_addr constant [24 x i8] c"  Result: output[0]=%d\0A\00", align 1
@.str.15 = private unnamed_addr constant [15 x i8] c"dsmil_layer(7)\00", section "llvm.metadata"
@.str.16 = private unnamed_addr constant [47 x i8] c"/workspace/dsmil/llvm-passes/test_ai_kernels.c\00", section "llvm.metadata"
@.str.17 = private unnamed_addr constant [17 x i8] c"dsmil_device(47)\00", section "llvm.metadata"
@llvm.global.annotations = appending global [8 x { ptr, ptr, ptr, i32, ptr }] [{ ptr, ptr, ptr, i32, ptr } { ptr @gemm_int8, ptr @.str.15, ptr @.str.16, i32 21, ptr null }, { ptr, ptr, ptr, i32, ptr } { ptr @gemm_int8, ptr @.str.17, ptr @.str.16, i32 21, ptr null }, { ptr, ptr, ptr, i32, ptr } { ptr @depthwise_conv_int8, ptr @.str.15, ptr @.str.16, i32 98, ptr null }, { ptr, ptr, ptr, i32, ptr } { ptr @gemm_int8_blocked, ptr @.str.15, ptr @.str.16, i32 38, ptr null }, { ptr, ptr, ptr, i32, ptr } { ptr @matvec_int8, ptr @.str.15, ptr @.str.16, i32 153, ptr null }, { ptr, ptr, ptr, i32, ptr } { ptr @batched_matvec_int8, ptr @.str.15, ptr @.str.16, i32 168, ptr null }, { ptr, ptr, ptr, i32, ptr } { ptr @attention_qk_int8, ptr @.str.15, ptr @.str.16, i32 129, ptr null }, { ptr, ptr, ptr, i32, ptr } { ptr @conv2d_int8, ptr @.str.15, ptr @.str.16, i32 70, ptr null }], section "llvm.metadata"
@str.18 = private unnamed_addr constant [56 x i8] c"DSLLVM Phase 3: AI Kernel Tests (AVX-VNNI Optimization)\00", align 1
@str.19 = private unnamed_addr constant [60 x i8] c"==========================================================\0A\00", align 1
@str.20 = private unnamed_addr constant [19 x i8] c"Small tests (4x4):\00", align 1
@str.21 = private unnamed_addr constant [29 x i8] c"Medium tests (32x32, 64x64):\00", align 1
@str.22 = private unnamed_addr constant [19 x i8] c"Convolution tests:\00", align 1
@str.23 = private unnamed_addr constant [28 x i8] c"Large GEMM (LLM inference):\00", align 1
@str.25 = private unnamed_addr constant [20 x i8] c"All tests complete!\00", align 1
@str.26 = private unnamed_addr constant [53 x i8] c"Compile with: dsmil-clang -fdsllvm-ai-accelerate -O3\00", align 1
@str.27 = private unnamed_addr constant [48 x i8] c"Expected: VPDPBUSD intrinsics in generated code\00", align 1
@str.28 = private unnamed_addr constant [59 x i8] c"==========================================================\00", align 1

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define dso_local void @gemm_int8(ptr nocapture noundef readonly %0, ptr nocapture noundef readonly %1, ptr nocapture noundef writeonly %2, i32 noundef %3, i32 noundef %4, i32 noundef %5) #0 {
  %7 = icmp sgt i32 %3, 0
  br i1 %7, label %8, label %28

8:                                                ; preds = %6
  %9 = icmp sgt i32 %4, 0
  %10 = icmp sgt i32 %5, 0
  %11 = sext i32 %4 to i64
  %12 = zext nneg i32 %3 to i64
  %13 = zext nneg i32 %4 to i64
  %14 = zext i32 %5 to i64
  %15 = and i64 %14, 3
  %16 = icmp ult i32 %5, 4
  %17 = and i64 %14, 2147483644
  %18 = icmp eq i64 %15, 0
  br label %19

19:                                               ; preds = %8, %33
  %20 = phi i64 [ 0, %8 ], [ %34, %33 ]
  br i1 %9, label %21, label %33

21:                                               ; preds = %19
  %22 = mul nsw i64 %20, %11
  %23 = trunc i64 %20 to i32
  %24 = mul i32 %23, %5
  %25 = zext i32 %24 to i64
  %26 = getelementptr i8, ptr %0, i64 %25
  %27 = getelementptr i32, ptr %2, i64 %22
  br label %29

28:                                               ; preds = %33, %6
  ret void

29:                                               ; preds = %21, %56
  %30 = phi i64 [ 0, %21 ], [ %59, %56 ]
  br i1 %10, label %31, label %56

31:                                               ; preds = %29
  %32 = getelementptr i8, ptr %1, i64 %30
  br i1 %16, label %36, label %61

33:                                               ; preds = %56, %19
  %34 = add nuw nsw i64 %20, 1
  %35 = icmp eq i64 %34, %12
  br i1 %35, label %28, label %19, !llvm.loop !5

36:                                               ; preds = %61, %31
  %37 = phi i32 [ undef, %31 ], [ %103, %61 ]
  %38 = phi i64 [ 0, %31 ], [ %104, %61 ]
  %39 = phi i32 [ 0, %31 ], [ %103, %61 ]
  br i1 %18, label %56, label %40

40:                                               ; preds = %36, %40
  %41 = phi i64 [ %53, %40 ], [ %38, %36 ]
  %42 = phi i32 [ %52, %40 ], [ %39, %36 ]
  %43 = phi i64 [ %54, %40 ], [ 0, %36 ]
  %44 = getelementptr i8, ptr %26, i64 %41
  %45 = load i8, ptr %44, align 1, !tbaa !7
  %46 = sext i8 %45 to i32
  %47 = mul nsw i64 %41, %11
  %48 = getelementptr i8, ptr %32, i64 %47
  %49 = load i8, ptr %48, align 1, !tbaa !7
  %50 = sext i8 %49 to i32
  %51 = mul nsw i32 %50, %46
  %52 = add nsw i32 %51, %42
  %53 = add nuw nsw i64 %41, 1
  %54 = add i64 %43, 1
  %55 = icmp eq i64 %54, %15
  br i1 %55, label %56, label %40, !llvm.loop !10

56:                                               ; preds = %36, %40, %29
  %57 = phi i32 [ 0, %29 ], [ %37, %36 ], [ %52, %40 ]
  %58 = getelementptr i32, ptr %27, i64 %30
  store i32 %57, ptr %58, align 4, !tbaa !12
  %59 = add nuw nsw i64 %30, 1
  %60 = icmp eq i64 %59, %13
  br i1 %60, label %33, label %29, !llvm.loop !14

61:                                               ; preds = %31, %61
  %62 = phi i64 [ %104, %61 ], [ 0, %31 ]
  %63 = phi i32 [ %103, %61 ], [ 0, %31 ]
  %64 = phi i64 [ %105, %61 ], [ 0, %31 ]
  %65 = getelementptr i8, ptr %26, i64 %62
  %66 = load i8, ptr %65, align 1, !tbaa !7
  %67 = sext i8 %66 to i32
  %68 = mul nsw i64 %62, %11
  %69 = getelementptr i8, ptr %32, i64 %68
  %70 = load i8, ptr %69, align 1, !tbaa !7
  %71 = sext i8 %70 to i32
  %72 = mul nsw i32 %71, %67
  %73 = add nsw i32 %72, %63
  %74 = or disjoint i64 %62, 1
  %75 = getelementptr i8, ptr %26, i64 %74
  %76 = load i8, ptr %75, align 1, !tbaa !7
  %77 = sext i8 %76 to i32
  %78 = mul nsw i64 %74, %11
  %79 = getelementptr i8, ptr %32, i64 %78
  %80 = load i8, ptr %79, align 1, !tbaa !7
  %81 = sext i8 %80 to i32
  %82 = mul nsw i32 %81, %77
  %83 = add nsw i32 %82, %73
  %84 = or disjoint i64 %62, 2
  %85 = getelementptr i8, ptr %26, i64 %84
  %86 = load i8, ptr %85, align 1, !tbaa !7
  %87 = sext i8 %86 to i32
  %88 = mul nsw i64 %84, %11
  %89 = getelementptr i8, ptr %32, i64 %88
  %90 = load i8, ptr %89, align 1, !tbaa !7
  %91 = sext i8 %90 to i32
  %92 = mul nsw i32 %91, %87
  %93 = add nsw i32 %92, %83
  %94 = or disjoint i64 %62, 3
  %95 = getelementptr i8, ptr %26, i64 %94
  %96 = load i8, ptr %95, align 1, !tbaa !7
  %97 = sext i8 %96 to i32
  %98 = mul nsw i64 %94, %11
  %99 = getelementptr i8, ptr %32, i64 %98
  %100 = load i8, ptr %99, align 1, !tbaa !7
  %101 = sext i8 %100 to i32
  %102 = mul nsw i32 %101, %97
  %103 = add nsw i32 %102, %93
  %104 = add nuw nsw i64 %62, 4
  %105 = add i64 %64, 4
  %106 = icmp eq i64 %105, %17
  br i1 %106, label %36, label %61, !llvm.loop !15
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define dso_local void @gemm_int8_blocked(ptr nocapture noundef readonly %0, ptr nocapture noundef readonly %1, ptr nocapture noundef %2, i32 noundef %3, i32 noundef %4, i32 noundef %5) #0 {
  %7 = icmp sgt i32 %3, 0
  br i1 %7, label %8, label %21

8:                                                ; preds = %6
  %9 = icmp sgt i32 %4, 0
  %10 = icmp sgt i32 %5, 0
  %11 = sext i32 %4 to i64
  %12 = sext i32 %5 to i64
  %13 = icmp eq i32 %4, 1
  br label %14

14:                                               ; preds = %8, %29
  %15 = phi i64 [ 0, %8 ], [ %30, %29 ]
  br i1 %9, label %16, label %29

16:                                               ; preds = %14
  %17 = trunc i64 %15 to i32
  %18 = add nuw nsw i32 %17, 32
  %19 = tail call i32 @llvm.smin.i32(i32 %18, i32 %3)
  %20 = sext i32 %19 to i64
  br label %22

21:                                               ; preds = %29, %6
  ret void

22:                                               ; preds = %16, %36
  %23 = phi i64 [ 0, %16 ], [ %37, %36 ]
  br i1 %10, label %24, label %36

24:                                               ; preds = %22
  %25 = trunc i64 %23 to i32
  %26 = add nuw nsw i32 %25, 32
  %27 = tail call i32 @llvm.smin.i32(i32 %26, i32 %4)
  %28 = sext i32 %27 to i64
  br label %40

29:                                               ; preds = %36, %14
  %30 = add nuw nsw i64 %15, 32
  %31 = trunc i64 %30 to i32
  %32 = icmp slt i32 %31, %3
  br i1 %32, label %14, label %21, !llvm.loop !16

33:                                               ; preds = %67
  %34 = icmp slt i32 %50, %5
  %35 = add i64 %41, 1
  br i1 %34, label %40, label %36, !llvm.loop !17

36:                                               ; preds = %33, %22
  %37 = add nuw nsw i64 %23, 32
  %38 = trunc i64 %37 to i32
  %39 = icmp slt i32 %38, %4
  br i1 %39, label %22, label %29, !llvm.loop !18

40:                                               ; preds = %33, %24
  %41 = phi i64 [ %35, %33 ], [ 0, %24 ]
  %42 = phi i64 [ %49, %33 ], [ 0, %24 ]
  %43 = add nuw i64 %42, 32
  %44 = tail call i64 @llvm.smin.i64(i64 %43, i64 %12)
  %45 = or disjoint i64 %42, 1
  %46 = tail call i64 @llvm.smax.i64(i64 %44, i64 %45)
  %47 = shl i64 %41, 5
  %48 = sub i64 %46, %47
  %49 = add nuw nsw i64 %42, 32
  %50 = trunc i64 %49 to i32
  %51 = tail call i32 @llvm.smin.i32(i32 %50, i32 %5)
  %52 = sext i32 %51 to i64
  %53 = icmp ugt i64 %48, 7
  %54 = and i1 %53, %13
  %55 = and i64 %46, 7
  %56 = sub i64 %48, %55
  %57 = add i64 %42, %56
  %58 = icmp eq i64 %55, 0
  br label %59

59:                                               ; preds = %67, %40
  %60 = phi i64 [ %15, %40 ], [ %68, %67 ]
  %61 = mul nsw i64 %60, %11
  %62 = trunc i64 %60 to i32
  %63 = mul i32 %62, %5
  %64 = zext i32 %63 to i64
  %65 = getelementptr i32, ptr %2, i64 %61
  %66 = getelementptr i8, ptr %0, i64 %64
  br label %70

67:                                               ; preds = %107
  %68 = add nuw nsw i64 %60, 1
  %69 = icmp slt i64 %68, %20
  br i1 %69, label %59, label %33, !llvm.loop !19

70:                                               ; preds = %107, %59
  %71 = phi i64 [ %23, %59 ], [ %109, %107 ]
  %72 = getelementptr i32, ptr %65, i64 %71
  %73 = load i32, ptr %72, align 4, !tbaa !12
  %74 = getelementptr i8, ptr %1, i64 %71
  br i1 %54, label %75, label %104

75:                                               ; preds = %70
  %76 = insertelement <4 x i32> <i32 poison, i32 0, i32 0, i32 0>, i32 %73, i64 0
  br label %77

77:                                               ; preds = %77, %75
  %78 = phi i64 [ 0, %75 ], [ %99, %77 ]
  %79 = phi <4 x i32> [ %76, %75 ], [ %97, %77 ]
  %80 = phi <4 x i32> [ zeroinitializer, %75 ], [ %98, %77 ]
  %81 = add i64 %42, %78
  %82 = getelementptr i8, ptr %66, i64 %81
  %83 = getelementptr i8, ptr %82, i64 4
  %84 = load <4 x i8>, ptr %82, align 1, !tbaa !7
  %85 = load <4 x i8>, ptr %83, align 1, !tbaa !7
  %86 = sext <4 x i8> %84 to <4 x i32>
  %87 = sext <4 x i8> %85 to <4 x i32>
  %88 = mul nuw nsw i64 %81, %11
  %89 = getelementptr i8, ptr %74, i64 %88
  %90 = getelementptr i8, ptr %89, i64 4
  %91 = load <4 x i8>, ptr %89, align 1, !tbaa !7
  %92 = load <4 x i8>, ptr %90, align 1, !tbaa !7
  %93 = sext <4 x i8> %91 to <4 x i32>
  %94 = sext <4 x i8> %92 to <4 x i32>
  %95 = mul nsw <4 x i32> %93, %86
  %96 = mul nsw <4 x i32> %94, %87
  %97 = add <4 x i32> %95, %79
  %98 = add <4 x i32> %96, %80
  %99 = add nuw i64 %78, 8
  %100 = icmp eq i64 %99, %56
  br i1 %100, label %101, label %77, !llvm.loop !20

101:                                              ; preds = %77
  %102 = add <4 x i32> %98, %97
  %103 = tail call i32 @llvm.vector.reduce.add.v4i32(<4 x i32> %102)
  br i1 %58, label %107, label %104

104:                                              ; preds = %70, %101
  %105 = phi i64 [ %42, %70 ], [ %57, %101 ]
  %106 = phi i32 [ %73, %70 ], [ %103, %101 ]
  br label %111

107:                                              ; preds = %111, %101
  %108 = phi i32 [ %103, %101 ], [ %122, %111 ]
  store i32 %108, ptr %72, align 4, !tbaa !12
  %109 = add nuw nsw i64 %71, 1
  %110 = icmp slt i64 %109, %28
  br i1 %110, label %70, label %67, !llvm.loop !23

111:                                              ; preds = %104, %111
  %112 = phi i64 [ %123, %111 ], [ %105, %104 ]
  %113 = phi i32 [ %122, %111 ], [ %106, %104 ]
  %114 = getelementptr i8, ptr %66, i64 %112
  %115 = load i8, ptr %114, align 1, !tbaa !7
  %116 = sext i8 %115 to i32
  %117 = mul nsw i64 %112, %11
  %118 = getelementptr i8, ptr %74, i64 %117
  %119 = load i8, ptr %118, align 1, !tbaa !7
  %120 = sext i8 %119 to i32
  %121 = mul nsw i32 %120, %116
  %122 = add nsw i32 %121, %113
  %123 = add nuw nsw i64 %112, 1
  %124 = icmp slt i64 %123, %52
  br i1 %124, label %111, label %107, !llvm.loop !24
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define dso_local void @conv2d_int8(ptr nocapture noundef readonly %0, ptr nocapture noundef readonly %1, ptr nocapture noundef writeonly %2, i32 noundef %3, i32 noundef %4, i32 noundef %5, i32 noundef %6) #0 {
  %8 = sub i32 %4, %6
  %9 = add i32 %8, 1
  %10 = icmp slt i32 %3, %5
  br i1 %10, label %40, label %11

11:                                               ; preds = %7
  %12 = icmp slt i32 %8, 0
  %13 = icmp sgt i32 %5, 0
  %14 = icmp sgt i32 %6, 0
  %15 = sext i32 %6 to i64
  %16 = add i32 %3, 1
  %17 = sub i32 %16, %5
  %18 = zext i32 %17 to i64
  %19 = zext i32 %9 to i64
  %20 = zext nneg i32 %5 to i64
  %21 = zext i32 %6 to i64
  %22 = add nsw i64 %21, -1
  %23 = zext i32 %4 to i64
  %24 = icmp ult i32 %6, 8
  %25 = trunc i64 %22 to i32
  %26 = icmp ugt i64 %22, 4294967295
  %27 = and i64 %21, 2147483640
  %28 = icmp eq i64 %27, %21
  %29 = and i64 %21, 1
  %30 = icmp eq i64 %29, 0
  %31 = add nsw i64 %21, -1
  br label %32

32:                                               ; preds = %11, %46
  %33 = phi i64 [ 0, %11 ], [ %47, %46 ]
  %34 = mul i64 %33, %23
  br i1 %12, label %46, label %35

35:                                               ; preds = %32
  %36 = trunc i64 %33 to i32
  %37 = mul i32 %9, %36
  %38 = zext i32 %37 to i64
  %39 = getelementptr i32, ptr %2, i64 %38
  br label %41

40:                                               ; preds = %46, %7
  ret void

41:                                               ; preds = %35, %117
  %42 = phi i64 [ 0, %35 ], [ %120, %117 ]
  %43 = add i64 %34, %42
  br i1 %13, label %44, label %117

44:                                               ; preds = %41
  %45 = trunc i64 %42 to i32
  br label %49

46:                                               ; preds = %117, %32
  %47 = add nuw nsw i64 %33, 1
  %48 = icmp eq i64 %47, %18
  br i1 %48, label %40, label %32, !llvm.loop !25

49:                                               ; preds = %44, %122
  %50 = phi i64 [ 0, %44 ], [ %124, %122 ]
  %51 = phi i32 [ 0, %44 ], [ %123, %122 ]
  %52 = mul i64 %50, %23
  %53 = add i64 %43, %52
  %54 = trunc i64 %53 to i32
  br i1 %14, label %55, label %122

55:                                               ; preds = %49
  %56 = add nuw nsw i64 %50, %33
  %57 = trunc i64 %56 to i32
  %58 = mul i32 %57, %4
  %59 = add i32 %58, %45
  %60 = mul nsw i64 %50, %15
  %61 = getelementptr i8, ptr %1, i64 %60
  br i1 %24, label %96, label %62

62:                                               ; preds = %55
  %63 = add i32 %54, %25
  %64 = icmp slt i32 %63, %54
  %65 = or i1 %64, %26
  br i1 %65, label %96, label %66

66:                                               ; preds = %62
  %67 = insertelement <4 x i32> <i32 poison, i32 0, i32 0, i32 0>, i32 %51, i64 0
  br label %68

68:                                               ; preds = %68, %66
  %69 = phi i64 [ 0, %66 ], [ %91, %68 ]
  %70 = phi <4 x i32> [ %67, %66 ], [ %89, %68 ]
  %71 = phi <4 x i32> [ zeroinitializer, %66 ], [ %90, %68 ]
  %72 = trunc i64 %69 to i32
  %73 = add i32 %59, %72
  %74 = sext i32 %73 to i64
  %75 = getelementptr inbounds i8, ptr %0, i64 %74
  %76 = getelementptr inbounds i8, ptr %75, i64 4
  %77 = load <4 x i8>, ptr %75, align 1, !tbaa !7
  %78 = load <4 x i8>, ptr %76, align 1, !tbaa !7
  %79 = sext <4 x i8> %77 to <4 x i32>
  %80 = sext <4 x i8> %78 to <4 x i32>
  %81 = getelementptr i8, ptr %61, i64 %69
  %82 = getelementptr i8, ptr %81, i64 4
  %83 = load <4 x i8>, ptr %81, align 1, !tbaa !7
  %84 = load <4 x i8>, ptr %82, align 1, !tbaa !7
  %85 = sext <4 x i8> %83 to <4 x i32>
  %86 = sext <4 x i8> %84 to <4 x i32>
  %87 = mul nsw <4 x i32> %85, %79
  %88 = mul nsw <4 x i32> %86, %80
  %89 = add <4 x i32> %87, %70
  %90 = add <4 x i32> %88, %71
  %91 = add nuw i64 %69, 8
  %92 = icmp eq i64 %91, %27
  br i1 %92, label %93, label %68, !llvm.loop !26

93:                                               ; preds = %68
  %94 = add <4 x i32> %90, %89
  %95 = tail call i32 @llvm.vector.reduce.add.v4i32(<4 x i32> %94)
  br i1 %28, label %122, label %96

96:                                               ; preds = %62, %55, %93
  %97 = phi i64 [ 0, %62 ], [ 0, %55 ], [ %27, %93 ]
  %98 = phi i32 [ %51, %62 ], [ %51, %55 ], [ %95, %93 ]
  br i1 %30, label %112, label %99

99:                                               ; preds = %96
  %100 = trunc i64 %97 to i32
  %101 = add i32 %59, %100
  %102 = sext i32 %101 to i64
  %103 = getelementptr inbounds i8, ptr %0, i64 %102
  %104 = load i8, ptr %103, align 1, !tbaa !7
  %105 = sext i8 %104 to i32
  %106 = getelementptr i8, ptr %61, i64 %97
  %107 = load i8, ptr %106, align 1, !tbaa !7
  %108 = sext i8 %107 to i32
  %109 = mul nsw i32 %108, %105
  %110 = add nsw i32 %109, %98
  %111 = or disjoint i64 %97, 1
  br label %112

112:                                              ; preds = %99, %96
  %113 = phi i32 [ undef, %96 ], [ %110, %99 ]
  %114 = phi i64 [ %97, %96 ], [ %111, %99 ]
  %115 = phi i32 [ %98, %96 ], [ %110, %99 ]
  %116 = icmp eq i64 %97, %31
  br i1 %116, label %122, label %126

117:                                              ; preds = %122, %41
  %118 = phi i32 [ 0, %41 ], [ %123, %122 ]
  %119 = getelementptr i32, ptr %39, i64 %42
  store i32 %118, ptr %119, align 4, !tbaa !12
  %120 = add nuw nsw i64 %42, 1
  %121 = icmp eq i64 %120, %19
  br i1 %121, label %46, label %41, !llvm.loop !27

122:                                              ; preds = %112, %126, %93, %49
  %123 = phi i32 [ %51, %49 ], [ %95, %93 ], [ %113, %112 ], [ %151, %126 ]
  %124 = add nuw nsw i64 %50, 1
  %125 = icmp eq i64 %124, %20
  br i1 %125, label %117, label %49, !llvm.loop !28

126:                                              ; preds = %112, %126
  %127 = phi i64 [ %152, %126 ], [ %114, %112 ]
  %128 = phi i32 [ %151, %126 ], [ %115, %112 ]
  %129 = trunc i64 %127 to i32
  %130 = add i32 %59, %129
  %131 = sext i32 %130 to i64
  %132 = getelementptr inbounds i8, ptr %0, i64 %131
  %133 = load i8, ptr %132, align 1, !tbaa !7
  %134 = sext i8 %133 to i32
  %135 = getelementptr i8, ptr %61, i64 %127
  %136 = load i8, ptr %135, align 1, !tbaa !7
  %137 = sext i8 %136 to i32
  %138 = mul nsw i32 %137, %134
  %139 = add nsw i32 %138, %128
  %140 = add nuw nsw i64 %127, 1
  %141 = trunc i64 %140 to i32
  %142 = add i32 %59, %141
  %143 = sext i32 %142 to i64
  %144 = getelementptr inbounds i8, ptr %0, i64 %143
  %145 = load i8, ptr %144, align 1, !tbaa !7
  %146 = sext i8 %145 to i32
  %147 = getelementptr i8, ptr %61, i64 %140
  %148 = load i8, ptr %147, align 1, !tbaa !7
  %149 = sext i8 %148 to i32
  %150 = mul nsw i32 %149, %146
  %151 = add nsw i32 %150, %139
  %152 = add nuw nsw i64 %127, 2
  %153 = icmp eq i64 %152, %21
  br i1 %153, label %122, label %126, !llvm.loop !29
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define dso_local void @depthwise_conv_int8(ptr nocapture noundef readonly %0, ptr nocapture noundef readonly %1, ptr nocapture noundef writeonly %2, i32 noundef %3, i32 noundef %4, i32 noundef %5, i32 noundef %6, i32 noundef %7) #0 {
  %9 = sub i32 %4, %7
  %10 = add i32 %9, 1
  %11 = icmp sgt i32 %5, 0
  br i1 %11, label %12, label %43

12:                                               ; preds = %8
  %13 = icmp slt i32 %3, %6
  %14 = icmp slt i32 %9, 0
  %15 = icmp sgt i32 %6, 0
  %16 = icmp sgt i32 %7, 0
  %17 = zext nneg i32 %5 to i64
  %18 = sext i32 %7 to i64
  %19 = add i32 %3, 1
  %20 = sub i32 %19, %6
  %21 = zext nneg i32 %5 to i64
  %22 = zext i32 %20 to i64
  %23 = zext i32 %10 to i64
  %24 = zext nneg i32 %6 to i64
  %25 = zext i32 %7 to i64
  %26 = add nsw i64 %25, -1
  %27 = zext i32 %4 to i64
  %28 = icmp ult i32 %7, 8
  %29 = icmp ne i32 %5, 1
  %30 = trunc i64 %26 to i32
  %31 = icmp ugt i64 %26, 4294967295
  %32 = and i64 %25, 2147483640
  %33 = icmp eq i64 %32, %25
  %34 = and i64 %25, 1
  %35 = icmp eq i64 %34, 0
  %36 = add nsw i64 %25, -1
  br label %37

37:                                               ; preds = %12, %50
  %38 = phi i64 [ 0, %12 ], [ %51, %50 ]
  br i1 %13, label %50, label %39

39:                                               ; preds = %37
  %40 = getelementptr i8, ptr %0, i64 %38
  %41 = getelementptr i8, ptr %1, i64 %38
  %42 = trunc i64 %38 to i32
  br label %44

43:                                               ; preds = %50, %8
  ret void

44:                                               ; preds = %39, %58
  %45 = phi i64 [ 0, %39 ], [ %59, %58 ]
  %46 = mul i64 %45, %27
  br i1 %14, label %58, label %47

47:                                               ; preds = %44
  %48 = trunc i64 %45 to i32
  %49 = mul i32 %10, %48
  br label %53

50:                                               ; preds = %58, %37
  %51 = add nuw nsw i64 %38, 1
  %52 = icmp eq i64 %51, %21
  br i1 %52, label %43, label %37, !llvm.loop !30

53:                                               ; preds = %47, %134
  %54 = phi i64 [ 0, %47 ], [ %142, %134 ]
  %55 = add i64 %46, %54
  br i1 %15, label %56, label %134

56:                                               ; preds = %53
  %57 = trunc i64 %54 to i32
  br label %61

58:                                               ; preds = %134, %44
  %59 = add nuw nsw i64 %45, 1
  %60 = icmp eq i64 %59, %22
  br i1 %60, label %50, label %44, !llvm.loop !31

61:                                               ; preds = %56, %144
  %62 = phi i64 [ 0, %56 ], [ %146, %144 ]
  %63 = phi i32 [ 0, %56 ], [ %145, %144 ]
  %64 = mul i64 %62, %27
  %65 = add i64 %55, %64
  %66 = trunc i64 %65 to i32
  br i1 %16, label %67, label %144

67:                                               ; preds = %61
  %68 = add nuw nsw i64 %62, %45
  %69 = trunc i64 %68 to i32
  %70 = mul i32 %69, %4
  %71 = add i32 %70, %57
  %72 = mul nsw i64 %62, %18
  br i1 %28, label %110, label %73

73:                                               ; preds = %67
  %74 = add i32 %66, %30
  %75 = icmp slt i32 %74, %66
  %76 = or i1 %75, %31
  %77 = or i1 %29, %76
  br i1 %77, label %110, label %78

78:                                               ; preds = %73
  %79 = insertelement <4 x i32> <i32 poison, i32 0, i32 0, i32 0>, i32 %63, i64 0
  br label %80

80:                                               ; preds = %80, %78
  %81 = phi i64 [ 0, %78 ], [ %105, %80 ]
  %82 = phi <4 x i32> [ %79, %78 ], [ %103, %80 ]
  %83 = phi <4 x i32> [ zeroinitializer, %78 ], [ %104, %80 ]
  %84 = trunc i64 %81 to i32
  %85 = add i32 %71, %84
  %86 = sext i32 %85 to i64
  %87 = add nuw nsw i64 %81, %72
  %88 = mul nsw i64 %87, %17
  %89 = getelementptr i8, ptr %40, i64 %86
  %90 = getelementptr i8, ptr %89, i64 4
  %91 = load <4 x i8>, ptr %89, align 1, !tbaa !7
  %92 = load <4 x i8>, ptr %90, align 1, !tbaa !7
  %93 = sext <4 x i8> %91 to <4 x i32>
  %94 = sext <4 x i8> %92 to <4 x i32>
  %95 = getelementptr i8, ptr %41, i64 %88
  %96 = getelementptr i8, ptr %95, i64 4
  %97 = load <4 x i8>, ptr %95, align 1, !tbaa !7
  %98 = load <4 x i8>, ptr %96, align 1, !tbaa !7
  %99 = sext <4 x i8> %97 to <4 x i32>
  %100 = sext <4 x i8> %98 to <4 x i32>
  %101 = mul nsw <4 x i32> %99, %93
  %102 = mul nsw <4 x i32> %100, %94
  %103 = add <4 x i32> %101, %82
  %104 = add <4 x i32> %102, %83
  %105 = add nuw i64 %81, 8
  %106 = icmp eq i64 %105, %32
  br i1 %106, label %107, label %80, !llvm.loop !32

107:                                              ; preds = %80
  %108 = add <4 x i32> %104, %103
  %109 = tail call i32 @llvm.vector.reduce.add.v4i32(<4 x i32> %108)
  br i1 %33, label %144, label %110

110:                                              ; preds = %73, %67, %107
  %111 = phi i64 [ 0, %73 ], [ 0, %67 ], [ %32, %107 ]
  %112 = phi i32 [ %63, %73 ], [ %63, %67 ], [ %109, %107 ]
  br i1 %35, label %129, label %113

113:                                              ; preds = %110
  %114 = trunc i64 %111 to i32
  %115 = add i32 %71, %114
  %116 = mul nsw i32 %115, %5
  %117 = sext i32 %116 to i64
  %118 = add nuw nsw i64 %111, %72
  %119 = mul nsw i64 %118, %17
  %120 = getelementptr i8, ptr %40, i64 %117
  %121 = load i8, ptr %120, align 1, !tbaa !7
  %122 = sext i8 %121 to i32
  %123 = getelementptr i8, ptr %41, i64 %119
  %124 = load i8, ptr %123, align 1, !tbaa !7
  %125 = sext i8 %124 to i32
  %126 = mul nsw i32 %125, %122
  %127 = add nsw i32 %126, %112
  %128 = or disjoint i64 %111, 1
  br label %129

129:                                              ; preds = %113, %110
  %130 = phi i32 [ undef, %110 ], [ %127, %113 ]
  %131 = phi i64 [ %111, %110 ], [ %128, %113 ]
  %132 = phi i32 [ %112, %110 ], [ %127, %113 ]
  %133 = icmp eq i64 %111, %36
  br i1 %133, label %144, label %148

134:                                              ; preds = %144, %53
  %135 = phi i32 [ 0, %53 ], [ %145, %144 ]
  %136 = trunc i64 %54 to i32
  %137 = add i32 %49, %136
  %138 = mul nsw i32 %137, %5
  %139 = add nuw nsw i32 %138, %42
  %140 = zext nneg i32 %139 to i64
  %141 = getelementptr inbounds i32, ptr %2, i64 %140
  store i32 %135, ptr %141, align 4, !tbaa !12
  %142 = add nuw nsw i64 %54, 1
  %143 = icmp eq i64 %142, %23
  br i1 %143, label %58, label %53, !llvm.loop !33

144:                                              ; preds = %129, %148, %107, %61
  %145 = phi i32 [ %63, %61 ], [ %109, %107 ], [ %130, %129 ], [ %179, %148 ]
  %146 = add nuw nsw i64 %62, 1
  %147 = icmp eq i64 %146, %24
  br i1 %147, label %134, label %61, !llvm.loop !34

148:                                              ; preds = %129, %148
  %149 = phi i64 [ %180, %148 ], [ %131, %129 ]
  %150 = phi i32 [ %179, %148 ], [ %132, %129 ]
  %151 = trunc i64 %149 to i32
  %152 = add i32 %71, %151
  %153 = mul nsw i32 %152, %5
  %154 = sext i32 %153 to i64
  %155 = add nuw nsw i64 %149, %72
  %156 = mul nsw i64 %155, %17
  %157 = getelementptr i8, ptr %40, i64 %154
  %158 = load i8, ptr %157, align 1, !tbaa !7
  %159 = sext i8 %158 to i32
  %160 = getelementptr i8, ptr %41, i64 %156
  %161 = load i8, ptr %160, align 1, !tbaa !7
  %162 = sext i8 %161 to i32
  %163 = mul nsw i32 %162, %159
  %164 = add nsw i32 %163, %150
  %165 = add nuw nsw i64 %149, 1
  %166 = trunc i64 %165 to i32
  %167 = add i32 %71, %166
  %168 = mul nsw i32 %167, %5
  %169 = sext i32 %168 to i64
  %170 = add nuw nsw i64 %165, %72
  %171 = mul nsw i64 %170, %17
  %172 = getelementptr i8, ptr %40, i64 %169
  %173 = load i8, ptr %172, align 1, !tbaa !7
  %174 = sext i8 %173 to i32
  %175 = getelementptr i8, ptr %41, i64 %171
  %176 = load i8, ptr %175, align 1, !tbaa !7
  %177 = sext i8 %176 to i32
  %178 = mul nsw i32 %177, %174
  %179 = add nsw i32 %178, %164
  %180 = add nuw nsw i64 %149, 2
  %181 = icmp eq i64 %180, %25
  br i1 %181, label %144, label %148, !llvm.loop !35
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define dso_local void @attention_qk_int8(ptr nocapture noundef readonly %0, ptr nocapture noundef readonly %1, ptr nocapture noundef writeonly %2, i32 noundef %3, i32 noundef %4) #0 {
  %6 = icmp sgt i32 %3, 0
  br i1 %6, label %7, label %24

7:                                                ; preds = %5
  %8 = icmp sgt i32 %4, 0
  %9 = zext nneg i32 %3 to i64
  %10 = zext i32 %4 to i64
  %11 = icmp ult i32 %4, 8
  %12 = and i64 %10, 2147483640
  %13 = icmp eq i64 %12, %10
  br label %14

14:                                               ; preds = %60, %7
  %15 = phi i64 [ 0, %7 ], [ %61, %60 ]
  %16 = trunc i64 %15 to i32
  %17 = mul i32 %16, %4
  %18 = zext i32 %17 to i64
  %19 = trunc i64 %15 to i32
  %20 = mul i32 %19, %3
  %21 = zext i32 %20 to i64
  %22 = getelementptr i8, ptr %0, i64 %18
  %23 = getelementptr i32, ptr %2, i64 %21
  br label %25

24:                                               ; preds = %60, %5
  ret void

25:                                               ; preds = %14, %63
  %26 = phi i64 [ 0, %14 ], [ %66, %63 ]
  br i1 %8, label %27, label %63

27:                                               ; preds = %25
  %28 = trunc i64 %26 to i32
  %29 = mul i32 %28, %4
  %30 = zext i32 %29 to i64
  %31 = getelementptr i8, ptr %1, i64 %30
  br i1 %11, label %57, label %32

32:                                               ; preds = %27, %32
  %33 = phi i64 [ %52, %32 ], [ 0, %27 ]
  %34 = phi <4 x i32> [ %50, %32 ], [ zeroinitializer, %27 ]
  %35 = phi <4 x i32> [ %51, %32 ], [ zeroinitializer, %27 ]
  %36 = getelementptr i8, ptr %22, i64 %33
  %37 = getelementptr i8, ptr %36, i64 4
  %38 = load <4 x i8>, ptr %36, align 1, !tbaa !7
  %39 = load <4 x i8>, ptr %37, align 1, !tbaa !7
  %40 = sext <4 x i8> %38 to <4 x i32>
  %41 = sext <4 x i8> %39 to <4 x i32>
  %42 = getelementptr i8, ptr %31, i64 %33
  %43 = getelementptr i8, ptr %42, i64 4
  %44 = load <4 x i8>, ptr %42, align 1, !tbaa !7
  %45 = load <4 x i8>, ptr %43, align 1, !tbaa !7
  %46 = sext <4 x i8> %44 to <4 x i32>
  %47 = sext <4 x i8> %45 to <4 x i32>
  %48 = mul nsw <4 x i32> %46, %40
  %49 = mul nsw <4 x i32> %47, %41
  %50 = add <4 x i32> %48, %34
  %51 = add <4 x i32> %49, %35
  %52 = add nuw i64 %33, 8
  %53 = icmp eq i64 %52, %12
  br i1 %53, label %54, label %32, !llvm.loop !36

54:                                               ; preds = %32
  %55 = add <4 x i32> %51, %50
  %56 = tail call i32 @llvm.vector.reduce.add.v4i32(<4 x i32> %55)
  br i1 %13, label %63, label %57

57:                                               ; preds = %27, %54
  %58 = phi i64 [ 0, %27 ], [ %12, %54 ]
  %59 = phi i32 [ 0, %27 ], [ %56, %54 ]
  br label %68

60:                                               ; preds = %63
  %61 = add nuw nsw i64 %15, 1
  %62 = icmp eq i64 %61, %9
  br i1 %62, label %24, label %14, !llvm.loop !37

63:                                               ; preds = %68, %54, %25
  %64 = phi i32 [ 0, %25 ], [ %56, %54 ], [ %78, %68 ]
  %65 = getelementptr i32, ptr %23, i64 %26
  store i32 %64, ptr %65, align 4, !tbaa !12
  %66 = add nuw nsw i64 %26, 1
  %67 = icmp eq i64 %66, %9
  br i1 %67, label %60, label %25, !llvm.loop !38

68:                                               ; preds = %57, %68
  %69 = phi i64 [ %79, %68 ], [ %58, %57 ]
  %70 = phi i32 [ %78, %68 ], [ %59, %57 ]
  %71 = getelementptr i8, ptr %22, i64 %69
  %72 = load i8, ptr %71, align 1, !tbaa !7
  %73 = sext i8 %72 to i32
  %74 = getelementptr i8, ptr %31, i64 %69
  %75 = load i8, ptr %74, align 1, !tbaa !7
  %76 = sext i8 %75 to i32
  %77 = mul nsw i32 %76, %73
  %78 = add nsw i32 %77, %70
  %79 = add nuw nsw i64 %69, 1
  %80 = icmp eq i64 %79, %10
  br i1 %80, label %63, label %68, !llvm.loop !39
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define dso_local void @matvec_int8(ptr nocapture noundef readonly %0, ptr nocapture noundef readonly %1, ptr nocapture noundef writeonly %2, i32 noundef %3, i32 noundef %4) #0 {
  %6 = icmp sgt i32 %4, 0
  br i1 %6, label %7, label %49

7:                                                ; preds = %5
  %8 = icmp sgt i32 %3, 0
  %9 = zext nneg i32 %4 to i64
  %10 = zext i32 %3 to i64
  %11 = icmp ult i32 %3, 8
  %12 = and i64 %10, 2147483640
  %13 = icmp eq i64 %12, %10
  br label %14

14:                                               ; preds = %7, %50
  %15 = phi i64 [ 0, %7 ], [ %53, %50 ]
  br i1 %8, label %16, label %50

16:                                               ; preds = %14
  %17 = trunc i64 %15 to i32
  %18 = mul i32 %17, %3
  %19 = zext i32 %18 to i64
  %20 = getelementptr i8, ptr %1, i64 %19
  br i1 %11, label %46, label %21

21:                                               ; preds = %16, %21
  %22 = phi i64 [ %41, %21 ], [ 0, %16 ]
  %23 = phi <4 x i32> [ %39, %21 ], [ zeroinitializer, %16 ]
  %24 = phi <4 x i32> [ %40, %21 ], [ zeroinitializer, %16 ]
  %25 = getelementptr inbounds i8, ptr %0, i64 %22
  %26 = getelementptr inbounds i8, ptr %25, i64 4
  %27 = load <4 x i8>, ptr %25, align 1, !tbaa !7
  %28 = load <4 x i8>, ptr %26, align 1, !tbaa !7
  %29 = sext <4 x i8> %27 to <4 x i32>
  %30 = sext <4 x i8> %28 to <4 x i32>
  %31 = getelementptr i8, ptr %20, i64 %22
  %32 = getelementptr i8, ptr %31, i64 4
  %33 = load <4 x i8>, ptr %31, align 1, !tbaa !7
  %34 = load <4 x i8>, ptr %32, align 1, !tbaa !7
  %35 = sext <4 x i8> %33 to <4 x i32>
  %36 = sext <4 x i8> %34 to <4 x i32>
  %37 = mul nsw <4 x i32> %35, %29
  %38 = mul nsw <4 x i32> %36, %30
  %39 = add <4 x i32> %37, %23
  %40 = add <4 x i32> %38, %24
  %41 = add nuw i64 %22, 8
  %42 = icmp eq i64 %41, %12
  br i1 %42, label %43, label %21, !llvm.loop !40

43:                                               ; preds = %21
  %44 = add <4 x i32> %40, %39
  %45 = tail call i32 @llvm.vector.reduce.add.v4i32(<4 x i32> %44)
  br i1 %13, label %50, label %46

46:                                               ; preds = %16, %43
  %47 = phi i64 [ 0, %16 ], [ %12, %43 ]
  %48 = phi i32 [ 0, %16 ], [ %45, %43 ]
  br label %55

49:                                               ; preds = %50, %5
  ret void

50:                                               ; preds = %55, %43, %14
  %51 = phi i32 [ 0, %14 ], [ %45, %43 ], [ %65, %55 ]
  %52 = getelementptr inbounds i32, ptr %2, i64 %15
  store i32 %51, ptr %52, align 4, !tbaa !12
  %53 = add nuw nsw i64 %15, 1
  %54 = icmp eq i64 %53, %9
  br i1 %54, label %49, label %14, !llvm.loop !41

55:                                               ; preds = %46, %55
  %56 = phi i64 [ %66, %55 ], [ %47, %46 ]
  %57 = phi i32 [ %65, %55 ], [ %48, %46 ]
  %58 = getelementptr inbounds i8, ptr %0, i64 %56
  %59 = load i8, ptr %58, align 1, !tbaa !7
  %60 = sext i8 %59 to i32
  %61 = getelementptr i8, ptr %20, i64 %56
  %62 = load i8, ptr %61, align 1, !tbaa !7
  %63 = sext i8 %62 to i32
  %64 = mul nsw i32 %63, %60
  %65 = add nsw i32 %64, %57
  %66 = add nuw nsw i64 %56, 1
  %67 = icmp eq i64 %66, %10
  br i1 %67, label %50, label %55, !llvm.loop !42
}

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define dso_local void @batched_matvec_int8(ptr nocapture noundef readonly %0, ptr nocapture noundef readonly %1, ptr nocapture noundef writeonly %2, i32 noundef %3, i32 noundef %4, i32 noundef %5) #0 {
  %7 = icmp sgt i32 %3, 0
  br i1 %7, label %8, label %19

8:                                                ; preds = %6
  %9 = icmp sgt i32 %5, 0
  %10 = icmp sgt i32 %4, 0
  %11 = zext nneg i32 %5 to i64
  %12 = zext i32 %4 to i64
  %13 = sext i32 %4 to i64
  %14 = sext i32 %5 to i64
  %15 = zext nneg i32 %3 to i64
  %16 = icmp ult i32 %4, 8
  %17 = and i64 %12, 2147483640
  %18 = icmp eq i64 %17, %12
  br label %20

19:                                               ; preds = %79, %6
  ret void

20:                                               ; preds = %8, %79
  %21 = phi i64 [ 0, %8 ], [ %80, %79 ]
  %22 = mul nsw i64 %21, %13
  %23 = getelementptr inbounds i8, ptr %0, i64 %22
  %24 = mul nsw i64 %21, %14
  %25 = getelementptr inbounds i32, ptr %2, i64 %24
  br i1 %9, label %26, label %79

26:                                               ; preds = %20, %61
  %27 = phi i64 [ %64, %61 ], [ 0, %20 ]
  br i1 %10, label %28, label %61

28:                                               ; preds = %26
  %29 = trunc i64 %27 to i32
  %30 = mul i32 %29, %4
  %31 = zext i32 %30 to i64
  %32 = getelementptr i8, ptr %1, i64 %31
  br i1 %16, label %58, label %33

33:                                               ; preds = %28, %33
  %34 = phi i64 [ %53, %33 ], [ 0, %28 ]
  %35 = phi <4 x i32> [ %51, %33 ], [ zeroinitializer, %28 ]
  %36 = phi <4 x i32> [ %52, %33 ], [ zeroinitializer, %28 ]
  %37 = getelementptr inbounds i8, ptr %23, i64 %34
  %38 = getelementptr inbounds i8, ptr %37, i64 4
  %39 = load <4 x i8>, ptr %37, align 1, !tbaa !7
  %40 = load <4 x i8>, ptr %38, align 1, !tbaa !7
  %41 = sext <4 x i8> %39 to <4 x i32>
  %42 = sext <4 x i8> %40 to <4 x i32>
  %43 = getelementptr i8, ptr %32, i64 %34
  %44 = getelementptr i8, ptr %43, i64 4
  %45 = load <4 x i8>, ptr %43, align 1, !tbaa !7
  %46 = load <4 x i8>, ptr %44, align 1, !tbaa !7
  %47 = sext <4 x i8> %45 to <4 x i32>
  %48 = sext <4 x i8> %46 to <4 x i32>
  %49 = mul nsw <4 x i32> %47, %41
  %50 = mul nsw <4 x i32> %48, %42
  %51 = add <4 x i32> %49, %35
  %52 = add <4 x i32> %50, %36
  %53 = add nuw i64 %34, 8
  %54 = icmp eq i64 %53, %17
  br i1 %54, label %55, label %33, !llvm.loop !43

55:                                               ; preds = %33
  %56 = add <4 x i32> %52, %51
  %57 = tail call i32 @llvm.vector.reduce.add.v4i32(<4 x i32> %56)
  br i1 %18, label %61, label %58

58:                                               ; preds = %28, %55
  %59 = phi i64 [ 0, %28 ], [ %17, %55 ]
  %60 = phi i32 [ 0, %28 ], [ %57, %55 ]
  br label %66

61:                                               ; preds = %66, %55, %26
  %62 = phi i32 [ 0, %26 ], [ %57, %55 ], [ %76, %66 ]
  %63 = getelementptr inbounds i32, ptr %25, i64 %27
  store i32 %62, ptr %63, align 4, !tbaa !12
  %64 = add nuw nsw i64 %27, 1
  %65 = icmp eq i64 %64, %11
  br i1 %65, label %79, label %26, !llvm.loop !41

66:                                               ; preds = %58, %66
  %67 = phi i64 [ %77, %66 ], [ %59, %58 ]
  %68 = phi i32 [ %76, %66 ], [ %60, %58 ]
  %69 = getelementptr inbounds i8, ptr %23, i64 %67
  %70 = load i8, ptr %69, align 1, !tbaa !7
  %71 = sext i8 %70 to i32
  %72 = getelementptr i8, ptr %32, i64 %67
  %73 = load i8, ptr %72, align 1, !tbaa !7
  %74 = sext i8 %73 to i32
  %75 = mul nsw i32 %74, %71
  %76 = add nsw i32 %75, %68
  %77 = add nuw nsw i64 %67, 1
  %78 = icmp eq i64 %77, %12
  br i1 %78, label %61, label %66, !llvm.loop !44

79:                                               ; preds = %61, %20
  %80 = add nuw nsw i64 %21, 1
  %81 = icmp eq i64 %80, %15
  br i1 %81, label %19, label %20, !llvm.loop !45
}

; Function Attrs: nounwind uwtable
define dso_local void @benchmark_gemm(i32 noundef %0, i32 noundef %1, i32 noundef %2) local_unnamed_addr #1 {
  %4 = mul nsw i32 %2, %0
  %5 = sext i32 %4 to i64
  %6 = tail call noalias ptr @malloc(i64 noundef %5) #8
  %7 = mul nsw i32 %2, %1
  %8 = sext i32 %7 to i64
  %9 = tail call noalias ptr @malloc(i64 noundef %8) #8
  %10 = mul nsw i32 %1, %0
  %11 = sext i32 %10 to i64
  %12 = tail call noalias ptr @calloc(i64 noundef %11, i64 noundef 4) #9
  %13 = icmp sgt i32 %4, 0
  br i1 %13, label %14, label %57

14:                                               ; preds = %3
  %15 = zext nneg i32 %4 to i64
  %16 = icmp ult i32 %4, 8
  br i1 %16, label %55, label %17

17:                                               ; preds = %14
  %18 = icmp ult i32 %4, 16
  br i1 %18, label %36, label %19

19:                                               ; preds = %17
  %20 = and i64 %15, 2147483632
  br label %21

21:                                               ; preds = %21, %19
  %22 = phi i64 [ 0, %19 ], [ %28, %21 ]
  %23 = phi <16 x i32> [ <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>, %19 ], [ %29, %21 ]
  %24 = urem <16 x i32> %23, <i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127>
  %25 = trunc <16 x i32> %24 to <16 x i8>
  %26 = add nsw <16 x i8> %25, <i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64>
  %27 = getelementptr inbounds i8, ptr %6, i64 %22
  store <16 x i8> %26, ptr %27, align 1, !tbaa !7
  %28 = add nuw i64 %22, 16
  %29 = add <16 x i32> %23, <i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16>
  %30 = icmp eq i64 %28, %20
  br i1 %30, label %31, label %21, !llvm.loop !46

31:                                               ; preds = %21
  %32 = icmp eq i64 %20, %15
  br i1 %32, label %57, label %33

33:                                               ; preds = %31
  %34 = and i64 %15, 8
  %35 = icmp eq i64 %34, 0
  br i1 %35, label %55, label %36

36:                                               ; preds = %17, %33
  %37 = phi i64 [ %20, %33 ], [ 0, %17 ]
  %38 = and i64 %15, 2147483640
  %39 = trunc i64 %37 to i32
  %40 = insertelement <8 x i32> poison, i32 %39, i64 0
  %41 = shufflevector <8 x i32> %40, <8 x i32> poison, <8 x i32> zeroinitializer
  %42 = or disjoint <8 x i32> %41, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  br label %43

43:                                               ; preds = %43, %36
  %44 = phi i64 [ %37, %36 ], [ %50, %43 ]
  %45 = phi <8 x i32> [ %42, %36 ], [ %51, %43 ]
  %46 = urem <8 x i32> %45, <i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127>
  %47 = trunc <8 x i32> %46 to <8 x i8>
  %48 = add nsw <8 x i8> %47, <i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64>
  %49 = getelementptr inbounds i8, ptr %6, i64 %44
  store <8 x i8> %48, ptr %49, align 1, !tbaa !7
  %50 = add nuw i64 %44, 8
  %51 = add <8 x i32> %45, <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>
  %52 = icmp eq i64 %50, %38
  br i1 %52, label %53, label %43, !llvm.loop !47

53:                                               ; preds = %43
  %54 = icmp eq i64 %38, %15
  br i1 %54, label %57, label %55

55:                                               ; preds = %14, %33, %53
  %56 = phi i64 [ 0, %14 ], [ %20, %33 ], [ %38, %53 ]
  br label %102

57:                                               ; preds = %102, %31, %53, %3
  %58 = icmp sgt i32 %7, 0
  br i1 %58, label %59, label %111

59:                                               ; preds = %57
  %60 = zext nneg i32 %7 to i64
  %61 = icmp ult i32 %7, 8
  br i1 %61, label %100, label %62

62:                                               ; preds = %59
  %63 = icmp ult i32 %7, 16
  br i1 %63, label %81, label %64

64:                                               ; preds = %62
  %65 = and i64 %60, 2147483632
  br label %66

66:                                               ; preds = %66, %64
  %67 = phi i64 [ 0, %64 ], [ %73, %66 ]
  %68 = phi <16 x i32> [ <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>, %64 ], [ %74, %66 ]
  %69 = urem <16 x i32> %68, <i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127>
  %70 = trunc <16 x i32> %69 to <16 x i8>
  %71 = add nsw <16 x i8> %70, <i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64>
  %72 = getelementptr inbounds i8, ptr %9, i64 %67
  store <16 x i8> %71, ptr %72, align 1, !tbaa !7
  %73 = add nuw i64 %67, 16
  %74 = add <16 x i32> %68, <i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16>
  %75 = icmp eq i64 %73, %65
  br i1 %75, label %76, label %66, !llvm.loop !48

76:                                               ; preds = %66
  %77 = icmp eq i64 %65, %60
  br i1 %77, label %111, label %78

78:                                               ; preds = %76
  %79 = and i64 %60, 8
  %80 = icmp eq i64 %79, 0
  br i1 %80, label %100, label %81

81:                                               ; preds = %62, %78
  %82 = phi i64 [ %65, %78 ], [ 0, %62 ]
  %83 = and i64 %60, 2147483640
  %84 = trunc i64 %82 to i32
  %85 = insertelement <8 x i32> poison, i32 %84, i64 0
  %86 = shufflevector <8 x i32> %85, <8 x i32> poison, <8 x i32> zeroinitializer
  %87 = or disjoint <8 x i32> %86, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  br label %88

88:                                               ; preds = %88, %81
  %89 = phi i64 [ %82, %81 ], [ %95, %88 ]
  %90 = phi <8 x i32> [ %87, %81 ], [ %96, %88 ]
  %91 = urem <8 x i32> %90, <i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127>
  %92 = trunc <8 x i32> %91 to <8 x i8>
  %93 = add nsw <8 x i8> %92, <i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64>
  %94 = getelementptr inbounds i8, ptr %9, i64 %89
  store <8 x i8> %93, ptr %94, align 1, !tbaa !7
  %95 = add nuw i64 %89, 8
  %96 = add <8 x i32> %90, <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>
  %97 = icmp eq i64 %95, %83
  br i1 %97, label %98, label %88, !llvm.loop !49

98:                                               ; preds = %88
  %99 = icmp eq i64 %83, %60
  br i1 %99, label %111, label %100

100:                                              ; preds = %59, %78, %98
  %101 = phi i64 [ 0, %59 ], [ %65, %78 ], [ %83, %98 ]
  br label %221

102:                                              ; preds = %55, %102
  %103 = phi i64 [ %109, %102 ], [ %56, %55 ]
  %104 = trunc i64 %103 to i32
  %105 = urem i32 %104, 127
  %106 = trunc i32 %105 to i8
  %107 = add nsw i8 %106, -64
  %108 = getelementptr inbounds i8, ptr %6, i64 %103
  store i8 %107, ptr %108, align 1, !tbaa !7
  %109 = add nuw nsw i64 %103, 1
  %110 = icmp eq i64 %109, %15
  br i1 %110, label %57, label %102, !llvm.loop !50

111:                                              ; preds = %221, %76, %98, %57
  %112 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str, i32 noundef %0, i32 noundef %1, i32 noundef %2)
  %113 = icmp sgt i32 %0, 0
  br i1 %113, label %114, label %214

114:                                              ; preds = %111
  %115 = icmp sgt i32 %1, 0
  %116 = icmp sgt i32 %2, 0
  %117 = sext i32 %1 to i64
  %118 = zext nneg i32 %0 to i64
  %119 = zext nneg i32 %1 to i64
  %120 = zext i32 %2 to i64
  %121 = and i64 %120, 3
  %122 = icmp ult i32 %2, 4
  %123 = and i64 %120, 2147483644
  %124 = icmp eq i64 %121, 0
  br label %125

125:                                              ; preds = %138, %114
  %126 = phi i64 [ 0, %114 ], [ %139, %138 ]
  br i1 %115, label %127, label %138

127:                                              ; preds = %125
  %128 = mul nsw i64 %126, %117
  %129 = trunc i64 %126 to i32
  %130 = mul i32 %129, %2
  %131 = zext i32 %130 to i64
  %132 = getelementptr i8, ptr %6, i64 %131
  %133 = getelementptr i32, ptr %12, i64 %128
  br label %134

134:                                              ; preds = %161, %127
  %135 = phi i64 [ 0, %127 ], [ %164, %161 ]
  br i1 %116, label %136, label %161

136:                                              ; preds = %134
  %137 = getelementptr i8, ptr %9, i64 %135
  br i1 %122, label %141, label %166

138:                                              ; preds = %161, %125
  %139 = add nuw nsw i64 %126, 1
  %140 = icmp eq i64 %139, %118
  br i1 %140, label %212, label %125, !llvm.loop !5

141:                                              ; preds = %166, %136
  %142 = phi i32 [ undef, %136 ], [ %208, %166 ]
  %143 = phi i64 [ 0, %136 ], [ %209, %166 ]
  %144 = phi i32 [ 0, %136 ], [ %208, %166 ]
  br i1 %124, label %161, label %145

145:                                              ; preds = %141, %145
  %146 = phi i64 [ %158, %145 ], [ %143, %141 ]
  %147 = phi i32 [ %157, %145 ], [ %144, %141 ]
  %148 = phi i64 [ %159, %145 ], [ 0, %141 ]
  %149 = getelementptr i8, ptr %132, i64 %146
  %150 = load i8, ptr %149, align 1, !tbaa !7
  %151 = sext i8 %150 to i32
  %152 = mul nsw i64 %146, %117
  %153 = getelementptr i8, ptr %137, i64 %152
  %154 = load i8, ptr %153, align 1, !tbaa !7
  %155 = sext i8 %154 to i32
  %156 = mul nsw i32 %155, %151
  %157 = add nsw i32 %156, %147
  %158 = add nuw nsw i64 %146, 1
  %159 = add i64 %148, 1
  %160 = icmp eq i64 %159, %121
  br i1 %160, label %161, label %145, !llvm.loop !51

161:                                              ; preds = %141, %145, %134
  %162 = phi i32 [ 0, %134 ], [ %142, %141 ], [ %157, %145 ]
  %163 = getelementptr i32, ptr %133, i64 %135
  store i32 %162, ptr %163, align 4, !tbaa !12
  %164 = add nuw nsw i64 %135, 1
  %165 = icmp eq i64 %164, %119
  br i1 %165, label %138, label %134, !llvm.loop !14

166:                                              ; preds = %136, %166
  %167 = phi i64 [ %209, %166 ], [ 0, %136 ]
  %168 = phi i32 [ %208, %166 ], [ 0, %136 ]
  %169 = phi i64 [ %210, %166 ], [ 0, %136 ]
  %170 = getelementptr i8, ptr %132, i64 %167
  %171 = load i8, ptr %170, align 1, !tbaa !7
  %172 = sext i8 %171 to i32
  %173 = mul nsw i64 %167, %117
  %174 = getelementptr i8, ptr %137, i64 %173
  %175 = load i8, ptr %174, align 1, !tbaa !7
  %176 = sext i8 %175 to i32
  %177 = mul nsw i32 %176, %172
  %178 = add nsw i32 %177, %168
  %179 = or disjoint i64 %167, 1
  %180 = getelementptr i8, ptr %132, i64 %179
  %181 = load i8, ptr %180, align 1, !tbaa !7
  %182 = sext i8 %181 to i32
  %183 = mul nsw i64 %179, %117
  %184 = getelementptr i8, ptr %137, i64 %183
  %185 = load i8, ptr %184, align 1, !tbaa !7
  %186 = sext i8 %185 to i32
  %187 = mul nsw i32 %186, %182
  %188 = add nsw i32 %187, %178
  %189 = or disjoint i64 %167, 2
  %190 = getelementptr i8, ptr %132, i64 %189
  %191 = load i8, ptr %190, align 1, !tbaa !7
  %192 = sext i8 %191 to i32
  %193 = mul nsw i64 %189, %117
  %194 = getelementptr i8, ptr %137, i64 %193
  %195 = load i8, ptr %194, align 1, !tbaa !7
  %196 = sext i8 %195 to i32
  %197 = mul nsw i32 %196, %192
  %198 = add nsw i32 %197, %188
  %199 = or disjoint i64 %167, 3
  %200 = getelementptr i8, ptr %132, i64 %199
  %201 = load i8, ptr %200, align 1, !tbaa !7
  %202 = sext i8 %201 to i32
  %203 = mul nsw i64 %199, %117
  %204 = getelementptr i8, ptr %137, i64 %203
  %205 = load i8, ptr %204, align 1, !tbaa !7
  %206 = sext i8 %205 to i32
  %207 = mul nsw i32 %206, %202
  %208 = add nsw i32 %207, %198
  %209 = add nuw nsw i64 %167, 4
  %210 = add i64 %169, 4
  %211 = icmp eq i64 %210, %123
  br i1 %211, label %141, label %166, !llvm.loop !15

212:                                              ; preds = %138
  %213 = load i32, ptr %12, align 4, !tbaa !12
  br label %214

214:                                              ; preds = %212, %111
  %215 = phi i32 [ %213, %212 ], [ 0, %111 ]
  %216 = add nsw i32 %10, -1
  %217 = sext i32 %216 to i64
  %218 = getelementptr inbounds i32, ptr %12, i64 %217
  %219 = load i32, ptr %218, align 4, !tbaa !12
  %220 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.1, i32 noundef %215, i32 noundef %216, i32 noundef %219)
  tail call void @free(ptr noundef %6) #10
  tail call void @free(ptr noundef %9) #10
  tail call void @free(ptr noundef %12) #10
  ret void

221:                                              ; preds = %100, %221
  %222 = phi i64 [ %228, %221 ], [ %101, %100 ]
  %223 = trunc i64 %222 to i32
  %224 = urem i32 %223, 127
  %225 = trunc i32 %224 to i8
  %226 = add nsw i8 %225, -64
  %227 = getelementptr inbounds i8, ptr %9, i64 %222
  store i8 %226, ptr %227, align 1, !tbaa !7
  %228 = add nuw nsw i64 %222, 1
  %229 = icmp eq i64 %228, %60
  br i1 %229, label %111, label %221, !llvm.loop !52
}

; Function Attrs: mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite)
declare noalias noundef ptr @malloc(i64 noundef) local_unnamed_addr #2

; Function Attrs: mustprogress nofree nounwind willreturn allockind("alloc,zeroed") allocsize(0,1) memory(inaccessiblemem: readwrite)
declare noalias noundef ptr @calloc(i64 noundef, i64 noundef) local_unnamed_addr #3

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #4

; Function Attrs: mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite)
declare void @free(ptr allocptr nocapture noundef) local_unnamed_addr #5

; Function Attrs: nounwind uwtable
define dso_local void @benchmark_conv(i32 noundef %0, i32 noundef %1, i32 noundef %2, i32 noundef %3) local_unnamed_addr #1 {
  %5 = mul nsw i32 %1, %0
  %6 = sext i32 %5 to i64
  %7 = tail call noalias ptr @malloc(i64 noundef %6) #8
  %8 = mul nsw i32 %3, %2
  %9 = sext i32 %8 to i64
  %10 = tail call noalias ptr @malloc(i64 noundef %9) #8
  %11 = add i32 %0, 1
  %12 = sub i32 %11, %2
  %13 = sub nsw i32 %1, %3
  %14 = add nsw i32 %13, 1
  %15 = mul nsw i32 %14, %12
  %16 = sext i32 %15 to i64
  %17 = tail call noalias ptr @calloc(i64 noundef %16, i64 noundef 4) #9
  %18 = icmp sgt i32 %5, 0
  br i1 %18, label %19, label %62

19:                                               ; preds = %4
  %20 = zext nneg i32 %5 to i64
  %21 = icmp ult i32 %5, 8
  br i1 %21, label %60, label %22

22:                                               ; preds = %19
  %23 = icmp ult i32 %5, 16
  br i1 %23, label %41, label %24

24:                                               ; preds = %22
  %25 = and i64 %20, 2147483632
  br label %26

26:                                               ; preds = %26, %24
  %27 = phi i64 [ 0, %24 ], [ %33, %26 ]
  %28 = phi <16 x i32> [ <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>, %24 ], [ %34, %26 ]
  %29 = urem <16 x i32> %28, <i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127>
  %30 = trunc <16 x i32> %29 to <16 x i8>
  %31 = add nsw <16 x i8> %30, <i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64>
  %32 = getelementptr inbounds i8, ptr %7, i64 %27
  store <16 x i8> %31, ptr %32, align 1, !tbaa !7
  %33 = add nuw i64 %27, 16
  %34 = add <16 x i32> %28, <i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16>
  %35 = icmp eq i64 %33, %25
  br i1 %35, label %36, label %26, !llvm.loop !53

36:                                               ; preds = %26
  %37 = icmp eq i64 %25, %20
  br i1 %37, label %62, label %38

38:                                               ; preds = %36
  %39 = and i64 %20, 8
  %40 = icmp eq i64 %39, 0
  br i1 %40, label %60, label %41

41:                                               ; preds = %22, %38
  %42 = phi i64 [ %25, %38 ], [ 0, %22 ]
  %43 = and i64 %20, 2147483640
  %44 = trunc i64 %42 to i32
  %45 = insertelement <8 x i32> poison, i32 %44, i64 0
  %46 = shufflevector <8 x i32> %45, <8 x i32> poison, <8 x i32> zeroinitializer
  %47 = or disjoint <8 x i32> %46, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  br label %48

48:                                               ; preds = %48, %41
  %49 = phi i64 [ %42, %41 ], [ %55, %48 ]
  %50 = phi <8 x i32> [ %47, %41 ], [ %56, %48 ]
  %51 = urem <8 x i32> %50, <i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127, i32 127>
  %52 = trunc <8 x i32> %51 to <8 x i8>
  %53 = add nsw <8 x i8> %52, <i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64, i8 -64>
  %54 = getelementptr inbounds i8, ptr %7, i64 %49
  store <8 x i8> %53, ptr %54, align 1, !tbaa !7
  %55 = add nuw i64 %49, 8
  %56 = add <8 x i32> %50, <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>
  %57 = icmp eq i64 %55, %43
  br i1 %57, label %58, label %48, !llvm.loop !54

58:                                               ; preds = %48
  %59 = icmp eq i64 %43, %20
  br i1 %59, label %62, label %60

60:                                               ; preds = %19, %38, %58
  %61 = phi i64 [ 0, %19 ], [ %25, %38 ], [ %43, %58 ]
  br label %107

62:                                               ; preds = %107, %36, %58, %4
  %63 = icmp sgt i32 %8, 0
  br i1 %63, label %64, label %116

64:                                               ; preds = %62
  %65 = zext nneg i32 %8 to i64
  %66 = icmp ult i32 %8, 8
  br i1 %66, label %105, label %67

67:                                               ; preds = %64
  %68 = icmp ult i32 %8, 16
  br i1 %68, label %86, label %69

69:                                               ; preds = %67
  %70 = and i64 %65, 2147483632
  br label %71

71:                                               ; preds = %71, %69
  %72 = phi i64 [ 0, %69 ], [ %78, %71 ]
  %73 = phi <16 x i32> [ <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9, i32 10, i32 11, i32 12, i32 13, i32 14, i32 15>, %69 ], [ %79, %71 ]
  %74 = urem <16 x i32> %73, <i32 9, i32 9, i32 9, i32 9, i32 9, i32 9, i32 9, i32 9, i32 9, i32 9, i32 9, i32 9, i32 9, i32 9, i32 9, i32 9>
  %75 = trunc <16 x i32> %74 to <16 x i8>
  %76 = add nsw <16 x i8> %75, <i8 -4, i8 -4, i8 -4, i8 -4, i8 -4, i8 -4, i8 -4, i8 -4, i8 -4, i8 -4, i8 -4, i8 -4, i8 -4, i8 -4, i8 -4, i8 -4>
  %77 = getelementptr inbounds i8, ptr %10, i64 %72
  store <16 x i8> %76, ptr %77, align 1, !tbaa !7
  %78 = add nuw i64 %72, 16
  %79 = add <16 x i32> %73, <i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16, i32 16>
  %80 = icmp eq i64 %78, %70
  br i1 %80, label %81, label %71, !llvm.loop !55

81:                                               ; preds = %71
  %82 = icmp eq i64 %70, %65
  br i1 %82, label %116, label %83

83:                                               ; preds = %81
  %84 = and i64 %65, 8
  %85 = icmp eq i64 %84, 0
  br i1 %85, label %105, label %86

86:                                               ; preds = %67, %83
  %87 = phi i64 [ %70, %83 ], [ 0, %67 ]
  %88 = and i64 %65, 2147483640
  %89 = trunc i64 %87 to i32
  %90 = insertelement <8 x i32> poison, i32 %89, i64 0
  %91 = shufflevector <8 x i32> %90, <8 x i32> poison, <8 x i32> zeroinitializer
  %92 = or disjoint <8 x i32> %91, <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  br label %93

93:                                               ; preds = %93, %86
  %94 = phi i64 [ %87, %86 ], [ %100, %93 ]
  %95 = phi <8 x i32> [ %92, %86 ], [ %101, %93 ]
  %96 = urem <8 x i32> %95, <i32 9, i32 9, i32 9, i32 9, i32 9, i32 9, i32 9, i32 9>
  %97 = trunc <8 x i32> %96 to <8 x i8>
  %98 = add nsw <8 x i8> %97, <i8 -4, i8 -4, i8 -4, i8 -4, i8 -4, i8 -4, i8 -4, i8 -4>
  %99 = getelementptr inbounds i8, ptr %10, i64 %94
  store <8 x i8> %98, ptr %99, align 1, !tbaa !7
  %100 = add nuw i64 %94, 8
  %101 = add <8 x i32> %95, <i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8, i32 8>
  %102 = icmp eq i64 %100, %88
  br i1 %102, label %103, label %93, !llvm.loop !56

103:                                              ; preds = %93
  %104 = icmp eq i64 %88, %65
  br i1 %104, label %116, label %105

105:                                              ; preds = %64, %83, %103
  %106 = phi i64 [ 0, %64 ], [ %70, %83 ], [ %88, %103 ]
  br label %264

107:                                              ; preds = %60, %107
  %108 = phi i64 [ %114, %107 ], [ %61, %60 ]
  %109 = trunc i64 %108 to i32
  %110 = urem i32 %109, 127
  %111 = trunc i32 %110 to i8
  %112 = add nsw i8 %111, -64
  %113 = getelementptr inbounds i8, ptr %7, i64 %108
  store i8 %112, ptr %113, align 1, !tbaa !7
  %114 = add nuw nsw i64 %108, 1
  %115 = icmp eq i64 %114, %20
  br i1 %115, label %62, label %107, !llvm.loop !57

116:                                              ; preds = %264, %81, %103, %62
  %117 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.2, i32 noundef %0, i32 noundef %1, i32 noundef %2, i32 noundef %3)
  %118 = icmp slt i32 %0, %2
  br i1 %118, label %261, label %119

119:                                              ; preds = %116
  %120 = icmp slt i32 %13, 0
  %121 = icmp sgt i32 %2, 0
  %122 = icmp sgt i32 %3, 0
  %123 = sext i32 %3 to i64
  %124 = zext i32 %12 to i64
  %125 = zext i32 %14 to i64
  %126 = zext nneg i32 %2 to i64
  %127 = zext i32 %3 to i64
  %128 = add nsw i64 %127, -1
  %129 = zext i32 %1 to i64
  %130 = icmp ult i32 %3, 8
  %131 = trunc i64 %128 to i32
  %132 = icmp ugt i64 %128, 4294967295
  %133 = and i64 %127, 2147483640
  %134 = icmp eq i64 %133, %127
  %135 = and i64 %127, 1
  %136 = icmp eq i64 %135, 0
  %137 = add nsw i64 %127, -1
  br label %138

138:                                              ; preds = %151, %119
  %139 = phi i64 [ 0, %119 ], [ %152, %151 ]
  %140 = mul i64 %139, %129
  br i1 %120, label %151, label %141

141:                                              ; preds = %138
  %142 = trunc i64 %139 to i32
  %143 = mul i32 %14, %142
  %144 = zext i32 %143 to i64
  %145 = getelementptr i32, ptr %17, i64 %144
  br label %146

146:                                              ; preds = %222, %141
  %147 = phi i64 [ 0, %141 ], [ %225, %222 ]
  %148 = add i64 %140, %147
  br i1 %121, label %149, label %222

149:                                              ; preds = %146
  %150 = trunc i64 %147 to i32
  br label %154

151:                                              ; preds = %222, %138
  %152 = add nuw nsw i64 %139, 1
  %153 = icmp eq i64 %152, %124
  br i1 %153, label %259, label %138, !llvm.loop !25

154:                                              ; preds = %227, %149
  %155 = phi i64 [ 0, %149 ], [ %229, %227 ]
  %156 = phi i32 [ 0, %149 ], [ %228, %227 ]
  %157 = mul i64 %155, %129
  %158 = add i64 %148, %157
  %159 = trunc i64 %158 to i32
  br i1 %122, label %160, label %227

160:                                              ; preds = %154
  %161 = add nuw nsw i64 %155, %139
  %162 = trunc i64 %161 to i32
  %163 = mul i32 %162, %1
  %164 = add i32 %163, %150
  %165 = mul nsw i64 %155, %123
  %166 = getelementptr i8, ptr %10, i64 %165
  br i1 %130, label %201, label %167

167:                                              ; preds = %160
  %168 = add i32 %159, %131
  %169 = icmp slt i32 %168, %159
  %170 = or i1 %169, %132
  br i1 %170, label %201, label %171

171:                                              ; preds = %167
  %172 = insertelement <4 x i32> <i32 poison, i32 0, i32 0, i32 0>, i32 %156, i64 0
  br label %173

173:                                              ; preds = %173, %171
  %174 = phi i64 [ 0, %171 ], [ %196, %173 ]
  %175 = phi <4 x i32> [ %172, %171 ], [ %194, %173 ]
  %176 = phi <4 x i32> [ zeroinitializer, %171 ], [ %195, %173 ]
  %177 = trunc i64 %174 to i32
  %178 = add i32 %164, %177
  %179 = sext i32 %178 to i64
  %180 = getelementptr inbounds i8, ptr %7, i64 %179
  %181 = getelementptr inbounds i8, ptr %180, i64 4
  %182 = load <4 x i8>, ptr %180, align 1, !tbaa !7
  %183 = load <4 x i8>, ptr %181, align 1, !tbaa !7
  %184 = sext <4 x i8> %182 to <4 x i32>
  %185 = sext <4 x i8> %183 to <4 x i32>
  %186 = getelementptr i8, ptr %166, i64 %174
  %187 = getelementptr i8, ptr %186, i64 4
  %188 = load <4 x i8>, ptr %186, align 1, !tbaa !7
  %189 = load <4 x i8>, ptr %187, align 1, !tbaa !7
  %190 = sext <4 x i8> %188 to <4 x i32>
  %191 = sext <4 x i8> %189 to <4 x i32>
  %192 = mul nsw <4 x i32> %190, %184
  %193 = mul nsw <4 x i32> %191, %185
  %194 = add <4 x i32> %192, %175
  %195 = add <4 x i32> %193, %176
  %196 = add nuw i64 %174, 8
  %197 = icmp eq i64 %196, %133
  br i1 %197, label %198, label %173, !llvm.loop !58

198:                                              ; preds = %173
  %199 = add <4 x i32> %195, %194
  %200 = tail call i32 @llvm.vector.reduce.add.v4i32(<4 x i32> %199)
  br i1 %134, label %227, label %201

201:                                              ; preds = %167, %160, %198
  %202 = phi i64 [ 0, %167 ], [ 0, %160 ], [ %133, %198 ]
  %203 = phi i32 [ %156, %167 ], [ %156, %160 ], [ %200, %198 ]
  br i1 %136, label %217, label %204

204:                                              ; preds = %201
  %205 = trunc i64 %202 to i32
  %206 = add i32 %164, %205
  %207 = sext i32 %206 to i64
  %208 = getelementptr inbounds i8, ptr %7, i64 %207
  %209 = load i8, ptr %208, align 1, !tbaa !7
  %210 = sext i8 %209 to i32
  %211 = getelementptr i8, ptr %166, i64 %202
  %212 = load i8, ptr %211, align 1, !tbaa !7
  %213 = sext i8 %212 to i32
  %214 = mul nsw i32 %213, %210
  %215 = add nsw i32 %214, %203
  %216 = or disjoint i64 %202, 1
  br label %217

217:                                              ; preds = %204, %201
  %218 = phi i32 [ undef, %201 ], [ %215, %204 ]
  %219 = phi i64 [ %202, %201 ], [ %216, %204 ]
  %220 = phi i32 [ %203, %201 ], [ %215, %204 ]
  %221 = icmp eq i64 %202, %137
  br i1 %221, label %227, label %231

222:                                              ; preds = %227, %146
  %223 = phi i32 [ 0, %146 ], [ %228, %227 ]
  %224 = getelementptr i32, ptr %145, i64 %147
  store i32 %223, ptr %224, align 4, !tbaa !12
  %225 = add nuw nsw i64 %147, 1
  %226 = icmp eq i64 %225, %125
  br i1 %226, label %151, label %146, !llvm.loop !27

227:                                              ; preds = %217, %231, %198, %154
  %228 = phi i32 [ %156, %154 ], [ %200, %198 ], [ %218, %217 ], [ %256, %231 ]
  %229 = add nuw nsw i64 %155, 1
  %230 = icmp eq i64 %229, %126
  br i1 %230, label %222, label %154, !llvm.loop !28

231:                                              ; preds = %217, %231
  %232 = phi i64 [ %257, %231 ], [ %219, %217 ]
  %233 = phi i32 [ %256, %231 ], [ %220, %217 ]
  %234 = trunc i64 %232 to i32
  %235 = add i32 %164, %234
  %236 = sext i32 %235 to i64
  %237 = getelementptr inbounds i8, ptr %7, i64 %236
  %238 = load i8, ptr %237, align 1, !tbaa !7
  %239 = sext i8 %238 to i32
  %240 = getelementptr i8, ptr %166, i64 %232
  %241 = load i8, ptr %240, align 1, !tbaa !7
  %242 = sext i8 %241 to i32
  %243 = mul nsw i32 %242, %239
  %244 = add nsw i32 %243, %233
  %245 = add nuw nsw i64 %232, 1
  %246 = trunc i64 %245 to i32
  %247 = add i32 %164, %246
  %248 = sext i32 %247 to i64
  %249 = getelementptr inbounds i8, ptr %7, i64 %248
  %250 = load i8, ptr %249, align 1, !tbaa !7
  %251 = sext i8 %250 to i32
  %252 = getelementptr i8, ptr %166, i64 %245
  %253 = load i8, ptr %252, align 1, !tbaa !7
  %254 = sext i8 %253 to i32
  %255 = mul nsw i32 %254, %251
  %256 = add nsw i32 %255, %244
  %257 = add nuw nsw i64 %232, 2
  %258 = icmp eq i64 %257, %127
  br i1 %258, label %227, label %231, !llvm.loop !59

259:                                              ; preds = %151
  %260 = load i32, ptr %17, align 4, !tbaa !12
  br label %261

261:                                              ; preds = %259, %116
  %262 = phi i32 [ %260, %259 ], [ 0, %116 ]
  %263 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.3, i32 noundef %262)
  tail call void @free(ptr noundef %7) #10
  tail call void @free(ptr noundef %10) #10
  tail call void @free(ptr noundef %17) #10
  ret void

264:                                              ; preds = %105, %264
  %265 = phi i64 [ %271, %264 ], [ %106, %105 ]
  %266 = trunc i64 %265 to i32
  %267 = urem i32 %266, 9
  %268 = trunc i32 %267 to i8
  %269 = add nsw i8 %268, -4
  %270 = getelementptr inbounds i8, ptr %10, i64 %265
  store i8 %269, ptr %270, align 1, !tbaa !7
  %271 = add nuw nsw i64 %265, 1
  %272 = icmp eq i64 %271, %65
  br i1 %272, label %116, label %264, !llvm.loop !60
}

; Function Attrs: nounwind uwtable
define dso_local noundef i32 @main() local_unnamed_addr #1 {
  %1 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.28)
  %2 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.18)
  %3 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.19)
  %4 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.20)
  tail call void @benchmark_gemm(i32 noundef 4, i32 noundef 4, i32 noundef 4)
  %5 = tail call i32 @putchar(i32 10)
  %6 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.21)
  tail call void @benchmark_gemm(i32 noundef 32, i32 noundef 32, i32 noundef 32)
  tail call void @benchmark_gemm(i32 noundef 64, i32 noundef 64, i32 noundef 64)
  %7 = tail call i32 @putchar(i32 10)
  %8 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.22)
  tail call void @benchmark_conv(i32 noundef 28, i32 noundef 28, i32 noundef 3, i32 noundef 3)
  tail call void @benchmark_conv(i32 noundef 224, i32 noundef 224, i32 noundef 7, i32 noundef 7)
  %9 = tail call i32 @putchar(i32 10)
  %10 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.23)
  tail call void @benchmark_gemm(i32 noundef 128, i32 noundef 4096, i32 noundef 4096)
  %11 = tail call i32 @putchar(i32 10)
  %12 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.28)
  %13 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.25)
  %14 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.26)
  %15 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.27)
  %16 = tail call i32 @puts(ptr nonnull dereferenceable(1) @str.28)
  ret i32 0
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.smin.i32(i32, i32) #6

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr nocapture noundef readonly) local_unnamed_addr #7

; Function Attrs: nofree nounwind
declare noundef i32 @putchar(i32 noundef) local_unnamed_addr #7

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.smin.i64(i64, i64) #6

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.smax.i64(i64, i64) #6

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.vector.reduce.add.v4i32(<4 x i32>) #6

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { mustprogress nofree nounwind willreturn allockind("alloc,uninitialized") allocsize(0) memory(inaccessiblemem: readwrite) "alloc-family"="malloc" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { mustprogress nofree nounwind willreturn allockind("alloc,zeroed") allocsize(0,1) memory(inaccessiblemem: readwrite) "alloc-family"="malloc" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nofree nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #5 = { mustprogress nounwind willreturn allockind("free") memory(argmem: readwrite, inaccessiblemem: readwrite) "alloc-family"="malloc" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #6 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #7 = { nofree nounwind }
attributes #8 = { nounwind allocsize(0) }
attributes #9 = { nounwind allocsize(0,1) }
attributes #10 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"Ubuntu clang version 18.1.3 (1ubuntu1)"}
!5 = distinct !{!5, !6}
!6 = !{!"llvm.loop.mustprogress"}
!7 = !{!8, !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C/C++ TBAA"}
!10 = distinct !{!10, !11}
!11 = !{!"llvm.loop.unroll.disable"}
!12 = !{!13, !13, i64 0}
!13 = !{!"int", !8, i64 0}
!14 = distinct !{!14, !6}
!15 = distinct !{!15, !6}
!16 = distinct !{!16, !6}
!17 = distinct !{!17, !6}
!18 = distinct !{!18, !6}
!19 = distinct !{!19, !6}
!20 = distinct !{!20, !6, !21, !22}
!21 = !{!"llvm.loop.isvectorized", i32 1}
!22 = !{!"llvm.loop.unroll.runtime.disable"}
!23 = distinct !{!23, !6}
!24 = distinct !{!24, !6, !21}
!25 = distinct !{!25, !6}
!26 = distinct !{!26, !6, !21, !22}
!27 = distinct !{!27, !6}
!28 = distinct !{!28, !6}
!29 = distinct !{!29, !6, !21}
!30 = distinct !{!30, !6}
!31 = distinct !{!31, !6}
!32 = distinct !{!32, !6, !21, !22}
!33 = distinct !{!33, !6}
!34 = distinct !{!34, !6}
!35 = distinct !{!35, !6, !21}
!36 = distinct !{!36, !6, !21, !22}
!37 = distinct !{!37, !6}
!38 = distinct !{!38, !6}
!39 = distinct !{!39, !6, !22, !21}
!40 = distinct !{!40, !6, !21, !22}
!41 = distinct !{!41, !6}
!42 = distinct !{!42, !6, !22, !21}
!43 = distinct !{!43, !6, !21, !22}
!44 = distinct !{!44, !6, !22, !21}
!45 = distinct !{!45, !6}
!46 = distinct !{!46, !6, !21, !22}
!47 = distinct !{!47, !6, !21, !22}
!48 = distinct !{!48, !6, !21, !22}
!49 = distinct !{!49, !6, !21, !22}
!50 = distinct !{!50, !6, !22, !21}
!51 = distinct !{!51, !11}
!52 = distinct !{!52, !6, !22, !21}
!53 = distinct !{!53, !6, !21, !22}
!54 = distinct !{!54, !6, !21, !22}
!55 = distinct !{!55, !6, !21, !22}
!56 = distinct !{!56, !6, !21, !22}
!57 = distinct !{!57, !6, !22, !21}
!58 = distinct !{!58, !6, !21, !22}
!59 = distinct !{!59, !6, !21}
!60 = distinct !{!60, !6, !22, !21}

; DSLLVM CPU Feature Metadata
!dsllvm.cpu.profile = !{!0}
!dsllvm.cpu.features = !{!1, !2, !3, !4, !5, !6, !7, !8, !9, !10, !11, !12, !13, !14, !15, !16, !17, !18, !19, !20, !21, !22, !23, !24, !25, !26, !27, !28, !29, !30, !31, !32, !33, !34, !35, !36, !37, !38, !39, !40, !41, !42, !43, !44, !45, !46, !47, !48, !49, !50, !51, !52, !53, !54, !55, !56, !57, !58, !59, !60, !61, !62}

!0 = !{!"mtr-mtl-dsmil"}
!1 = !{!"3dnowprefetch"}
!2 = !{!"abm"}
!3 = !{!"aes"}
!4 = !{!"arch_capabilities"}
!5 = !{!"arch_lbr"}
!6 = !{!"arch_perfmon"}
!7 = !{!"avx2"}
!8 = !{!"avx_vnni"}
!9 = !{!"bmi1"}
!10 = !{!"bmi2"}
!11 = !{!"bts"}
!12 = !{!"bus_lock_detect"}
!13 = !{!"clflush"}
!14 = !{!"clflushopt"}
!15 = !{!"clwb"}
!16 = !{!"constant_tsc"}
!17 = !{!"ept"}
!18 = !{!"ept_ad"}
!19 = !{!"erms"}
!20 = !{!"flexpriority"}
!21 = !{!"flush_l1d"}
!22 = !{!"fma"}
!23 = !{!"fsrm"}
!24 = !{!"gfni"}
!25 = !{!"hfi"}
!26 = !{!"hwp"}
!27 = !{!"hwp_epp"}
!28 = !{!"hwp_notify"}
!29 = !{!"ibpb"}
!30 = !{!"ibrs_enhanced"}
!31 = !{!"intel_pt"}
!32 = !{!"invpcid"}
!33 = !{!"md_clear"}
!34 = !{!"nonstop_tsc"}
!35 = !{!"nopl"}
!36 = !{!"nx"}
!37 = !{!"pcid"}
!38 = !{!"pclmulqdq"}
!39 = !{!"pdpe1gb"}
!40 = !{!"pebs"}
!41 = !{!"pku"}
!42 = !{!"rdrand"}
!43 = !{!"rdseed"}
!44 = !{!"rdtscp"}
!45 = !{!"rep_good"}
!46 = !{!"sha_ni"}
!47 = !{!"smap"}
!48 = !{!"smep"}
!49 = !{!"smx"}
!50 = !{!"split_lock_detect"}
!51 = !{!"ssbd"}
!52 = !{!"stibp"}
!53 = !{!"tme"}
!54 = !{!"tpr_shadow"}
!55 = !{!"tsc_deadline_timer"}
!56 = !{!"umip"}
!57 = !{!"user_shstk"}
!58 = !{!"vaes"}
!59 = !{!"vmx"}
!60 = !{!"vnmi"}
!61 = !{!"vpid"}
!62 = !{!"x2apic"}
