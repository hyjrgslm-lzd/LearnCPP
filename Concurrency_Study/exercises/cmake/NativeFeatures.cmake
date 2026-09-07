include_guard(GLOBAL)
set(_cs_native_features CS_HAS_STD_SIMD CS_HAS_STD_SENDERS CS_HAS_STD_HAZARD_POINTER
    CS_HAS_STD_RCU CS_HAS_ATOMIC_MIN_MAX CS_HAS_INPLACE_STOP_TOKEN)
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/capabilities")
foreach(feature IN LISTS _cs_native_features)
    set(${feature} 0)
    if(CONCURRENCY_STUDY_ENABLE_CXX26)
        try_compile(_cs_available
            PROJECT cs_native_probe
            SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/native_probe"
            BINARY_DIR "${CMAKE_BINARY_DIR}/capabilities/${feature}"
            TARGET cs_native_probe
            CMAKE_FLAGS "-DCS_FEATURE=${feature}"
            NO_CACHE
            OUTPUT_VARIABLE _cs_probe_output)
        file(WRITE "${CMAKE_BINARY_DIR}/capabilities/${feature}.log" "${_cs_probe_output}")
        if(_cs_available)
            set(${feature} 1)
        endif()
        message(STATUS "[study] ${feature} compile/link probe: ${${feature}}")
    endif()
endforeach()
