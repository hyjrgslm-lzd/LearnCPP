cmake_minimum_required(VERSION 3.28)
project(CAPSTONE2_A_cutlass_deep_dive LANGUAGES CXX CUDA)

# ============================================================
# 分支 A：CUTLASS 3.x CollectiveBuilder 精读 + 手写 warp-specialized GEMM
# 要求：Hopper sm_90a，CUTLASS 3.x headers，CUDA 13.x
# ============================================================

add_executable(CAPSTONE2_A_cutlass_deep_dive main.cu)

target_link_libraries(CAPSTONE2_A_cutlass_deep_dive PRIVATE
    gpu_study_common
    cutlass::headers)

set_target_properties(CAPSTONE2_A_cutlass_deep_dive PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON
    # sm_90a：Hopper warp-specialized GEMM（TMA + wgmma）
    CUDA_ARCHITECTURES "90a")

# UTF-8 源文件（MSVC）
if(MSVC)
    target_compile_options(CAPSTONE2_A_cutlass_deep_dive PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:/utf-8>)
endif()

# 需要 Hopper sm_90a：CUTLASS SM90 warp-specialized GEMM（CollectiveBuilder 生成 KernelTmaWarpSpecializedCooperative）
# 非 Hopper 机器运行时会触发 runtime skip（main 函数内 has_hopper_features 检测）
# 编译时仍可通过（CUDA_ARCHITECTURES 设置了 sm_90a，若编译机无 sm_90a 驱动，nvcc 依然能编译 PTX）

message(STATUS "[CAPSTONE2_A] CUDA_ARCHITECTURES = ${CMAKE_CUDA_ARCHITECTURES} -> overridden to 90a for this target")
