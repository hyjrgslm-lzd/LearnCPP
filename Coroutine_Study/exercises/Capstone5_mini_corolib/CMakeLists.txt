cmake_minimum_required(VERSION 3.25)
project(Capstone5_mini_corolib LANGUAGES CXX)

include(CTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /await:strict /Zi /bigobj)
endif()

function(_mini_add_executable target source)
    add_executable(${target} ${source})
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
if(TARGET stdexec::stdexec)
    _mini_add_executable(mini_as_awaitable_test tests/as_awaitable_test.cpp)
    target_link_libraries(mini_as_awaitable_test PRIVATE stdexec::stdexec)
endif()

if(COROUTINE_STUDY_BUILD_REFERENCE)
    function(_mini_add_reference_test target source)
        add_executable(${target} ${source})
        target_include_directories(${target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/reference/include
        )
        add_test(NAME ${target} COMMAND ${target})
    endfunction()

    _mini_add_reference_test(mini_reference_task        reference/tests/task_test.cpp)
    _mini_add_reference_test(mini_reference_generator   reference/tests/generator_test.cpp)
    _mini_add_reference_test(mini_reference_sync_wait   reference/tests/sync_wait_test.cpp)
    _mini_add_reference_test(mini_reference_when_all    reference/tests/when_all_test.cpp)
    _mini_add_reference_test(mini_reference_when_any    reference/tests/when_any_test.cpp)
    _mini_add_reference_test(mini_reference_run_loop    reference/tests/run_loop_test.cpp)
    _mini_add_reference_test(mini_reference_task_scope  reference/tests/task_scope_test.cpp)
    _mini_add_reference_test(mini_reference_stop        reference/tests/stop_test.cpp)
    _mini_add_reference_test(mini_reference_shared_task reference/tests/shared_task_test.cpp)
    if(TARGET stdexec::stdexec)
        _mini_add_reference_test(mini_reference_as_awaitable reference/tests/as_awaitable_test.cpp)
        target_link_libraries(mini_reference_as_awaitable PRIVATE stdexec::stdexec)
    endif()
endif()

# 进阶：HALO 验证 flag —— 仅 Clang 17+
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(Capstone5_mini_corolib PRIVATE
        -Rpass=coroutine-elide
        -Rpass-missed=coroutine-elide
    )
endif()
