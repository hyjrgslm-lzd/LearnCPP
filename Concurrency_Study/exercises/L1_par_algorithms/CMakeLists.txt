cmake_minimum_required(VERSION 3.25)
project(L1_par_algorithms LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /EHsc)
endif()

# 让本题既能作为顶层子目录、也能被独立打开时找到 cmake/*Setup.cmake
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

add_executable(L1_par_algorithms main.cpp)
target_include_directories(L1_par_algorithms PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
# batch3 接入并行 STL 后端（MSVC 内置无需；GCC/Clang 需 TBB）：
# include(TbbSetup)
# target_link_libraries(L1_par_algorithms PRIVATE ${CONCURRENCY_STUDY_PAR_LIBS})
