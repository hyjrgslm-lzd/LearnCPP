; ModuleID = '/mnt/f/CPPTrain/LearnCPP/C09_Coroutines/exercises/F3_halo_diagnose/main.cpp'
source_filename = "/mnt/f/CPPTrain/LearnCPP/C09_Coroutines/exercises/F3_halo_diagnose/main.cpp"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%"struct.demo::generator" = type { %"struct.std::__n4861::coroutine_handle" }
%"struct.std::__n4861::coroutine_handle" = type { ptr }
%_Z12range_valuesi.Frame = type { ptr, ptr, %"struct.demo::generator<int>::promise_type", i32, i32, i2 }
%"struct.demo::generator<int>::promise_type" = type { i32, %"class.std::__exception_ptr::exception_ptr" }
%"class.std::__exception_ptr::exception_ptr" = type { ptr }
%"class.std::basic_string_view" = type { i64, ptr }
%"class.std::__cxx11::basic_string" = type { %"struct.std::__cxx11::basic_string<char>::_Alloc_hider", i64, %union.anon.1 }
%"struct.std::__cxx11::basic_string<char>::_Alloc_hider" = type { ptr }
%union.anon.1 = type { i64, [8 x i8] }
%"class.std::allocator" = type { i8 }

$_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_ = comdat any

@_ZL17escaped_generator = internal unnamed_addr global ptr null, align 8
@_ZL4sink = internal global i64 0, align 8
@.str = private unnamed_addr constant [6 x i8] c"local\00", align 1
@.str.1 = private unnamed_addr constant [8 x i8] c"escaped\00", align 1
@.str.2 = private unnamed_addr constant [33 x i8] c"version must be local or escaped\00", align 1
@.str.3 = private unnamed_addr constant [43 x i8] c"selected version must compute the same sum\00", align 1
@.str.4 = private unnamed_addr constant [60 x i8] c"version=%.*s,n=%d,iterations=%d,ns_per_iter=%.3f,sink=%lld\0A\00", align 1
@_ZTISt9exception = external constant ptr
@.str.5 = private unnamed_addr constant [8 x i8] c"--bench\00", align 1
@.str.6 = private unnamed_addr constant [34 x i8] c"n and iterations must be positive\00", align 1
@stderr = external local_unnamed_addr global ptr, align 8
@.str.12 = private unnamed_addr constant [26 x i8] c"starter check failed: %s\0A\00", align 1
@_ZTISt13runtime_error = external constant ptr
@.str.13 = private unnamed_addr constant [50 x i8] c"basic_string: construction from null is not valid\00", align 1
@.str.14 = private unnamed_addr constant [24 x i8] c"basic_string::_M_create\00", align 1
@str = private unnamed_addr constant [26 x i8] c"F3 starter correctness OK\00", align 1
@str.15 = private unnamed_addr constant [78 x i8] c"Run sample_f3.py with this executable for 1 warmup + 5 independent processes.\00", align 1
@str.16 = private unnamed_addr constant [69 x i8] c"Use compiler remark/IR/assembly for HALO; timing alone is not proof.\00", align 1

; Function Attrs: mustprogress uwtable
define dso_local void @_Z12range_valuesi(ptr dead_on_unwind nocapture writable writeonly sret(%"struct.demo::generator") align 8 %0, i32 noundef %1) local_unnamed_addr #0 personality ptr @__gxx_personality_v0 {
  %3 = tail call noalias noundef nonnull dereferenceable(48) ptr @_Znwm(i64 noundef 48) #21
  store ptr @_Z12range_valuesi.resume, ptr %3, align 8
  %4 = getelementptr inbounds %_Z12range_valuesi.Frame, ptr %3, i64 0, i32 1
  store ptr @_Z12range_valuesi.destroy, ptr %4, align 8
  %5 = getelementptr inbounds %_Z12range_valuesi.Frame, ptr %3, i64 0, i32 2
  %6 = getelementptr inbounds %_Z12range_valuesi.Frame, ptr %3, i64 0, i32 3
  store i32 %1, ptr %6, align 4
  store i32 %1, ptr %5, align 8, !tbaa !5
  %7 = getelementptr inbounds %_Z12range_valuesi.Frame, ptr %3, i64 0, i32 2, i32 1
  store ptr null, ptr %7, align 8, !tbaa !12
  store ptr %3, ptr %0, align 8, !tbaa.struct !13, !alias.scope !15
  %8 = getelementptr inbounds %_Z12range_valuesi.Frame, ptr %3, i64 0, i32 5
  store i2 0, ptr %8, align 1
  ret void
}

; Function Attrs: nobuiltin allocsize(0)
declare noundef nonnull ptr @_Znwm(i64 noundef) local_unnamed_addr #1

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #2

; Function Attrs: mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #2

; Function Attrs: nobuiltin nounwind
declare void @_ZdlPv(ptr noundef) local_unnamed_addr #3

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable
define dso_local noundef i64 @_Z13local_consumei(i32 noundef %0) #4 personality ptr @__gxx_personality_v0 {
  %2 = icmp slt i32 %0, 2
  br i1 %2, label %29, label %3, !llvm.loop !18

3:                                                ; preds = %1
  %4 = add nsw i32 %0, -1
  %5 = icmp ult i32 %0, 5
  br i1 %5, label %26, label %6

6:                                                ; preds = %3
  %7 = and i32 %4, -4
  %8 = or disjoint i32 %7, 1
  br label %9

