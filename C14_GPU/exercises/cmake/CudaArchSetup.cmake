# CudaArchSetup.cmake — configure-time CUDA 架构自适应
#
# 设计目标：
#   项目可拷贝到任意机器，自动适配本机 nvcc 实际支持的 GPU 架构。
#   把项目期望架构集（Ampere/Ada/Hopper/Blackwell）与 nvcc --list-gpu-arch
#   结果取交集，结合 CUDA toolkit 版本判定 architecture-accelerated（_a）形式可用性，
#   动态写入 CMAKE_CUDA_ARCHITECTURES，并把启用项作为编译宏暴露给 C++/CUDA 代码。
#
# 暴露：
#   函数 gpu_study_resolve_cuda_architectures()
#       —— 必须在 project(... CUDA) 之前调用，设置 CMAKE_CUDA_ARCHITECTURES。
#   函数 gpu_study_define_arch_macros(<target>)
#       —— project() 之后调用，给 target 注入 GPU_STUDY_HAS_SM_<XX>{,A}=1
#          及家族聚合宏 GPU_STUDY_HAS_{AMPERE,HOPPER,BLACKWELL}=1。
#
# 期望架构集（按发布顺序，新→旧）：
#   120a / 120  —— Blackwell consumer (RTX 50)
#   100a / 100  —— Blackwell datacenter (B100/B200)
#   90a  / 90   —— Hopper (H100/H200)
#   89          —— Ada Lovelace (RTX 40)
#   86          —— Ampere consumer (RTX 30)
#   80          —— Ampere datacenter (A100)

include_guard(GLOBAL)

# ─────────────────────────────────────────────────────────────────────────────
# Architecture-accelerated (_a) 形式的最小 CUDA toolkit 版本
# 来源：NVIDIA CUDA Toolkit Release Notes
# ─────────────────────────────────────────────────────────────────────────────
#   sm_90a  : CUDA 12.0+
#   sm_100a : CUDA 12.8+
#   sm_120a : CUDA 12.8+
#
# 普通 sm_X 形式只受 --list-gpu-arch 输出门控。

set(_GPU_STUDY_DESIRED_ARCHS "80;86;89;90;90a;100;100a;120;120a")

function(_gpu_study_find_nvcc out_var)
    find_program(_nvcc_path
        NAMES nvcc
        HINTS ENV CUDAToolkit_ROOT ENV CUDA_PATH ENV CUDA_HOME
        PATH_SUFFIXES bin
        NO_CACHE)
    set(${out_var} "${_nvcc_path}" PARENT_SCOPE)
endfunction()

function(_gpu_study_query_nvcc_version nvcc out_major out_minor)
    execute_process(
        COMMAND "${nvcc}" --version
        OUTPUT_VARIABLE _ver
        ERROR_QUIET
        RESULT_VARIABLE _rc)
    set(${out_major} "0" PARENT_SCOPE)
    set(${out_minor} "0" PARENT_SCOPE)
    if(NOT _rc EQUAL 0)
        return()
    endif()
    if(_ver MATCHES "release ([0-9]+)\\.([0-9]+)")
        set(${out_major} "${CMAKE_MATCH_1}" PARENT_SCOPE)
        set(${out_minor} "${CMAKE_MATCH_2}" PARENT_SCOPE)
    endif()
endfunction()

function(_gpu_study_query_supported_bases nvcc out_bases)
    execute_process(
        COMMAND "${nvcc}" --list-gpu-arch
        OUTPUT_VARIABLE _out
        ERROR_QUIET
        RESULT_VARIABLE _rc)
    set(_bases "")
    if(_rc EQUAL 0)
        string(REPLACE "\n" ";" _lines "${_out}")
        foreach(_line IN LISTS _lines)
            string(STRIP "${_line}" _line)
            if(_line MATCHES "^compute_([0-9]+)$")
                list(APPEND _bases "${CMAKE_MATCH_1}")
            endif()
        endforeach()
        list(REMOVE_DUPLICATES _bases)
    endif()
    set(${out_bases} "${_bases}" PARENT_SCOPE)
endfunction()

# arch 形如 90 / 90a；返回 base / suffix 与是否兼容当前 nvcc
function(_gpu_study_arch_supported arch supported_bases cuda_major cuda_minor out_ok)
    if(NOT arch MATCHES "^([0-9]+)(a?)$")
        set(${out_ok} FALSE PARENT_SCOPE)
        return()
    endif()
    set(_base "${CMAKE_MATCH_1}")
    set(_suffix "${CMAKE_MATCH_2}")

    if(NOT _base IN_LIST supported_bases)
        set(${out_ok} FALSE PARENT_SCOPE)
        return()
    endif()

    if(_suffix STREQUAL "a")
        # _a 形式的最小 CUDA 版本门控
        set(_min_major 12)
        set(_min_minor 0)
        if(_base STREQUAL "100" OR _base STREQUAL "120")
            set(_min_minor 8)
        endif()
        if(cuda_major LESS _min_major)
            set(${out_ok} FALSE PARENT_SCOPE)
            return()
        endif()
        if(cuda_major EQUAL _min_major AND cuda_minor LESS _min_minor)
            set(${out_ok} FALSE PARENT_SCOPE)
            return()
        endif()
    endif()

    set(${out_ok} TRUE PARENT_SCOPE)
