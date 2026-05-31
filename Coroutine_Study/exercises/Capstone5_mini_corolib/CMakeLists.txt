cmake_minimum_required(VERSION 3.25)
project(Capstone5_mini_corolib LANGUAGES CXX)

# 如本目录被独立打开（不通过顶层 add_subdirectory），跳过
if(NOT TARGET stdexec::stdexec AND NOT COROUTINE_STUDY_STANDALONE)
    message(WARNING "Capstone5_mini_corolib requires stage3 deps; configure via top-level CMakeLists.txt")
    return()
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /await:strict /Zi /bigobj)
endif()

# helper: 创建一个 mini-* target，自动注入 include 路径与 stage3 deps
function(_mini_add_executable target source)
    add_executable(${target} ${source})
    if(COMMAND coroutine_study_link_stage3_deps)
        coroutine_study_link_stage3_deps(${target})
    else()
        target_link_libraries(${target} PRIVATE stdexec::stdexec)
    endif()
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/include
    )
endfunction()

# 主 demo
_mini_add_executable(Capstone5_mini_corolib src/main.cpp)

# 测试可执行（每组件一个 TU）
_mini_add_executable(mini_task_test       tests/task_test.cpp)
_mini_add_executable(mini_generator_test  tests/generator_test.cpp)
_mini_add_executable(mini_when_all_test   tests/when_all_test.cpp)
_mini_add_executable(mini_sync_wait_test  tests/sync_wait_test.cpp)
_mini_add_executable(mini_scope_test      tests/scope_test.cpp)
_mini_add_executable(mini_as_awaitable_test tests/as_awaitable_test.cpp)

# 进阶：HALO 验证 flag —— 仅 Clang 17+
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(Capstone5_mini_corolib PRIVATE
        -Rpass=coroutine-elide
        -Rpass-missed=coroutine-elide
    )
endif()
