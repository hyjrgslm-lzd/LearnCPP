if(NOT CASE_SOURCE OR NOT CONSUMER_SOURCE OR NOT CASE_BUILD OR NOT INSTALL_PREFIX OR NOT CONSUMER_BUILD)
    message(FATAL_ERROR "missing package consumer paths")
endif()

set(_generator_args -G "${GENERATOR}")
if(MAKE_PROGRAM)
    list(APPEND _generator_args "-DCMAKE_MAKE_PROGRAM=${MAKE_PROGRAM}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${CASE_SOURCE}" -B "${CASE_BUILD}" ${_generator_args} -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX}
    RESULT_VARIABLE _configure_result
)
if(NOT _configure_result EQUAL 0)
    message(FATAL_ERROR "module package configure failed: ${_configure_result}")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" --build "${CASE_BUILD}" --config "${CONFIG}" RESULT_VARIABLE _build_result)
if(NOT _build_result EQUAL 0)
    message(FATAL_ERROR "module package build failed: ${_build_result}")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" --install "${CASE_BUILD}" --config "${CONFIG}" RESULT_VARIABLE _install_result)
if(NOT _install_result EQUAL 0)
    message(FATAL_ERROR "module package install failed: ${_install_result}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${CONSUMER_SOURCE}" -B "${CONSUMER_BUILD}" ${_generator_args} -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=${INSTALL_PREFIX}
    RESULT_VARIABLE _consumer_configure_result
)
if(NOT _consumer_configure_result EQUAL 0)
    message(FATAL_ERROR "module consumer configure failed: ${_consumer_configure_result}")
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" --build "${CONSUMER_BUILD}" --config "${CONFIG}" RESULT_VARIABLE _consumer_build_result)
if(NOT _consumer_build_result EQUAL 0)
    message(FATAL_ERROR "module consumer build failed: ${_consumer_build_result}")
endif()

execute_process(COMMAND "${CONSUMER_BUILD}/h1_module_consumer" RESULT_VARIABLE _run_result)
if(NOT _run_result EQUAL 0)
    message(FATAL_ERROR "module consumer run failed: ${_run_result}")
endif()
