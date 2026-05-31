cmake_minimum_required(VERSION 3.28)

# ============================================================
# pre-project() 配置块（顺序敏感）：
#   1. CMAKE_MODULE_PATH 早注册，使后续 include() 生效
#   2. MSVC C++26 标志注入
#   3. CUDA 架构 configure-time 探测（必须早于 project(... CUDA)）
# ============================================================
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# MSVC C++26 启用补丁：CMake 4.x 尚未把 MSVC 19.5x 自动映射为 C++26
if(MSVC OR CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_CXX26_STANDARD_COMPILE_OPTION  "-std:c++latest")
    set(CMAKE_CXX26_EXTENSION_COMPILE_OPTION "-std:c++latest")
endif()

# CUDA 架构自适应：调 nvcc --list-gpu-arch 与期望集取交集
include(CudaArchSetup)
gpu_study_resolve_cuda_architectures()

project(GPU_Study LANGUAGES CXX CUDA)

# Host: C++26（对齐 Ranges_Study）
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Device: CUDA C++20（nvcc 13.x 主流）
set(CMAKE_CUDA_STANDARD 20)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_EXTENSIONS OFF)

# ============================================================
# MSVC 专属编译选项 —— 必须用 generator expression 限定语言：
# 裸传给 nvcc 时 / 前缀参数会被识别为输入文件 (NVCC §4.2.5.6)，触发
# "single input file required" fatal。CUDA host 端真正需要的部分通过
# -Xcompiler 显式转发。
# ============================================================
if(MSVC)
    # CXX (.cpp): conforming preprocessor — OptiX 8 headers (e.g.
    # optix_denoiser_tiling.h) use __VA_OPT__; traditional preprocessor errors
    # with C2589.
    add_compile_options(
        $<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>
        $<$<COMPILE_LANGUAGE:CXX>:/Zc:preprocessor>
        $<$<COMPILE_LANGUAGE:CXX>:/utf-8>
        $<$<COMPILE_LANGUAGE:CXX>:/permissive->
    )
    # CUDA host: KEEP traditional preprocessor.
    #   /Zc:preprocessor exposes a CCCL bug where MSVC's SAL legacy macros
    #   (__in / __out / __inout, defined as empty object-like in <sal.h>)
    #   textually erase CCCL's parameter names of the same spelling in
    #   cccl/cuda/__ptx/instructions/generated/tcgen05_ld.h ('asm operand must
    #   have scalar type' on every wrapper). /U cannot durably undefine them
    #   because <sal.h> redefines them on every include.
    #   Instead we silence CCCL's traditional-preprocessor #error (it is only
    #   a warning, no exercise here actually depends on the affected paths)
    #   via CCCL_IGNORE_MSVC_TRADITIONAL_PREPROCESSOR_WARNING.
    #   /Zc:__cplusplus    : make __cplusplus reflect the actual standard
    add_compile_options(
        $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/Zc:__cplusplus>
    )
    add_compile_definitions(
        $<$<COMPILE_LANGUAGE:CUDA>:CCCL_IGNORE_MSVC_TRADITIONAL_PREPROCESSOR_WARNING>
    )
endif()

# CUDA 通用开关
add_compile_options($<$<COMPILE_LANGUAGE:CUDA>:--expt-relaxed-constexpr>)
add_compile_options($<$<COMPILE_LANGUAGE:CUDA>:--extended-lambda>)

# Nsight Compute source-SASS 需要的行号信息（可选打开）
option(GPU_STUDY_LINEINFO "Generate --generate-line-info for Nsight Compute" OFF)
if(GPU_STUDY_LINEINFO)
    add_compile_options($<$<COMPILE_LANGUAGE:CUDA>:--generate-line-info>)
endif()

# 扩展位（warnings / sanitizer / CUTLASS FetchContent / OptiX 发现）
include(GpuStudySetup)
include(CutlassSetup)
include(OptiXSetup)

# 运行时依赖
find_package(CUDAToolkit REQUIRED)

