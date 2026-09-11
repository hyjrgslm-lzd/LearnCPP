coroutine_study_add_exercise(F3_halo_diagnose REFERENCE_SOURCES solution.cpp)

if(BUILD_TESTING)
    add_test(NAME F3_halo_contract COMMAND F3_halo_diagnose)
    set_tests_properties(F3_halo_contract PROPERTIES LABELS "observation;contract" TIMEOUT 30)
endif()

target_compile_options(F3_halo_diagnose PRIVATE
    $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/Zc:__cplusplus;/utf-8;/O2>
    $<$<AND:$<NOT:$<CXX_COMPILER_ID:MSVC>>,$<CONFIG:Release>>:-O2>
)
if(TARGET F3_halo_diagnose_reference)
    target_compile_options(F3_halo_diagnose_reference PRIVATE
        $<$<AND:$<CXX_COMPILER_ID:MSVC>,$<CONFIG:Release>>:/Zc:__cplusplus;/utf-8;/O2>
        $<$<AND:$<NOT:$<CXX_COMPILER_ID:MSVC>>,$<CONFIG:Release>>:-O2>
    )
endif()
