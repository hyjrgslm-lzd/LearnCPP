cmake_minimum_required(VERSION 3.25)

project(c04_field_projection_sample_blind LANGUAGES CXX)

add_executable(sample_blind_field_projection
    ../../../../exercises/A02_field_projection/checks/field_projection_checks.cpp
)

target_compile_features(sample_blind_field_projection PRIVATE cxx_std_23)
target_include_directories(sample_blind_field_projection PRIVATE
    .
    ../../../../exercises/A02_field_projection/checks
    ../../../../../C01_Build_Compile_Link/exercises/include
)

add_executable(sample_blind_student_projection
    ../../../../exercises/A02_field_projection/checks/field_projection_checks.cpp
)

target_compile_features(sample_blind_student_projection PRIVATE cxx_std_23)
target_include_directories(sample_blind_student_projection PRIVATE
    ../../../../exercises/A02_field_projection/src/student
    ../../../../exercises/A02_field_projection/checks
    ../../../../../C01_Build_Compile_Link/exercises/include
)

foreach(variant IN ITEMS reference good)
    add_executable(sample_blind_unknown_schema_${variant} EXCLUDE_FROM_ALL
        unknown_schema_probe.cpp
    )
    target_compile_features(sample_blind_unknown_schema_${variant} PRIVATE cxx_std_23)
    if(variant STREQUAL "reference")
        set(impl_dir "../../../../exercises/A02_field_projection/src/reference")
    else()
        set(impl_dir "../../../../exercises/A02_field_projection/validation/good")
    endif()
    target_include_directories(sample_blind_unknown_schema_${variant} PRIVATE
        "${impl_dir}"
        ../../../../exercises/A02_field_projection/checks
        ../../../../../C01_Build_Compile_Link/exercises/include
    )
endforeach()
