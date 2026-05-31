cmake_minimum_required(VERSION 3.28)

# ============================================================
# MSVC C++26 启用补丁
# 当前 CMake (4.2.x) 尚未把 MSVC 19.5x 映射到 /std:c++latest 作为 C++26，
# 这里在 project() 之前显式注入编译选项，使 CMAKE_CXX_STANDARD 26 可用。
# ============================================================
if(MSVC OR CMAKE_GENERATOR MATCHES "Visual Studio")
    set(CMAKE_CXX26_STANDARD_COMPILE_OPTION  "-std:c++latest")
    set(CMAKE_CXX26_EXTENSION_COMPILE_OPTION "-std:c++latest")
endif()

project(ranges_exercises LANGUAGES CXX)

# ============================================================
# C++ 标准：统一 C++26
# ============================================================
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# MSVC：启用 __cplusplus 正确报告 + UTF-8 源码 + 严格一致性
if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /permissive-)
endif()

# 预留扩展位（sanitizer / warning level 等）
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(RangesSetup)

# ============================================================
# 阶段一：使用层（概念理解与管道组合）
# ============================================================
add_subdirectory(01_mental_model_warmup)
add_subdirectory(A1_iota_view)
add_subdirectory(A2_istream_view)
add_subdirectory(A3_repeat_cartesian)
add_subdirectory(B1_all_ref_owning)
add_subdirectory(B2_filter_transform_degrade)
add_subdirectory(B3_take_drop_closure)
add_subdirectory(C1_1_join)
add_subdirectory(C1_2_split_evolution)
add_subdirectory(C1_3_common_reverse_elements)
add_subdirectory(C2_1_projection)
add_subdirectory(C2_2_dangling_borrowed)
add_subdirectory(C2_3_ranges_to)
add_subdirectory(D1_zip_adjacent_chunk)
add_subdirectory(D2_chunk_by_join_with_asconst)
add_subdirectory(D3_std_generator)
add_subdirectory(CAPSTONE1_log_pipeline)

# ============================================================
# 阶段二：实现层（自定义 view 与适配器开发）
# ============================================================
add_subdirectory(E1_my_begin_cpo)
add_subdirectory(E2_niebloid)
add_subdirectory(E3_cpo_tagdispatch_compare)
add_subdirectory(F1_iterator_hierarchy)
add_subdirectory(G1_my_take_view)
add_subdirectory(G2_my_transform_closure)
add_subdirectory(G3_my_enumerate_borrowed)
add_subdirectory(H1_non_propagating_cache)
add_subdirectory(H2_common_iter_proxy)
add_subdirectory(H3_generator_const_iter)
add_subdirectory(CAPSTONE3_impl_source_reading)
add_subdirectory(CAPSTONE4_mini_ranges)
