cmake_minimum_required(VERSION 3.28)
project(A04_type_pipelines LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(NAME A04_type_pipelines HEADER type_pipelines.hpp CHECK checks/type_pipeline_checks.cpp
    BAD_DIAGNOSTIC "error/stopped and throwing value channels survive")
c04_add_observation(A04_eager_counterexample observations/eager_counterexample.cpp)

c04_add_compile_case(NAME A04_diag_zip SOURCE validation/diagnostics/zip_mismatch.cpp
    CONTROL validation/diagnostics/control.cpp PATTERN "invalid use of incomplete type|undefined template|implicit instantiation of undefined template|使用了未定义类型.*zip|未能使别名模板"
    SETUP validation/diagnostics/setup.cmake)
c04_add_compile_case(NAME A04_diag_flatten SOURCE validation/diagnostics/flatten_non_list.cpp
    CONTROL validation/diagnostics/control.cpp PATTERN "invalid use of incomplete type|undefined template|implicit instantiation of undefined template|使用了未定义类型.*flatten|未能使别名模板"
    SETUP validation/diagnostics/setup.cmake)
c04_add_compile_case(NAME A04_diag_eager SOURCE validation/diagnostics/eager_lazy.cpp
    CONTROL validation/diagnostics/control.cpp PATTERN "Explosive|type"
    SETUP validation/diagnostics/setup.cmake)
