if(NOT DEFINED EXE OR NOT DEFINED WORK_DIR OR NOT DEFINED BINARY_DIR)
    message(FATAL_ERROR "EXE, WORK_DIR and BINARY_DIR are required")
endif()

file(REAL_PATH "${BINARY_DIR}" _binary_dir)
get_filename_component(_work_dir "${WORK_DIR}" ABSOLUTE BASE_DIR "${_binary_dir}")
string(FIND "${_work_dir}" "${_binary_dir}" _work_pos)
if(NOT _work_pos EQUAL 0)
    message(FATAL_ERROR "refusing to delete path outside this build tree: ${_work_dir}")
endif()
file(REAL_PATH "${EXE}" _exe)
get_filename_component(_exe_ext "${_exe}" EXT)
file(REMOVE_RECURSE "${_work_dir}")
file(MAKE_DIRECTORY "${_work_dir}")
file(COPY_FILE "${_exe}" "${_work_dir}/missing_dll_check${_exe_ext}" ONLY_IF_DIFFERENT)

execute_process(
    COMMAND "${_work_dir}/missing_dll_check${_exe_ext}"
    WORKING_DIRECTORY "${_work_dir}"
    RESULT_VARIABLE run_result
    OUTPUT_VARIABLE run_out
    ERROR_VARIABLE run_err
)

if(run_result EQUAL 0)
    message(FATAL_ERROR "missing DLL case unexpectedly ran successfully:\n${run_out}\n${run_err}")
endif()

set(_combined "${run_out}\n${run_err}")
if(_combined MATCHES "linked check passed")
    message(FATAL_ERROR "missing DLL case reached the linked program logic:\n${_combined}")
endif()

message(STATUS "missing DLL case failed before linked program logic as expected")