9:                                                ; preds = %9, %6
  %10 = phi i32 [ 0, %6 ], [ %19, %9 ]
  %11 = phi <2 x i32> [ <i32 1, i32 2>, %6 ], [ %20, %9 ]
  %12 = phi <2 x i64> [ zeroinitializer, %6 ], [ %17, %9 ]
  %13 = phi <2 x i64> [ zeroinitializer, %6 ], [ %18, %9 ]
  %14 = add <2 x i32> %11, <i32 2, i32 2>
  %15 = zext <2 x i32> %11 to <2 x i64>
  %16 = zext <2 x i32> %14 to <2 x i64>
  %17 = add <2 x i64> %12, %15
  %18 = add <2 x i64> %13, %16
  %19 = add nuw i32 %10, 4
  %20 = add <2 x i32> %11, <i32 4, i32 4>
  %21 = icmp eq i32 %19, %7
  br i1 %21, label %22, label %9, !llvm.loop !20

22:                                               ; preds = %9
  %23 = add <2 x i64> %18, %17
  %24 = tail call i64 @llvm.vector.reduce.add.v2i64(<2 x i64> %23)
  %25 = icmp eq i32 %4, %7
  br i1 %25, label %29, label %26

26:                                               ; preds = %3, %22
  %27 = phi i32 [ 1, %3 ], [ %8, %22 ]
  %28 = phi i64 [ 0, %3 ], [ %24, %22 ]
  br label %31

29:                                               ; preds = %31, %22, %1
  %30 = phi i64 [ 0, %1 ], [ %24, %22 ], [ %35, %31 ]
  ret i64 %30

31:                                               ; preds = %26, %31
  %32 = phi i32 [ %36, %31 ], [ %27, %26 ]
  %33 = phi i64 [ %35, %31 ], [ %28, %26 ]
  %34 = zext nneg i32 %32 to i64
  %35 = add nuw nsw i64 %33, %34
  %36 = add nuw nsw i32 %32, 1
  %37 = icmp eq i32 %36, %0
  br i1 %37, label %29, label %31, !llvm.loop !23
}

declare i32 @__gxx_personality_v0(...)

; Function Attrs: mustprogress nofree norecurse nosync nounwind memory(write, argmem: none, inaccessiblemem: none) uwtable
define dso_local noundef i64 @_Z15escaped_consumei(i32 noundef %0) #5 personality ptr @__gxx_personality_v0 {
  %2 = alloca %"struct.demo::generator", align 8
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %2) #22
  store ptr %2, ptr @_ZL17escaped_generator, align 8, !tbaa !14
  %3 = icmp sgt i32 %0, 0
  br i1 %3, label %4, label %27

4:                                                ; preds = %1
  %5 = zext nneg i32 %0 to i64
  %6 = icmp ult i32 %0, 4
  br i1 %6, label %24, label %7

7:                                                ; preds = %4
  %8 = and i64 %5, 2147483644
  br label %9

9:                                                ; preds = %9, %7
  %10 = phi i64 [ 0, %7 ], [ %17, %9 ]
  %11 = phi <2 x i64> [ <i64 0, i64 1>, %7 ], [ %18, %9 ]
  %12 = phi <2 x i64> [ zeroinitializer, %7 ], [ %15, %9 ]
  %13 = phi <2 x i64> [ zeroinitializer, %7 ], [ %16, %9 ]
  %14 = add <2 x i64> %11, <i64 2, i64 2>
  %15 = add <2 x i64> %12, %11
  %16 = add <2 x i64> %13, %14
  %17 = add nuw i64 %10, 4
  %18 = add <2 x i64> %11, <i64 4, i64 4>
  %19 = icmp eq i64 %17, %8
  br i1 %19, label %20, label %9, !llvm.loop !24

20:                                               ; preds = %9
  %21 = add <2 x i64> %16, %15
  %22 = call i64 @llvm.vector.reduce.add.v2i64(<2 x i64> %21)
  %23 = icmp eq i64 %8, %5
  br i1 %23, label %27, label %24

24:                                               ; preds = %4, %20
  %25 = phi i64 [ 0, %4 ], [ %8, %20 ]
  %26 = phi i64 [ 0, %4 ], [ %22, %20 ]
  br label %29

27:                                               ; preds = %29, %20, %1
  %28 = phi i64 [ 0, %1 ], [ %22, %20 ], [ %32, %29 ]
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %2) #22
  ret i64 %28

29:                                               ; preds = %24, %29
  %30 = phi i64 [ %33, %29 ], [ %25, %24 ]
  %31 = phi i64 [ %32, %29 ], [ %26, %24 ]
  %32 = add nuw nsw i64 %31, %30
  %33 = add nuw nsw i64 %30, 1
  %34 = icmp eq i64 %33, %5
  br i1 %34, label %27, label %29, !llvm.loop !25
}

; Function Attrs: mustprogress uwtable
define dso_local noundef double @_Z8bench_nsPFliEii(ptr nocapture noundef readonly %0, i32 noundef %1, i32 noundef %2) local_unnamed_addr #0 {
  %4 = tail call i64 @_ZNSt6chrono3_V212steady_clock3nowEv() #22
  %5 = icmp sgt i32 %2, 0
  br i1 %5, label %15, label %6

6:                                                ; preds = %15, %3
  %7 = phi i64 [ 0, %3 ], [ %19, %15 ]
  %8 = tail call i64 @_ZNSt6chrono3_V212steady_clock3nowEv() #22
  %9 = load volatile i64, ptr @_ZL4sink, align 8, !tbaa !26
  %10 = add nsw i64 %9, %7
  store volatile i64 %10, ptr @_ZL4sink, align 8, !tbaa !26
  %11 = sub nsw i64 %8, %4
  %12 = sitofp i64 %11 to double
  %13 = sitofp i32 %2 to double
  %14 = fdiv double %12, %13
  ret double %14

