cmake_minimum_required(VERSION 3.28)
project(L01_templates LANGUAGES CXX)
include(../cmake/StudySetup.cmake)

c04_add_exercise(
    NAME L01_templates
    HEADER quantity_templates.hpp
    CHECK checks/quantity_checks.cpp
    BAD_DIAGNOSTIC "different units must not be addable"
)

c04_add_observation(L01_templates_instantiation observations/instantiation.cpp)

add_executable(L01_templates_extern_template
    observations/extern_template/main.cpp
    observations/extern_template/use_a.cpp
    observations/extern_template/use_b.cpp
    observations/extern_template/twice_instantiation.cpp)
c04_configure_target(L01_templates_extern_template)
target_include_directories(L01_templates_extern_template PRIVATE observations/extern_template)
c04_add_test(L01_templates_extern_template LABELS observation)

c04_add_test(L01_templates_missing_extern_provider
    COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/observations/extern_template/check_missing_provider.py"
        --build "${CMAKE_CURRENT_BINARY_DIR}/diag/missing"
        --generator "${CMAKE_GENERATOR}" --platform "${CMAKE_GENERATOR_PLATFORM}"
        --config "$<CONFIG>" --compiler "${CMAKE_CXX_COMPILER}"
    LABELS diagnostic TIMEOUT 420)
