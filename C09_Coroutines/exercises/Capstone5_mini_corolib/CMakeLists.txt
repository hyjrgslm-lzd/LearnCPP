cmake_minimum_required(VERSION 3.25)
project(Capstone5_mini_corolib LANGUAGES CXX)

if(NOT COMMAND coroutine_study_add_exercise)
    include("${CMAKE_CURRENT_LIST_DIR}/../cmake/StudySetup.cmake")
endif()

include(CTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/Zc:__cplusplus /utf-8 /await:strict /Zi /bigobj)
endif()

function(_mini_add_executable target source)
    add_executable(${target} ${source})
    coroutine_study_add_ide_files(${target})
    target_include_directories(${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${COROUTINE_STUDY_EXERCISES_DIR}/include
    )
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../include")
        target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)
    endif()
endfunction()

function(_mini_sources target)
    target_sources(${target} PRIVATE ${ARGN})
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
target_sources(Capstone5_mini_corolib PRIVATE
    include/mini/as_awaitable.hpp
    include/mini/async_scope.hpp
    include/mini/generator.hpp
    include/mini/shared_task.hpp
    include/mini/single_thread_executor.hpp
    include/mini/stop_token.hpp
    include/mini/sync_wait.hpp
    include/mini/task.hpp
    include/mini/when_all.hpp
    include/mini/when_any.hpp
)

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
_mini_sources(mini_task_test
    include/mini/task.hpp
    ${COROUTINE_STUDY_EXERCISES_DIR}/include/coroutine_study/exercise_check.hpp
)
_mini_sources(mini_generator_test
    include/mini/generator.hpp
    ${COROUTINE_STUDY_EXERCISES_DIR}/include/coroutine_study/exercise_check.hpp
)
_mini_sources(mini_when_all_test
    include/mini/sync_wait.hpp
    include/mini/task.hpp
    include/mini/when_all.hpp
    ${COROUTINE_STUDY_EXERCISES_DIR}/include/coroutine_study/exercise_check.hpp
)
_mini_sources(mini_sync_wait_test
    include/mini/sync_wait.hpp
    include/mini/task.hpp
    ${COROUTINE_STUDY_EXERCISES_DIR}/include/coroutine_study/exercise_check.hpp
)
_mini_sources(mini_scope_test
    include/mini/async_scope.hpp
    include/mini/single_thread_executor.hpp
    include/mini/stop_token.hpp
    include/mini/task.hpp
    ${COROUTINE_STUDY_EXERCISES_DIR}/include/coroutine_study/exercise_check.hpp
)
_mini_sources(mini_when_any_test
    include/mini/task.hpp
    include/mini/when_any.hpp
    tests/manual_event.hpp
    ${COROUTINE_STUDY_EXERCISES_DIR}/include/coroutine_study/exercise_check.hpp
)
_mini_sources(mini_shared_task_test
    include/mini/shared_task.hpp
    include/mini/task.hpp
    tests/manual_event.hpp
    ${COROUTINE_STUDY_EXERCISES_DIR}/include/coroutine_study/exercise_check.hpp
)
_mini_sources(mini_run_loop_test
    include/mini/single_thread_executor.hpp
    include/mini/task.hpp
    ${COROUTINE_STUDY_EXERCISES_DIR}/include/coroutine_study/exercise_check.hpp
)
_mini_sources(mini_stop_test
    include/mini/stop_token.hpp
    ${COROUTINE_STUDY_EXERCISES_DIR}/include/coroutine_study/exercise_check.hpp
)
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
        coroutine_study_add_ide_files(${target})
        target_sources(${target} PRIVATE reference/include/mini_ref/mini.hpp)
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
    target_sources(mini_reference_sync_wait PRIVATE reference/tests/async_test_helpers.hpp)
    target_sources(mini_reference_when_all PRIVATE reference/tests/async_test_helpers.hpp)
    target_sources(mini_reference_when_any PRIVATE reference/tests/async_test_helpers.hpp)
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

learncpp_setup_ide("Capstone5_mini_corolib")