15:                                               ; preds = %3, %15
  %16 = phi i64 [ %19, %15 ], [ 0, %3 ]
  %17 = phi i32 [ %20, %15 ], [ 0, %3 ]
  %18 = tail call noundef i64 %0(i32 noundef %1)
  %19 = add nsw i64 %18, %16
  %20 = add nuw nsw i32 %17, 1
  %21 = icmp eq i32 %20, %2
  br i1 %21, label %6, label %15, !llvm.loop !28
}

; Function Attrs: nounwind
declare i64 @_ZNSt6chrono3_V212steady_clock3nowEv() local_unnamed_addr #6

; Function Attrs: mustprogress nofree nounwind willreturn memory(argmem: read) uwtable
define dso_local noundef ptr @_Z14select_versionSt17basic_string_viewIcSt11char_traitsIcEE(i64 %0, ptr nocapture readonly %1) local_unnamed_addr #7 personality ptr @__gxx_personality_v0 {
  switch i64 %0, label %9 [
    i64 5, label %3
    i64 7, label %6
  ]

3:                                                ; preds = %2
  %4 = tail call i32 @bcmp(ptr noundef nonnull dereferenceable(5) %1, ptr noundef nonnull dereferenceable(5) @.str, i64 5)
  %5 = icmp eq i32 %4, 0
  br i1 %5, label %10, label %9

6:                                                ; preds = %2
  %7 = tail call i32 @bcmp(ptr noundef nonnull dereferenceable(7) %1, ptr noundef nonnull dereferenceable(7) @.str.1, i64 7)
  %8 = icmp eq i32 %7, 0
  br i1 %8, label %10, label %9

9:                                                ; preds = %3, %2, %6
  br label %10

10:                                               ; preds = %9, %6, %3
  %11 = phi ptr [ @_Z13local_consumei, %3 ], [ null, %9 ], [ @_Z15escaped_consumei, %6 ]
  ret ptr %11
}

; Function Attrs: mustprogress nocallback nofree nounwind willreturn memory(argmem: readwrite)
declare void @llvm.memcpy.p0.p0.i64(ptr noalias nocapture writeonly, ptr noalias nocapture readonly, i64, i1 immarg) #8

; Function Attrs: mustprogress uwtable
define dso_local noundef i32 @_Z7run_oneSt17basic_string_viewIcSt11char_traitsIcEEii(i64 %0, ptr %1, i32 noundef %2, i32 noundef %3) local_unnamed_addr #0 personality ptr @__gxx_personality_v0 {
  %5 = alloca %"class.std::basic_string_view", align 8
  %6 = alloca %"class.std::__cxx11::basic_string", align 8
  %7 = alloca %"class.std::allocator", align 1
  %8 = alloca %"class.std::basic_string_view", align 8
  %9 = alloca %"class.std::__cxx11::basic_string", align 8
  %10 = alloca %"class.std::allocator", align 1
  switch i64 %0, label %17 [
    i64 5, label %11
    i64 7, label %14
  ]

11:                                               ; preds = %4
  %12 = tail call i32 @bcmp(ptr noundef nonnull dereferenceable(5) %1, ptr noundef nonnull dereferenceable(5) @.str, i64 5)
  %13 = icmp eq i32 %12, 0
  br i1 %13, label %41, label %17

14:                                               ; preds = %4
  %15 = tail call i32 @bcmp(ptr noundef nonnull dereferenceable(7) %1, ptr noundef nonnull dereferenceable(7) @.str.1, i64 7)
  %16 = icmp eq i32 %15, 0
  br i1 %16, label %41, label %17

17:                                               ; preds = %14, %11, %4
  call void @llvm.lifetime.start.p0(i64 16, ptr nonnull %8)
  store i64 32, ptr %8, align 8
  %18 = getelementptr inbounds { i64, ptr }, ptr %8, i64 0, i32 1
  store ptr @.str.2, ptr %18, align 8
  %19 = tail call ptr @__cxa_allocate_exception(i64 16) #22
  call void @llvm.lifetime.start.p0(i64 32, ptr nonnull %9) #22
  call void @llvm.lifetime.start.p0(i64 1, ptr nonnull %10) #22
  invoke void @_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_(ptr noundef nonnull align 8 dereferenceable(32) %9, ptr noundef nonnull align 8 dereferenceable(16) %8, ptr noundef nonnull align 1 dereferenceable(1) %10)
          to label %20 unwind label %22

20:                                               ; preds = %17
  invoke void @_ZNSt13runtime_errorC1ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE(ptr noundef nonnull align 8 dereferenceable(16) %19, ptr noundef nonnull align 8 dereferenceable(32) %9)
          to label %21 unwind label %24

21:                                               ; preds = %20
  invoke void @__cxa_throw(ptr nonnull %19, ptr nonnull @_ZTISt13runtime_error, ptr nonnull @_ZNSt13runtime_errorD1Ev) #23
          to label %40 unwind label %24

22:                                               ; preds = %17
  %23 = landingpad { ptr, i32 }
          cleanup
  call void @llvm.lifetime.end.p0(i64 1, ptr nonnull %10) #22
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %9) #22
  br label %35

24:                                               ; preds = %21, %20
  %25 = phi i1 [ false, %21 ], [ true, %20 ]
  %26 = landingpad { ptr, i32 }
          cleanup
  %27 = load ptr, ptr %9, align 8, !tbaa !29
  %28 = getelementptr inbounds %"class.std::__cxx11::basic_string", ptr %9, i64 0, i32 2
  %29 = icmp eq ptr %27, %28
  br i1 %29, label %30, label %34