endfunction()

# ─────────────────────────────────────────────────────────────────────────────
# Public：探测 + 设置 CMAKE_CUDA_ARCHITECTURES（必须在 project() 之前调用）
# ─────────────────────────────────────────────────────────────────────────────
function(gpu_study_resolve_cuda_architectures)
    if(DEFINED CMAKE_CUDA_ARCHITECTURES AND NOT CMAKE_CUDA_ARCHITECTURES STREQUAL "")
        message(STATUS "[CudaArch] 用户显式指定 CMAKE_CUDA_ARCHITECTURES=${CMAKE_CUDA_ARCHITECTURES}，跳过探测")
        return()
    endif()

    _gpu_study_find_nvcc(_nvcc)
    if(NOT _nvcc)
        message(WARNING "[CudaArch] 未找到 nvcc。CMAKE_CUDA_ARCHITECTURES 将由 CMake 默认推断（可能为 sm_75）。"
                        "建议设置 CUDAToolkit_ROOT / CUDA_PATH 环境变量。")
        return()
    endif()
    message(STATUS "[CudaArch] nvcc = ${_nvcc}")

    _gpu_study_query_nvcc_version("${_nvcc}" _cmajor _cminor)
    message(STATUS "[CudaArch] CUDA toolkit ${_cmajor}.${_cminor}")

    _gpu_study_query_supported_bases("${_nvcc}" _bases)
    if(NOT _bases)
        message(WARNING "[CudaArch] nvcc --list-gpu-arch 解析失败，回退到 CMake 默认行为")
        return()
    endif()
    message(STATUS "[CudaArch] nvcc 支持的 base archs: ${_bases}")

    set(_enabled "")
    foreach(_a IN LISTS _GPU_STUDY_DESIRED_ARCHS)
        _gpu_study_arch_supported("${_a}" "${_bases}" "${_cmajor}" "${_cminor}" _ok)
        if(_ok)
            list(APPEND _enabled "${_a}")
        endif()
    endforeach()

    if(NOT _enabled)
        # 极端兜底：nvcc 太老（如 11.x），从 supported_bases 选最高一个
        list(SORT _bases COMPARE NATURAL ORDER DESCENDING)
        list(GET _bases 0 _fallback)
        set(_enabled "${_fallback}")
        message(WARNING "[CudaArch] 期望集与 nvcc 支持集无交集，回退到 sm_${_fallback}")
    endif()

    set(CMAKE_CUDA_ARCHITECTURES "${_enabled}" CACHE STRING
        "CUDA architectures (auto-resolved by CudaArchSetup)" FORCE)
    message(STATUS "[CudaArch] CMAKE_CUDA_ARCHITECTURES = ${_enabled}")
endfunction()

# ─────────────────────────────────────────────────────────────────────────────
# Public：把启用的 arch 作为 compile def 暴露给 target
# 用法：gpu_study_define_arch_macros(gpu_study_common)  # INTERFACE target
# ─────────────────────────────────────────────────────────────────────────────
function(gpu_study_define_arch_macros target)
    get_target_property(_type "${target}" TYPE)
    if(_type STREQUAL "INTERFACE_LIBRARY")
        set(_scope INTERFACE)
    else()
        set(_scope PUBLIC)
    endif()

    set(_has_ampere FALSE)
    set(_has_hopper FALSE)
    set(_has_blackwell FALSE)

    foreach(_a IN LISTS CMAKE_CUDA_ARCHITECTURES)
        if(NOT _a MATCHES "^([0-9]+)(a?)$")
            continue()
        endif()
        set(_base "${CMAKE_MATCH_1}")
        set(_suffix "${CMAKE_MATCH_2}")

        # 单架构宏：GPU_STUDY_HAS_SM_90 / GPU_STUDY_HAS_SM_90A 等
        string(TOUPPER "${_a}" _aU)
        target_compile_definitions("${target}" ${_scope} "GPU_STUDY_HAS_SM_${_aU}=1")

        # 家族归属
        if(_base GREATER_EQUAL 80 AND _base LESS 90)
            set(_has_ampere TRUE)
        elseif(_base GREATER_EQUAL 90 AND _base LESS 100)
            set(_has_hopper TRUE)
        elseif(_base GREATER_EQUAL 100)
            set(_has_blackwell TRUE)
        endif()
    endforeach()

    if(_has_ampere)
        target_compile_definitions("${target}" ${_scope} GPU_STUDY_HAS_AMPERE=1)
    endif()
    if(_has_hopper)
        target_compile_definitions("${target}" ${_scope} GPU_STUDY_HAS_HOPPER=1)
    endif()
    if(_has_blackwell)
        target_compile_definitions("${target}" ${_scope} GPU_STUDY_HAS_BLACKWELL=1)
    endif()

    message(STATUS "[CudaArch] target ${target} 注入宏：archs=${CMAKE_CUDA_ARCHITECTURES} "
                   "ampere=${_has_ampere} hopper=${_has_hopper} blackwell=${_has_blackwell}")
endfunction()
