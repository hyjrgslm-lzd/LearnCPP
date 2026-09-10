include_guard(GLOBAL)
set(_cs_cxx26_native_features CS_HAS_STD_SIMD CS_HAS_STD_SENDERS CS_HAS_STD_HAZARD_POINTER
    CS_HAS_STD_RCU CS_HAS_ATOMIC_MIN_MAX CS_HAS_INPLACE_STOP_TOKEN)
set(_cs_cxx29_native_features CS_HAS_STD_THREAD_ATTRIBUTES CS_HAS_STD_HAZARD_POINTER_BATCH)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/capabilities")

function(cs_probe_native_features enable_option enable_name)
    foreach(feature IN LISTS ARGN)
        set(${feature} 0 PARENT_SCOPE)
        if(NOT ${enable_option})
            file(WRITE "${CMAKE_BINARY_DIR}/capabilities/${feature}.log" "DISABLED: ${enable_name}=OFF\n")
            message(STATUS "[study] ${feature} compile/link probe: DISABLED")
        else()
            try_compile(_cs_available
                PROJECT cs_native_probe
                SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/native_probe"
                BINARY_DIR "${CMAKE_BINARY_DIR}/capabilities/${feature}"
                TARGET cs_native_probe
                CMAKE_FLAGS "-DCS_FEATURE=${feature}"
                NO_CACHE
                OUTPUT_VARIABLE _cs_probe_output)
            file(WRITE "${CMAKE_BINARY_DIR}/capabilities/${feature}.log" "${_cs_probe_output}")
            set(_cs_value 0)
            if(_cs_available)
                set(_cs_value 1)
                set(${feature} 1 PARENT_SCOPE)
            endif()
            message(STATUS "[study] ${feature} compile/link probe: ${_cs_value}")
        endif()
    endforeach()
endfunction()

cs_probe_native_features(CONCURRENCY_STUDY_ENABLE_CXX26 CONCURRENCY_STUDY_ENABLE_CXX26 ${_cs_cxx26_native_features})
cs_probe_native_features(CONCURRENCY_STUDY_ENABLE_CXX29 CONCURRENCY_STUDY_ENABLE_CXX29 ${_cs_cxx29_native_features})