30:                                               ; preds = %24
  %31 = getelementptr inbounds %"class.std::__cxx11::basic_string", ptr %9, i64 0, i32 1
  %32 = load i64, ptr %31, align 8, !tbaa !32
  %33 = icmp ult i64 %32, 16
  call void @llvm.assume(i1 %33)
  call void @llvm.lifetime.end.p0(i64 1, ptr nonnull %10) #22
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %9) #22
  br i1 %25, label %35, label %38

34:                                               ; preds = %24
  call void @_ZdlPv(ptr noundef %27) #24
  call void @llvm.lifetime.end.p0(i64 1, ptr nonnull %10) #22
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %9) #22
  br i1 %25, label %35, label %38

35:                                               ; preds = %22, %30, %34, %55, %63, %67
  %36 = phi ptr [ %52, %67 ], [ %52, %63 ], [ %52, %55 ], [ %19, %34 ], [ %19, %30 ], [ %19, %22 ]
  %37 = phi { ptr, i32 } [ %59, %67 ], [ %59, %63 ], [ %56, %55 ], [ %26, %34 ], [ %26, %30 ], [ %23, %22 ]
  call void @__cxa_free_exception(ptr %36) #22
  br label %38

38:                                               ; preds = %35, %63, %67, %30, %34
  %39 = phi { ptr, i32 } [ %26, %34 ], [ %26, %30 ], [ %59, %67 ], [ %59, %63 ], [ %37, %35 ]
  resume { ptr, i32 } %39

40:                                               ; preds = %21
  unreachable

41:                                               ; preds = %11, %14
  %42 = phi ptr [ @_Z15escaped_consumei, %14 ], [ @_Z13local_consumei, %11 ]
  %43 = add nsw i32 %2, -1
  %44 = sext i32 %43 to i64
  %45 = sext i32 %2 to i64
  %46 = mul nsw i64 %44, %45
  %47 = sdiv i64 %46, 2
  %48 = tail call noundef i64 %42(i32 noundef %2), !callees !33
  %49 = icmp eq i64 %48, %47
  call void @llvm.lifetime.start.p0(i64 16, ptr nonnull %5)
  store i64 42, ptr %5, align 8
  %50 = getelementptr inbounds { i64, ptr }, ptr %5, i64 0, i32 1
  store ptr @.str.3, ptr %50, align 8
  br i1 %49, label %69, label %51

51:                                               ; preds = %41
  %52 = tail call ptr @__cxa_allocate_exception(i64 16) #22
  call void @llvm.lifetime.start.p0(i64 32, ptr nonnull %6) #22
  call void @llvm.lifetime.start.p0(i64 1, ptr nonnull %7) #22
  invoke void @_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_(ptr noundef nonnull align 8 dereferenceable(32) %6, ptr noundef nonnull align 8 dereferenceable(16) %5, ptr noundef nonnull align 1 dereferenceable(1) %7)
          to label %53 unwind label %55

53:                                               ; preds = %51
  invoke void @_ZNSt13runtime_errorC1ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE(ptr noundef nonnull align 8 dereferenceable(16) %52, ptr noundef nonnull align 8 dereferenceable(32) %6)
          to label %54 unwind label %57

54:                                               ; preds = %53
  invoke void @__cxa_throw(ptr nonnull %52, ptr nonnull @_ZTISt13runtime_error, ptr nonnull @_ZNSt13runtime_errorD1Ev) #23
          to label %68 unwind label %57

55:                                               ; preds = %51
  %56 = landingpad { ptr, i32 }
          cleanup
  call void @llvm.lifetime.end.p0(i64 1, ptr nonnull %7) #22
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %6) #22
  br label %35

57:                                               ; preds = %54, %53
  %58 = phi i1 [ false, %54 ], [ true, %53 ]
  %59 = landingpad { ptr, i32 }
          cleanup
  %60 = load ptr, ptr %6, align 8, !tbaa !29
  %61 = getelementptr inbounds %"class.std::__cxx11::basic_string", ptr %6, i64 0, i32 2
  %62 = icmp eq ptr %60, %61
  br i1 %62, label %63, label %67

63:                                               ; preds = %57
  %64 = getelementptr inbounds %"class.std::__cxx11::basic_string", ptr %6, i64 0, i32 1
  %65 = load i64, ptr %64, align 8, !tbaa !32
  %66 = icmp ult i64 %65, 16
  call void @llvm.assume(i1 %66)
  call void @llvm.lifetime.end.p0(i64 1, ptr nonnull %7) #22
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %6) #22
  br i1 %58, label %35, label %38

67:                                               ; preds = %57
  call void @_ZdlPv(ptr noundef %60) #24
  call void @llvm.lifetime.end.p0(i64 1, ptr nonnull %7) #22
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %6) #22
  br i1 %58, label %35, label %38

68:                                               ; preds = %54
  unreachable

69:                                               ; preds = %41
  call void @llvm.lifetime.end.p0(i64 16, ptr nonnull %5)
  %70 = tail call i64 @_ZNSt6chrono3_V212steady_clock3nowEv() #22
  br label %71

71:                                               ; preds = %71, %69
  %72 = phi i64 [ %75, %71 ], [ 0, %69 ]
  %73 = phi i32 [ %76, %71 ], [ 0, %69 ]
  %74 = tail call noundef i64 %42(i32 noundef %2)
  %75 = add nsw i64 %74, %72
  %76 = add nuw nsw i32 %73, 1
  %77 = icmp eq i32 %76, 1000
  br i1 %77, label %78, label %71, !llvm.loop !28

