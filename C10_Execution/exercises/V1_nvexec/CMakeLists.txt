cmake_minimum_required(VERSION 3.28)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(C10_V1_nvexec LANGUAGES CXX)
    include("../cmake/StudySetup.cmake")
endif()

find_program(C10_NVEXEC_NVCXX nvc++ DOC "NVIDIA HPC SDK nvc++ compiler")
set(C10_NVEXEC_INCLUDE "${FETCHCONTENT_SOURCE_DIR_STDEXEC}/include")
set(C10_NVEXEC_STDPAR_FLAG "-stdpar=gpu")
set(C10_NVEXEC_MEMORY_FLAG "-gpu=mem:separate")
set(C10_NVEXEC_LANGUAGE_FLAGS "-x" "c++")

if(C10_NVEXEC_NVCXX)
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/nvcxx_std_probe.cu"
        "#include <iostream>\nint main(){ std::cout << __cplusplus << '\\n'; }\n")
    execute_process(
        COMMAND "${C10_NVEXEC_NVCXX}" ${C10_NVEXEC_STDPAR_FLAG} ${C10_NVEXEC_MEMORY_FLAG}
                -std=c++26 ${C10_NVEXEC_LANGUAGE_FLAGS}
                "${CMAKE_CURRENT_BINARY_DIR}/nvcxx_std_probe.cu"
                -o "${CMAKE_CURRENT_BINARY_DIR}/nvcxx_std_probe.exe"
        RESULT_VARIABLE C10_NVEXEC_CXX26_RESULT
        OUTPUT_VARIABLE C10_NVEXEC_CXX26_STDOUT
        ERROR_VARIABLE C10_NVEXEC_CXX26_STDERR)
    if(C10_NVEXEC_CXX26_RESULT EQUAL 0)
        set(C10_NVEXEC_STD_FLAG "-std=c++26")
        set(C10_NVEXEC_STD_RESULT "c++26")
    else()
        set(C10_NVEXEC_STD_FLAG "-std=c++23")
        set(C10_NVEXEC_STD_RESULT "c++23")
    endif()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/nvexec-config.txt"
        "nvcxx=${C10_NVEXEC_NVCXX}\n"
        "std_probe_result=${C10_NVEXEC_CXX26_RESULT}\n"
        "selected_standard=${C10_NVEXEC_STD_RESULT}\n"
        "compile_flags=${C10_NVEXEC_STDPAR_FLAG} ${C10_NVEXEC_MEMORY_FLAG} ${C10_NVEXEC_STD_FLAG} -x c++ -I${C10_NVEXEC_INCLUDE}\n"
        "probe_stdout=${C10_NVEXEC_CXX26_STDOUT}\n"
        "probe_stderr=${C10_NVEXEC_CXX26_STDERR}\n")

    set(single_exe "${CMAKE_CURRENT_BINARY_DIR}/pipeline_single_gpu.exe")
    set(multi_exe "${CMAKE_CURRENT_BINARY_DIR}/pipeline_multi_gpu.exe")
    set(nvcxx_flags ${C10_NVEXEC_STDPAR_FLAG} ${C10_NVEXEC_MEMORY_FLAG}
                    ${C10_NVEXEC_STD_FLAG} ${C10_NVEXEC_LANGUAGE_FLAGS}
                    "-I${C10_NVEXEC_INCLUDE}")

    add_custom_command(OUTPUT "${single_exe}"
        COMMAND "${C10_NVEXEC_NVCXX}" ${nvcxx_flags}
                "${CMAKE_CURRENT_SOURCE_DIR}/pipeline_single_gpu.cu" -o "${single_exe}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline_single_gpu.cu"
        VERBATIM)
    add_custom_command(OUTPUT "${multi_exe}"
        COMMAND "${C10_NVEXEC_NVCXX}" ${nvcxx_flags}
                "${CMAKE_CURRENT_SOURCE_DIR}/pipeline_multi_gpu.cu" -o "${multi_exe}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/pipeline_multi_gpu.cu"
        VERBATIM)
    add_custom_target(V1_nvexec_single_gpu_build ALL DEPENDS "${single_exe}")
    add_custom_target(V1_nvexec_multi_gpu_build ALL DEPENDS "${multi_exe}")

    c10_add_test(V1_nvexec_single_gpu COMMAND "${single_exe}" LABELS nvexec gpu)
    c10_add_test(V1_nvexec_multi_gpu COMMAND "${multi_exe}" LABELS nvexec gpu ALLOW_SKIP)
    c10_record_unit(V1_nvexec_single_gpu observation none pipeline_single_gpu.cu none none)
    c10_record_unit(V1_nvexec_multi_gpu observation none pipeline_multi_gpu.cu none none)
else()
    c10_add_observation(V1_nvexec_single_gpu skip_single.cpp ALLOW_SKIP LABELS nvexec gpu)
    c10_add_observation(V1_nvexec_multi_gpu skip_multi.cpp ALLOW_SKIP LABELS nvexec gpu)
endif()
