cmake_minimum_required(VERSION 3.28)
project(C07_OS_Memory_System_IO LANGUAGES CXX)
include(cmake/StudySetup.cmake)
# Calibrate the supervisor under CTest's timeout, not recursively under itself.
if(BUILD_TESTING)
    add_test(NAME C07_runner_controls COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_runner.py")
    set_tests_properties(C07_runner_controls PROPERTIES TIMEOUT 60 LABELS validation)
endif()
set(lessons L01_handles L02_sync_io L03_virtual_memory L04_mapping L05_pmr
    L06_process_ipc L07_readiness L08_completion L09_dynamic_loading P1_file_pipeline B01_costs)
foreach(lesson IN LISTS lessons)
    add_subdirectory(${lesson})
endforeach()
get_property(students GLOBAL PROPERTY C07_STUDENT_TARGETS)
add_custom_target(c07_students DEPENDS ${students})
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/student-targets-$<CONFIG>.txt" CONTENT "$<JOIN:${students},\n>\n")