78:                                               ; preds = %71
  %79 = tail call i64 @_ZNSt6chrono3_V212steady_clock3nowEv() #22
  %80 = load volatile i64, ptr @_ZL4sink, align 8, !tbaa !26
  %81 = add nsw i64 %80, %75
  store volatile i64 %81, ptr @_ZL4sink, align 8, !tbaa !26
  %82 = tail call i64 @_ZNSt6chrono3_V212steady_clock3nowEv() #22
  %83 = icmp sgt i32 %3, 0
  br i1 %83, label %84, label %91

84:                                               ; preds = %78, %84
  %85 = phi i64 [ %88, %84 ], [ 0, %78 ]
  %86 = phi i32 [ %89, %84 ], [ 0, %78 ]
  %87 = tail call noundef i64 %42(i32 noundef %2)
  %88 = add nsw i64 %87, %85
  %89 = add nuw nsw i32 %86, 1
  %90 = icmp eq i32 %89, %3
  br i1 %90, label %91, label %84, !llvm.loop !28

91:                                               ; preds = %84, %78
  %92 = phi i64 [ 0, %78 ], [ %88, %84 ]
  %93 = tail call i64 @_ZNSt6chrono3_V212steady_clock3nowEv() #22
  %94 = load volatile i64, ptr @_ZL4sink, align 8, !tbaa !26
  %95 = add nsw i64 %94, %92
  store volatile i64 %95, ptr @_ZL4sink, align 8, !tbaa !26
  %96 = sub nsw i64 %93, %82
  %97 = sitofp i64 %96 to double
  %98 = sitofp i32 %3 to double
  %99 = fdiv double %97, %98
  %100 = trunc i64 %0 to i32
  %101 = load volatile i64, ptr @_ZL4sink, align 8, !tbaa !26
  %102 = tail call i32 (ptr, ...) @printf(ptr noundef nonnull dereferenceable(1) @.str.4, i32 noundef %100, ptr noundef %1, i32 noundef %2, i32 noundef %3, double noundef %99, i64 noundef %101)
  ret i32 0
}

; Function Attrs: nofree nounwind
declare noundef i32 @printf(ptr nocapture noundef readonly, ...) local_unnamed_addr #9

; Function Attrs: mustprogress norecurse uwtable
define dso_local noundef i32 @main(i32 noundef %0, ptr nocapture noundef readonly %1) local_unnamed_addr #10 personality ptr @__gxx_personality_v0 {
  %3 = alloca %"struct.demo::generator", align 8
  %4 = alloca %"class.std::basic_string_view", align 8
  %5 = alloca %"class.std::__cxx11::basic_string", align 8
  %6 = alloca %"class.std::allocator", align 1
  %7 = icmp eq i32 %0, 5
  br i1 %7, label %8, label %56

8:                                                ; preds = %2
  %9 = getelementptr inbounds ptr, ptr %1, i64 1
  %10 = load ptr, ptr %9, align 8, !tbaa !14
  %11 = tail call noundef i64 @strlen(ptr noundef nonnull dereferenceable(1) %10) #22
  %12 = icmp eq i64 %11, 7
  br i1 %12, label %13, label %56

13:                                               ; preds = %8
  %14 = tail call i32 @bcmp(ptr noundef nonnull dereferenceable(7) %10, ptr noundef nonnull dereferenceable(7) @.str.5, i64 7)
  %15 = icmp eq i32 %14, 0
  br i1 %15, label %16, label %56

16:                                               ; preds = %13
  %17 = getelementptr inbounds ptr, ptr %1, i64 2
  %18 = load ptr, ptr %17, align 8, !tbaa !14
  %19 = tail call noundef i64 @strlen(ptr noundef nonnull dereferenceable(1) %18) #22
  %20 = getelementptr inbounds ptr, ptr %1, i64 3
  %21 = load ptr, ptr %20, align 8, !tbaa !14
  %22 = tail call i64 @__isoc23_strtol(ptr noundef nonnull %21, ptr noundef null, i32 noundef 10) #22
  %23 = trunc i64 %22 to i32
  %24 = getelementptr inbounds ptr, ptr %1, i64 4
  %25 = load ptr, ptr %24, align 8, !tbaa !14
  %26 = tail call i64 @__isoc23_strtol(ptr noundef nonnull %25, ptr noundef null, i32 noundef 10) #22
  %27 = trunc i64 %26 to i32
  %28 = icmp sgt i32 %23, 0
  %29 = icmp sgt i32 %27, 0
  %30 = select i1 %28, i1 %29, i1 false
  call void @llvm.lifetime.start.p0(i64 16, ptr nonnull %4)
  store i64 33, ptr %4, align 8
  %31 = getelementptr inbounds { i64, ptr }, ptr %4, i64 0, i32 1
  store ptr @.str.6, ptr %31, align 8
  br i1 %30, label %52, label %32

32:                                               ; preds = %16
  %33 = tail call ptr @__cxa_allocate_exception(i64 16) #22
  call void @llvm.lifetime.start.p0(i64 32, ptr nonnull %5) #22
  call void @llvm.lifetime.start.p0(i64 1, ptr nonnull %6) #22
  invoke void @_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_(ptr noundef nonnull align 8 dereferenceable(32) %5, ptr noundef nonnull align 8 dereferenceable(16) %4, ptr noundef nonnull align 1 dereferenceable(1) %6)
          to label %34 unwind label %36