# ============================================================
# 公共 header-only 接口
# ============================================================
add_library(gpu_study_common INTERFACE)
# 源码用 #include "common/xxx.cuh" 写法 → 暴露 exercises 根作为 include 搜索路径
target_include_directories(gpu_study_common INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
# CUDA 12+ 移除 CUDA::nvToolsExt 导入目标；NVTX3 改为 header-only 的 CUDA::nvtx3
target_link_libraries(gpu_study_common INTERFACE CUDA::cudart CUDA::nvtx3)
# 注入启用的架构宏（GPU_STUDY_HAS_SM_<XX>{,A}=1 / GPU_STUDY_HAS_{AMPERE,HOPPER,BLACKWELL}=1）
gpu_study_define_arch_macros(gpu_study_common)

# ============================================================
# 练习子目录（P2/P4 阶段由各 agent 追加 add_subdirectory）
# ============================================================
# Stage 1
add_subdirectory(A1_hello_device)
add_subdirectory(A2_index_mapping)
add_subdirectory(A3_nvcc_ptx_sass)
add_subdirectory(A4_sanitizer_debug)
add_subdirectory(B1_global_coalescing)
add_subdirectory(B2_shared_memory_basics)
add_subdirectory(B3_bank_conflict_and_swizzle)
add_subdirectory(B4_pinned_vs_pageable_and_unified)
add_subdirectory(B5_async_memcpy_and_cp_async)
add_subdirectory(B6_texture_and_constant)
add_subdirectory(C1_block_warp_thread_layout)
add_subdirectory(C2_syncthreads_and_divergence)
add_subdirectory(C3_cooperative_groups)
add_subdirectory(C4_hopper_cluster_launch)
add_subdirectory(C5_mbarrier_async_barrier)
add_subdirectory(D1_warp_shuffle_basics)
add_subdirectory(D2_vote_ballot)
add_subdirectory(D3_warp_reduce_scan)
add_subdirectory(D4_ldg_and_read_only_cache)
add_subdirectory(D5_occupancy_and_launch_bounds)
add_subdirectory(D6_bottleneck_classification)
add_subdirectory(E1_streams_and_events)
add_subdirectory(E2_async_mem_pool)
add_subdirectory(E3_cuda_graph_capture)
add_subdirectory(E4_cuda_graph_explicit)
add_subdirectory(E5_mps_and_concurrent_clients)
add_subdirectory(F1_nsight_systems_timeline)
add_subdirectory(F2_nsight_compute_first_look)
add_subdirectory(F3_roofline_and_bottleneck)
add_subdirectory(F4_compute_sanitizer_suite)
add_subdirectory(F5_nsight_vse_kernel_breakpoint)
# add_subdirectory(CAPSTONE1_data_parallel_pipeline)
add_subdirectory(CAPSTONE1_image_pipeline)

# Stage 2
add_subdirectory(G1_wmma_fp16_basic)
add_subdirectory(G2_mma_sync_ptx)
add_subdirectory(G3_hopper_wgmma)
add_subdirectory(G4_tma_cp_async_bulk)
add_subdirectory(G5_warp_specialized_pipeline)
add_subdirectory(H1_gemm_naive_and_tiled)
add_subdirectory(H2_gemm_warp_tile_and_mma)
add_subdirectory(H3_gemm_pipelined_double_buffer)
add_subdirectory(H4_gemm_hopper_warp_specialized)
add_subdirectory(H5_cutlass_collective_builder)
add_subdirectory(H6_cute_layout_and_tensor)
add_subdirectory(I1_softmax_online)
add_subdirectory(I2_layernorm_rmsnorm_welford)
add_subdirectory(I3_gelu_silu_fused)
add_subdirectory(I4_attention_fwd_unfused_vs_fused)
add_subdirectory(I5_flash_attention_v2_style)
add_subdirectory(I6_fp8_gemm_with_scaling)
# add_subdirectory(J1_cublas_cublaslt)
# add_subdirectory(J2_cudnn_graph_api)
# add_subdirectory(J3_nccl_allreduce)
# add_subdirectory(J4_triton_dsl)
# add_subdirectory(J5_tensorrt_engine)
# add_subdirectory(J6_transformer_engine_fp8)
# add_subdirectory(K1_optix_pipeline)
# add_subdirectory(K2_optix_bvh_sbt)
# add_subdirectory(K3_optix_denoiser)
# add_subdirectory(K4_optix_cuda_interop)
add_subdirectory(K1_optix_hello_pipeline)
add_subdirectory(K2_sbt_and_acceleration_structure)
add_subdirectory(K3_intersection_anyhit_closesthit_miss)
add_subdirectory(K4_optix_denoiser_and_cuda_interop)
# add_subdirectory(CAPSTONE2a_cutlass_gemm_study)
# add_subdirectory(CAPSTONE2b_flash_attention_repro)
# add_subdirectory(CAPSTONE2c_optix_cornell_box)
