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
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../include")
        target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    endif()
endfunction()

function(_mini_register_test target label timeout)
    if(BUILD_TESTING)
        add_test(NAME ${target} COMMAND ${target})
        set_tests_properties(${target} PROPERTIES
            LABELS "${label}"
            TIMEOUT ${timeout}
        )
    endif()
endfunction()

# 主 demo
_mini_add_executable(Capstone5_mini_corolib src/main.cpp)

# 测试可执行（每组件一个 TU）
_mini_add_executable(mini_task_test       tests/task_test.cpp)
_mini_add_executable(mini_generator_test  tests/generator_test.cpp)
_mini_add_executable(mini_when_all_test   tests/when_all_test.cpp)
_mini_add_executable(mini_sync_wait_test  tests/sync_wait_test.cpp)
_mini_add_executable(mini_scope_test      tests/scope_test.cpp)
_mini_add_executable(mini_when_any_test   tests/when_any_test.cpp)
_mini_add_executable(mini_shared_task_test tests/shared_task_test.cpp)
_mini_add_executable(mini_run_loop_test   tests/run_loop_test.cpp)
_mini_add_executable(mini_stop_test       tests/stop_test.cpp)
if(COROUTINE_STUDY_TEST_STARTERS)
    _mini_register_test(mini_task_test starter 30)
    _mini_register_test(mini_generator_test starter 30)
    _mini_register_test(mini_when_all_test starter 30)
    _mini_register_test(mini_sync_wait_test starter 30)
    _mini_register_test(mini_scope_test starter 30)
    _mini_register_test(mini_when_any_test starter 30)
    _mini_register_test(mini_shared_task_test starter 30)
    _mini_register_test(mini_run_loop_test starter 30)
    _mini_register_test(mini_stop_test "starter;observation" 30)
endif()
if(TARGET stdexec::stdexec)
    _mini_add_executable(mini_task_sender_test tests/task_sender_test.cpp)
    target_link_libraries(mini_task_sender_test PRIVATE stdexec::stdexec)
    _mini_add_executable(mini_as_awaitable_test tests/as_awaitable_test.cpp)
    target_link_libraries(mini_as_awaitable_test PRIVATE stdexec::stdexec)
    if(COROUTINE_STUDY_TEST_STARTERS)
        _mini_register_test(mini_task_sender_test "starter;stdexec" 30)
        _mini_register_test(mini_as_awaitable_test starter 30)
    endif()
endif()

if(COROUTINE_STUDY_BUILD_REFERENCE)
    function(_mini_add_reference_test target source)
        add_executable(${target} ${source})
        target_include_directories(${target} PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/reference/include
        )
        _mini_register_test(${target} reference 30)
    endfunction()

    _mini_add_reference_test(mini_reference_task        reference/tests/task_test.cpp)
    _mini_add_reference_test(mini_reference_generator   reference/tests/generator_test.cpp)
    _mini_add_reference_test(mini_reference_sync_wait   reference/tests/sync_wait_test.cpp)
    _mini_add_reference_test(mini_reference_when_all    reference/tests/when_all_test.cpp)
    _mini_add_reference_test(mini_reference_when_any    reference/tests/when_any_test.cpp)
    _mini_add_reference_test(mini_reference_run_loop    reference/tests/run_loop_test.cpp)
    _mini_add_reference_test(mini_reference_task_scope  reference/tests/task_scope_test.cpp)
    _mini_add_reference_test(mini_reference_scope_spawn_failure reference/tests/scope_spawn_failure_test.cpp)
    _mini_add_reference_test(mini_reference_stop        reference/tests/stop_test.cpp)
    _mini_add_reference_test(mini_reference_shared_task reference/tests/shared_task_test.cpp)
    _mini_add_reference_test(mini_reference_shared_task_abandon reference/tests/shared_task_abandon_test.cpp)
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