34:                                               ; preds = %32
  invoke void @_ZNSt13runtime_errorC1ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE(ptr noundef nonnull align 8 dereferenceable(16) %33, ptr noundef nonnull align 8 dereferenceable(32) %5)
          to label %35 unwind label %38

35:                                               ; preds = %34
  invoke void @__cxa_throw(ptr nonnull %33, ptr nonnull @_ZTISt13runtime_error, ptr nonnull @_ZNSt13runtime_errorD1Ev) #23
          to label %51 unwind label %38

36:                                               ; preds = %32
  %37 = landingpad { ptr, i32 }
          cleanup
          catch ptr @_ZTISt9exception
  call void @llvm.lifetime.end.p0(i64 1, ptr nonnull %6) #22
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %5) #22
  br label %49

38:                                               ; preds = %35, %34
  %39 = phi i1 [ false, %35 ], [ true, %34 ]
  %40 = landingpad { ptr, i32 }
          cleanup
          catch ptr @_ZTISt9exception
  %41 = load ptr, ptr %5, align 8, !tbaa !29
  %42 = getelementptr inbounds %"class.std::__cxx11::basic_string", ptr %5, i64 0, i32 2
  %43 = icmp eq ptr %41, %42
  br i1 %43, label %44, label %48

44:                                               ; preds = %38
  %45 = getelementptr inbounds %"class.std::__cxx11::basic_string", ptr %5, i64 0, i32 1
  %46 = load i64, ptr %45, align 8, !tbaa !32
  %47 = icmp ult i64 %46, 16
  call void @llvm.assume(i1 %47)
  call void @llvm.lifetime.end.p0(i64 1, ptr nonnull %6) #22
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %5) #22
  br i1 %39, label %49, label %60

48:                                               ; preds = %38
  call void @_ZdlPv(ptr noundef %41) #24
  call void @llvm.lifetime.end.p0(i64 1, ptr nonnull %6) #22
  call void @llvm.lifetime.end.p0(i64 32, ptr nonnull %5) #22
  br i1 %39, label %49, label %60

49:                                               ; preds = %48, %44, %36
  %50 = phi { ptr, i32 } [ %37, %36 ], [ %40, %48 ], [ %40, %44 ]
  call void @__cxa_free_exception(ptr %33) #22
  br label %60

51:                                               ; preds = %35
  unreachable

52:                                               ; preds = %16
  call void @llvm.lifetime.end.p0(i64 16, ptr nonnull %4)
  %53 = invoke noundef i32 @_Z7run_oneSt17basic_string_viewIcSt11char_traitsIcEEii(i64 %19, ptr %18, i32 noundef %23, i32 noundef %27)
          to label %74 unwind label %54

54:                                               ; preds = %52
  %55 = landingpad { ptr, i32 }
          cleanup
          catch ptr @_ZTISt9exception
  br label %60

56:                                               ; preds = %13, %8, %2
  call void @llvm.lifetime.start.p0(i64 8, ptr nonnull %3) #22
  store ptr %3, ptr @_ZL17escaped_generator, align 8, !tbaa !14
  call void @llvm.lifetime.end.p0(i64 8, ptr nonnull %3) #22
  %57 = call i32 @puts(ptr nonnull dereferenceable(1) @str)
  %58 = call i32 @puts(ptr nonnull dereferenceable(1) @str.15)
  %59 = call i32 @puts(ptr nonnull dereferenceable(1) @str.16)
  br label %74

60:                                               ; preds = %54, %49, %48, %44
  %61 = phi { ptr, i32 } [ %55, %54 ], [ %40, %48 ], [ %50, %49 ], [ %40, %44 ]
  %62 = extractvalue { ptr, i32 } %61, 1
  %63 = call i32 @llvm.eh.typeid.for(ptr nonnull @_ZTISt9exception) #22
  %64 = icmp eq i32 %62, %63
  br i1 %64, label %65, label %76

65:                                               ; preds = %60
  %66 = extractvalue { ptr, i32 } %61, 0
  %67 = call ptr @__cxa_begin_catch(ptr %66) #22
  %68 = load ptr, ptr @stderr, align 8, !tbaa !14
  %69 = load ptr, ptr %67, align 8, !tbaa !34
  %70 = getelementptr inbounds ptr, ptr %69, i64 2
  %71 = load ptr, ptr %70, align 8
  %72 = call noundef ptr %71(ptr noundef nonnull align 8 dereferenceable(8) %67) #22
  %73 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %68, ptr noundef nonnull @.str.12, ptr noundef %72) #25
  call void @__cxa_end_catch()
  br label %74

74:                                               ; preds = %52, %56, %65
  %75 = phi i32 [ 1, %65 ], [ 0, %56 ], [ 0, %52 ]
  ret i32 %75

76:                                               ; preds = %60
  resume { ptr, i32 } %61
}

; Function Attrs: nofree nosync nounwind memory(none)
declare i32 @llvm.eh.typeid.for(ptr) #11

declare ptr @__cxa_begin_catch(ptr) local_unnamed_addr

; Function Attrs: nofree nounwind
declare noundef i32 @fprintf(ptr nocapture noundef, ptr nocapture noundef readonly, ...) local_unnamed_addr #9

declare void @__cxa_end_catch() local_unnamed_addr

; Function Attrs: nounwind
declare void @_ZNSt15__exception_ptr13exception_ptr10_M_releaseEv(ptr noundef nonnull align 8 dereferenceable(8)) local_unnamed_addr #6

; Function Attrs: mustprogress nofree nounwind willreturn memory(argmem: read)
declare i64 @strlen(ptr nocapture noundef) local_unnamed_addr #12

