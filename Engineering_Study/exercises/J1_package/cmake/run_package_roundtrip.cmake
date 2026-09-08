if(NOT DEFINED SOURCE_DIR OR NOT DEFINED BINARY_DIR OR NOT DEFINED CONFIG)
    message(FATAL_ERROR "SOURCE_DIR, BINARY_DIR and CONFIG are required")
endif()

function(make_generator_args out_var)
    set(args)
    if(DEFINED GENERATOR AND NOT GENERATOR STREQUAL "")
        list(APPEND args -G "${GENERATOR}")
    endif()
    if(DEFINED PLATFORM AND NOT PLATFORM STREQUAL "")
        list(APPEND args -A "${PLATFORM}")
    endif()
    if(DEFINED INSTANCE AND NOT INSTANCE STREQUAL "")
        list(APPEND args -DCMAKE_GENERATOR_INSTANCE=${INSTANCE})
    endif()
    if(DEFINED MAKE_PROGRAM AND NOT MAKE_PROGRAM STREQUAL "" AND DEFINED GENERATOR AND GENERATOR MATCHES "Ninja")
        list(APPEND args -DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM})
    endif()
    set(${out_var} "${args}" PARENT_SCOPE)
endfunction()

function(run_checked label)
    cmake_parse_arguments(ARG "" "WORKING_DIRECTORY" "COMMAND" ${ARGN})
    execute_process(
        COMMAND ${ARG_COMMAND}
        WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err
    )
    if(NOT result EQUAL 0)
        message(FATAL_ERROR "${label} failed:\n${out}\n${err}")
    endif()
    message(STATUS "${label} passed")
endfunction()

function(expect_failure label expect_regex)
    cmake_parse_arguments(ARG "" "WORKING_DIRECTORY" "COMMAND" ${ARGN})
    execute_process(
        COMMAND ${ARG_COMMAND}
        WORKING_DIRECTORY "${ARG_WORKING_DIRECTORY}"
        RESULT_VARIABLE result
        OUTPUT_VARIABLE out
        ERROR_VARIABLE err
    )
    set(combined "${out}\n${err}")
    if(result EQUAL 0)
        message(FATAL_ERROR "${label} unexpectedly succeeded")
    endif()
    if(NOT combined MATCHES "${expect_regex}")
        message(FATAL_ERROR "${label} failed for the wrong reason:\n${combined}")
    endif()
    message(STATUS "${label} failed as expected")
endfunction()

function(find_consumer_exe out_var build_dir)
    file(GLOB_RECURSE candidates "${build_dir}/package_consumer.exe" "${build_dir}/package_consumer")
    list(LENGTH candidates count)
    if(NOT count EQUAL 1)
        message(FATAL_ERROR "expected one package_consumer executable in ${build_dir}, found ${count}: ${candidates}")
    endif()
    list(GET candidates 0 exe)
    set(${out_var} "${exe}" PARENT_SCOPE)
endfunction()

file(REAL_PATH "${BINARY_DIR}" binary_root)
set(stage_root "${binary_root}/stage")
file(MAKE_DIRECTORY "${stage_root}")
string(FIND "${stage_root}" "${binary_root}" stage_pos)
if(NOT stage_pos EQUAL 0)
    message(FATAL_ERROR "stage root is outside this build tree: ${stage_root}")
endif()

set(prefix_a "${stage_root}/${CONFIG}/prefix_A")
set(prefix_b "${stage_root}/${CONFIG}/prefix_B")
set(prefix_missing "${stage_root}/${CONFIG}/prefix_missing")
set(consumer_static_build "${stage_root}/${CONFIG}/consumer_static_build")
set(consumer_shared_build "${stage_root}/${CONFIG}/consumer_shared_build")
set(consumer_version_build "${stage_root}/${CONFIG}/consumer_version_build")
set(consumer_missing_target_build "${stage_root}/${CONFIG}/consumer_missing_target_build")
set(missing_dll_run_dir "${stage_root}/${CONFIG}/missing_dll_run")

foreach(path IN ITEMS "${prefix_a}" "${prefix_b}" "${prefix_missing}" "${consumer_static_build}" "${consumer_shared_build}" "${consumer_version_build}" "${consumer_missing_target_build}" "${missing_dll_run_dir}")
    get_filename_component(real_path "${path}" ABSOLUTE BASE_DIR "${binary_root}")
    string(FIND "${real_path}" "${stage_root}" path_pos)
    if(NOT path_pos EQUAL 0)
        message(FATAL_ERROR "refusing to delete path outside stage: ${real_path}")
    endif()
    file(REMOVE_RECURSE "${real_path}")
endforeach()
file(MAKE_DIRECTORY "${stage_root}/${CONFIG}")

run_checked("install package"
    COMMAND "${CMAKE_COMMAND}" --install "${BINARY_DIR}" --config "${CONFIG}" --prefix "${prefix_a}"
)
file(COPY "${prefix_a}/" DESTINATION "${prefix_b}")
file(COPY "${prefix_a}/" DESTINATION "${prefix_missing}")

