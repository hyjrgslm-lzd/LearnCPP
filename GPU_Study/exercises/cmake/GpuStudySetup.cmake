# GpuStudySetup.cmake — 警告级别 / sanitizer / 工具链开关
include_guard(GLOBAL)

option(GPU_STUDY_ENABLE_WARNINGS "启用较严的警告等级" ON)
option(GPU_STUDY_ENABLE_ASAN     "启用 AddressSanitizer（仅 host）" OFF)
option(GPU_STUDY_ENABLE_UBSAN    "启用 UndefinedBehaviorSanitizer（仅 host）" OFF)

if(GPU_STUDY_ENABLE_WARNINGS)
    if(MSVC)
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:/W4>)
    else()
        add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-Wall>
                            $<$<COMPILE_LANGUAGE:CXX>:-Wextra>
                            $<$<COMPILE_LANGUAGE:CXX>:-Wpedantic>)
    endif()
endif()

# nvcc 的警告转发：默认关 -Wextra 避免海量模板噪音；需要时手工打开
option(GPU_STUDY_CUDA_WARNINGS "对 CUDA 源也启用 host-warning 转发" OFF)
if(GPU_STUDY_CUDA_WARNINGS AND NOT MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=-Wall>)
endif()

if(GPU_STUDY_ENABLE_ASAN AND NOT MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fsanitize=address>)
    add_link_options(-fsanitize=address)
endif()

if(GPU_STUDY_ENABLE_UBSAN AND NOT MSVC)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fsanitize=undefined>)
    add_link_options(-fsanitize=undefined)
endif()

message(STATUS "GPU_Study: CXX=${CMAKE_CXX_STANDARD}  CUDA=${CMAKE_CUDA_STANDARD}  ARCHS=${CMAKE_CUDA_ARCHITECTURES}")