declare ptr @__cxa_allocate_exception(i64) local_unnamed_addr

; Function Attrs: mustprogress uwtable
define linkonce_odr dso_local void @_ZNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEC2ISt17basic_string_viewIcS2_EvEERKT_RKS3_(ptr noundef nonnull align 8 dereferenceable(32) %0, ptr noundef nonnull align 8 dereferenceable(16) %1, ptr noundef nonnull align 1 dereferenceable(1) %2) unnamed_addr #0 comdat align 2 personality ptr @__gxx_personality_v0 {
  %4 = load i64, ptr %1, align 8, !tbaa.struct !36
  %5 = getelementptr inbounds i8, ptr %1, i64 8
  %6 = load ptr, ptr %5, align 8, !tbaa.struct !13
  %7 = getelementptr inbounds %"class.std::__cxx11::basic_string", ptr %0, i64 0, i32 2
  store ptr %7, ptr %0, align 8, !tbaa !37
  %8 = icmp eq ptr %6, null
  %9 = icmp ne i64 %4, 0
  %10 = and i1 %9, %8
  br i1 %10, label %11, label %12

11:                                               ; preds = %3
  tail call void @_ZSt19__throw_logic_errorPKc(ptr noundef nonnull @.str.13) #23
  unreachable

12:                                               ; preds = %3
  %13 = icmp ugt i64 %4, 15
  br i1 %13, label %14, label %23

14:                                               ; preds = %12
  %15 = icmp slt i64 %4, 0
  br i1 %15, label %16, label %17

16:                                               ; preds = %14
  tail call void @_ZSt20__throw_length_errorPKc(ptr noundef nonnull @.str.14) #23
  unreachable

17:                                               ; preds = %14
  %18 = add nuw i64 %4, 1
  %19 = icmp slt i64 %18, 0
  br i1 %19, label %20, label %21, !prof !38

20:                                               ; preds = %17
  tail call void @_ZSt17__throw_bad_allocv() #23
  unreachable

21:                                               ; preds = %17
  %22 = tail call noalias noundef nonnull ptr @_Znwm(i64 noundef %18) #26
  store ptr %22, ptr %0, align 8, !tbaa !29
  store i64 %4, ptr %7, align 8, !tbaa !39
  br label %23

23:                                               ; preds = %21, %12
  %24 = phi ptr [ %22, %21 ], [ %7, %12 ]
  switch i64 %4, label %27 [
    i64 1, label %25
    i64 0, label %28
  ]

25:                                               ; preds = %23
  %26 = load i8, ptr %6, align 1, !tbaa !39
  store i8 %26, ptr %24, align 1, !tbaa !39
  br label %28

27:                                               ; preds = %23
  tail call void @llvm.memcpy.p0.p0.i64(ptr nonnull align 1 %24, ptr align 1 %6, i64 %4, i1 false)
  br label %28

28:                                               ; preds = %23, %25, %27
  %29 = getelementptr inbounds %"class.std::__cxx11::basic_string", ptr %0, i64 0, i32 1
  store i64 %4, ptr %29, align 8, !tbaa !32
  %30 = getelementptr inbounds i8, ptr %24, i64 %4
  store i8 0, ptr %30, align 1, !tbaa !39
  ret void
}

declare void @_ZNSt13runtime_errorC1ERKNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE(ptr noundef nonnull align 8 dereferenceable(16), ptr noundef nonnull align 8 dereferenceable(32)) unnamed_addr #13

; Function Attrs: nounwind
declare void @_ZNSt13runtime_errorD1Ev(ptr noundef nonnull align 8 dereferenceable(16)) unnamed_addr #6

declare void @__cxa_throw(ptr, ptr, ptr) local_unnamed_addr

declare void @__cxa_free_exception(ptr) local_unnamed_addr

; Function Attrs: noreturn
declare void @_ZSt19__throw_logic_errorPKc(ptr noundef) local_unnamed_addr #14

; Function Attrs: noreturn
declare void @_ZSt20__throw_length_errorPKc(ptr noundef) local_unnamed_addr #14

; Function Attrs: noreturn
declare void @_ZSt17__throw_bad_allocv() local_unnamed_addr #14

; Function Attrs: nounwind
declare i64 @__isoc23_strtol(ptr noundef, ptr noundef, i32 noundef) local_unnamed_addr #6

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write)
declare void @llvm.assume(i1 noundef) #15

; Function Attrs: nofree nounwind
declare noundef i32 @puts(ptr nocapture noundef readonly) local_unnamed_addr #16

; Function Attrs: mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite) uwtable
define internal fastcc void @_Z12range_valuesi.resume(ptr nocapture noundef nonnull align 8 dereferenceable(48) %0) #17 personality ptr @__gxx_personality_v0 {
  %2 = getelementptr inbounds %_Z12range_valuesi.Frame, ptr %0, i64 0, i32 2
  %3 = getelementptr inbounds %_Z12range_valuesi.Frame, ptr %0, i64 0, i32 5
  %4 = load i2, ptr %3, align 8
  %5 = icmp eq i2 %4, 0
  br i1 %5, label %6, label %13

6:                                                ; preds = %1
  %7 = getelementptr inbounds %_Z12range_valuesi.Frame, ptr %0, i64 0, i32 3
  %8 = load i32, ptr %7, align 8
  %9 = icmp sgt i32 %8, 0
  br i1 %9, label %10, label %20

10:                                               ; preds = %13, %6
  %11 = phi i32 [ 0, %6 ], [ %18, %13 ]
  %12 = getelementptr inbounds %_Z12range_valuesi.Frame, ptr %0, i64 0, i32 4
  store i32 %11, ptr %12, align 4
  store i32 %11, ptr %2, align 8, !tbaa !5
  store i2 1, ptr %3, align 8
  br label %21