file(GLOB_RECURSE exported_files
    "${prefix_b}/lib/cmake/LessonPackage/*.cmake"
)
set(exported_text "")
foreach(file IN LISTS exported_files)
    file(READ "${file}" one_file)
    string(APPEND exported_text "\n${one_file}")
endforeach()
file(REAL_PATH "${SOURCE_DIR}" source_real)
file(REAL_PATH "${BINARY_DIR}" build_real)
if(exported_text MATCHES "${source_real}" OR exported_text MATCHES "${build_real}")
    message(FATAL_ERROR "exported package contains source/build path")
endif()
if(NOT exported_text MATCHES "INTERFACE_COMPILE_DEFINITIONS[^\\n]*LESSON_STATIC")
    message(FATAL_ERROR "static exported target does not propagate LESSON_STATIC")
endif()
if(NOT exported_text MATCHES "INTERFACE_INCLUDE_DIRECTORIES")
    message(FATAL_ERROR "exported targets do not propagate include directories")
endif()

make_generator_args(generator_args)
set(common_package_args
    -DCMAKE_PREFIX_PATH=${prefix_b}
    -DCMAKE_BUILD_TYPE=${CONFIG}
    -DCMAKE_FIND_PACKAGE_NO_PACKAGE_REGISTRY=ON
    -DCMAKE_FIND_PACKAGE_NO_SYSTEM_PACKAGE_REGISTRY=ON
    -DCMAKE_FIND_USE_PACKAGE_REGISTRY=FALSE
    -DCMAKE_FIND_USE_SYSTEM_PACKAGE_REGISTRY=FALSE
)

run_checked("configure static consumer"
    COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}/consumer" -B "${consumer_static_build}" ${generator_args} ${common_package_args} -DLESSON_TARGET=lesson_static -DREQUESTED_VERSION=1.0.0
)
run_checked("build static consumer"
    COMMAND "${CMAKE_COMMAND}" --build "${consumer_static_build}" --config "${CONFIG}"
)
find_consumer_exe(static_consumer_exe "${consumer_static_build}")
run_checked("run static consumer"
    COMMAND "${static_consumer_exe}"
)

run_checked("configure shared consumer"
    COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}/consumer" -B "${consumer_shared_build}" ${generator_args} ${common_package_args} -DLESSON_TARGET=lesson_shared -DREQUESTED_VERSION=1.0.0
)
run_checked("build shared consumer"
    COMMAND "${CMAKE_COMMAND}" --build "${consumer_shared_build}" --config "${CONFIG}"
)
find_consumer_exe(shared_consumer_exe "${consumer_shared_build}")
if(WIN32)
    run_checked("run shared consumer"
        COMMAND "${CMAKE_COMMAND}" -E env "PATH=${prefix_b}/bin" "${shared_consumer_exe}"
    )
else()
    run_checked("run shared consumer"
        COMMAND "${CMAKE_COMMAND}" -E env "LD_LIBRARY_PATH=${prefix_b}/lib:$ENV{LD_LIBRARY_PATH}" "${shared_consumer_exe}"
    )
endif()

expect_failure("reject version 2.0.0" "LessonPackage|version|2\\.0\\.0"
    COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}/consumer" -B "${consumer_version_build}" ${generator_args} ${common_package_args} -DLESSON_TARGET=lesson_static -DREQUESTED_VERSION=2.0.0
)
expect_failure("reject missing exported target" "LessonPackage::lesson"
    COMMAND "${CMAKE_COMMAND}" -S "${SOURCE_DIR}/consumer" -B "${consumer_missing_target_build}" ${generator_args} ${common_package_args} -DLESSON_TARGET=missing -DREQUESTED_VERSION=1.0.0
)

if(WIN32)
    file(GLOB dlls "${prefix_missing}/bin/*.dll")
    foreach(dll IN LISTS dlls)
        file(REAL_PATH "${dll}" dll_real)
        string(FIND "${dll_real}" "${prefix_missing}" dll_pos)
        if(NOT dll_pos EQUAL 0)
            message(FATAL_ERROR "refusing to remove DLL outside missing prefix: ${dll_real}")
        endif()
        file(REMOVE "${dll_real}")
    endforeach()
    file(MAKE_DIRECTORY "${missing_dll_run_dir}")
    get_filename_component(shared_consumer_name "${shared_consumer_exe}" NAME)
    set(missing_dll_exe "${missing_dll_run_dir}/${shared_consumer_name}")
    file(COPY_FILE "${shared_consumer_exe}" "${missing_dll_exe}" ONLY_IF_DIFFERENT)
    expect_failure("missing DLL run" "dll|DLL|library|module|程序|启动|0xc0000135|STATUS_DLL_NOT_FOUND"
        WORKING_DIRECTORY "${missing_dll_run_dir}"
        COMMAND "${CMAKE_COMMAND}" -E env "PATH=${prefix_missing}/bin" "${missing_dll_exe}"
    )
endif()
