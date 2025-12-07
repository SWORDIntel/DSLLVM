; ModuleID = '/workspace/dsmil/llvm-passes/build/gemm_simple.c'
source_filename = "/workspace/dsmil/llvm-passes/build/gemm_simple.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable
define dso_local void @gemm_4x4(ptr nocapture noundef readonly %0, ptr nocapture noundef readonly %1, ptr nocapture noundef writeonly %2) local_unnamed_addr #0 {
  br label %4

4:                                                ; preds = %3, %14
  %5 = phi i64 [ 0, %3 ], [ %15, %14 ]
  %6 = shl nuw nsw i64 %5, 2
  %7 = shl nuw nsw i64 %5, 2
  %8 = getelementptr i8, ptr %0, i64 %6
  %9 = getelementptr i32, ptr %2, i64 %7
  br label %11

10:                                               ; preds = %14
  ret void

11:                                               ; preds = %4, %17
  %12 = phi i64 [ 0, %4 ], [ %19, %17 ]
  %13 = getelementptr i8, ptr %1, i64 %12
  br label %21

14:                                               ; preds = %17
  %15 = add nuw nsw i64 %5, 1
  %16 = icmp eq i64 %15, 4
  br i1 %16, label %10, label %4, !llvm.loop !5

17:                                               ; preds = %21
  %18 = getelementptr i32, ptr %9, i64 %12
  store i32 %32, ptr %18, align 4, !tbaa !8
  %19 = add nuw nsw i64 %12, 1
  %20 = icmp eq i64 %19, 4
  br i1 %20, label %14, label %11, !llvm.loop !12

21:                                               ; preds = %11, %21
  %22 = phi i64 [ 0, %11 ], [ %33, %21 ]
  %23 = phi i32 [ 0, %11 ], [ %32, %21 ]
  %24 = getelementptr i8, ptr %8, i64 %22
  %25 = load i8, ptr %24, align 1, !tbaa !13
  %26 = sext i8 %25 to i32
  %27 = shl nuw nsw i64 %22, 2
  %28 = getelementptr i8, ptr %13, i64 %27
  %29 = load i8, ptr %28, align 1, !tbaa !13
  %30 = sext i8 %29 to i32
  %31 = mul nsw i32 %30, %26
  %32 = add nsw i32 %31, %23
  %33 = add nuw nsw i64 %22, 1
  %34 = icmp eq i64 %33, 4
  br i1 %34, label %17, label %21, !llvm.loop !14
}

attributes #0 = { nofree norecurse nosync nounwind memory(argmem: readwrite) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"Ubuntu clang version 18.1.3 (1ubuntu1)"}
!5 = distinct !{!5, !6, !7}
!6 = !{!"llvm.loop.mustprogress"}
!7 = !{!"llvm.loop.unroll.disable"}
!8 = !{!9, !9, i64 0}
!9 = !{!"int", !10, i64 0}
!10 = !{!"omnipotent char", !11, i64 0}
!11 = !{!"Simple C/C++ TBAA"}
!12 = distinct !{!12, !6, !7}
!13 = !{!10, !10, i64 0}
!14 = distinct !{!14, !6, !7}

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