13:                                               ; preds = %1
  %14 = getelementptr inbounds %_Z12range_valuesi.Frame, ptr %0, i64 0, i32 4
  %15 = load i32, ptr %14, align 4
  %16 = getelementptr inbounds %_Z12range_valuesi.Frame, ptr %0, i64 0, i32 3
  %17 = load i32, ptr %16, align 8
  %18 = add nuw nsw i32 %15, 1
  %19 = icmp eq i32 %18, %17
  br i1 %19, label %20, label %10, !llvm.loop !18

20:                                               ; preds = %13, %6
  store ptr null, ptr %0, align 8
  br label %21

21:                                               ; preds = %10, %20
  ret void
}

; Function Attrs: mustprogress nounwind uwtable
define internal fastcc void @_Z12range_valuesi.destroy(ptr noundef nonnull align 8 dereferenceable(48) %0) #18 personality ptr @__gxx_personality_v0 {
  %2 = getelementptr inbounds %_Z12range_valuesi.Frame, ptr %0, i64 0, i32 2, i32 1
  %3 = load ptr, ptr %2, align 8, !tbaa !12
  %4 = icmp eq ptr %3, null
  br i1 %4, label %6, label %5

5:                                                ; preds = %1
  tail call void @_ZNSt15__exception_ptr13exception_ptr10_M_releaseEv(ptr noundef nonnull align 8 dereferenceable(8) %2) #22
  br label %6

6:                                                ; preds = %5, %1
  tail call void @_ZdlPv(ptr noundef nonnull %0) #22
  ret void
}

; Function Attrs: nofree nounwind willreturn memory(argmem: read)
declare i32 @bcmp(ptr nocapture, ptr nocapture, i64) local_unnamed_addr #19

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.vector.reduce.add.v2i64(<2 x i64>) #20

attributes #0 = { mustprogress uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nobuiltin allocsize(0) "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { mustprogress nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #3 = { nobuiltin nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { mustprogress nofree norecurse nosync nounwind willreturn memory(none) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #5 = { mustprogress nofree norecurse nosync nounwind memory(write, argmem: none, inaccessiblemem: none) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #6 = { nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #7 = { mustprogress nofree nounwind willreturn memory(argmem: read) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #8 = { mustprogress nocallback nofree nounwind willreturn memory(argmem: readwrite) }
attributes #9 = { nofree nounwind "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #10 = { mustprogress norecurse uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #11 = { nofree nosync nounwind memory(none) }
attributes #12 = { mustprogress nofree nounwind willreturn memory(argmem: read) "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #13 = { "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #14 = { noreturn "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #15 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #16 = { nofree nounwind }
attributes #17 = { mustprogress nofree norecurse nosync nounwind willreturn memory(argmem: readwrite) uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #18 = { mustprogress nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #19 = { nofree nounwind willreturn memory(argmem: read) }
attributes #20 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #21 = { allocsize(0) }
attributes #22 = { nounwind }
attributes #23 = { noreturn }
attributes #24 = { builtin nounwind }
attributes #25 = { cold }
attributes #26 = { builtin allocsize(0) }

!llvm.linker.options = !{}
!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"Ubuntu clang version 18.1.3 (1ubuntu1)"}
!5 = !{!6, !7, i64 0}
!6 = !{!"_ZTSN4demo9generatorIiE12promise_typeE", !7, i64 0, !10, i64 8}
!7 = !{!"int", !8, i64 0}
!8 = !{!"omnipotent char", !9, i64 0}
!9 = !{!"Simple C++ TBAA"}
!10 = !{!"_ZTSNSt15__exception_ptr13exception_ptrE", !11, i64 0}
!11 = !{!"any pointer", !8, i64 0}
!12 = !{!10, !11, i64 0}
!13 = !{i64 0, i64 8, !14}
!14 = !{!11, !11, i64 0}
!15 = !{!16}
!16 = distinct !{!16, !17, !"_ZN4demo9generatorIiE12promise_type17get_return_objectEv: argument 0"}
!17 = distinct !{!17, !"_ZN4demo9generatorIiE12promise_type17get_return_objectEv"}
!18 = distinct !{!18, !19}
!19 = !{!"llvm.loop.mustprogress"}
!20 = distinct !{!20, !19, !21, !22}
!21 = !{!"llvm.loop.isvectorized", i32 1}
!22 = !{!"llvm.loop.unroll.runtime.disable"}
!23 = distinct !{!23, !19, !22, !21}
!24 = distinct !{!24, !19, !21, !22}
!25 = distinct !{!25, !19, !22, !21}
!26 = !{!27, !27, i64 0}
!27 = !{!"long", !8, i64 0}
!28 = distinct !{!28, !19}
!29 = !{!30, !11, i64 0}
!30 = !{!"_ZTSNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE", !31, i64 0, !27, i64 8, !8, i64 16}
!31 = !{!"_ZTSNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEE12_Alloc_hiderE", !11, i64 0}
!32 = !{!30, !27, i64 8}
!33 = !{ptr @_Z13local_consumei, ptr @_Z15escaped_consumei}
!34 = !{!35, !35, i64 0}
!35 = !{!"vtable pointer", !9, i64 0}
!36 = !{i64 0, i64 8, !26, i64 8, i64 8, !14}
!37 = !{!31, !11, i64 0}
!38 = !{!"branch_weights", i32 1, i32 2000}
!39 = !{!8, !8, i64 0}
