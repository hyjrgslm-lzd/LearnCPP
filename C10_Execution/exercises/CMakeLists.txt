cmake_minimum_required(VERSION 3.28)
project(C10_Execution LANGUAGES CXX)
include(cmake/StudySetup.cmake)
set(C10_DEFAULT_UNITS
    A1_two_executions
    A2_value_channel
    A3_fan_out_merge
    B4_scheduler_capability
    B5_context_switch
    B6_pipeline
    C1_7_error_channel
    C1_8_cancellation
    C2_9_environment
    C2_10_scope_lifetime
    D11_minimal_receiver
    D12_minimal_sender
    D13_sender_adaptor
    D14_coroutine_bridge
    E1_cpo_niebloid
    E2_tag_invoke
    E3_evolution_compare
    F1_type_list
    F2_completion_signatures
    F3_concepts
    G1_my_then
    G2_pipe_syntax
    G3_retry
    H1_run_loop
    H2_custom_query
    H3_coroutine_task
    R1_task_scope
    P2_mini_execution
    S1_usage_reading
    S2_implementation_reading
    B01_costs
    B02_bulk_domain
    F01_native
    V1_nvexec
)
if(WIN32 OR C10_STUDY_ENABLE_IO_URING)
    list(APPEND C10_DEFAULT_UNITS I1_native_io P1_pipeline)
else()
    message(STATUS "C10 native I/O NOT_ENABLED: use C10_STUDY_ENABLE_IO_URING=ON with C10_LIBURING_ROOT")
endif()
set(C10_UNITS "${C10_DEFAULT_UNITS}" CACHE STRING "Semicolon-separated course units (new build recommended after changing coverage)")
foreach(unit IN LISTS C10_UNITS)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${unit}/CMakeLists.txt")
        message(FATAL_ERROR "Unknown C10 unit: ${unit}")
    endif()
    add_subdirectory(${unit})
endforeach()
