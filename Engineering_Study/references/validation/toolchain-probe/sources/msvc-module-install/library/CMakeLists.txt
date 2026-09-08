cmake_minimum_required(VERSION 4.2)
project(msvc_module_install_probe LANGUAGES CXX)

add_library(arithmetic_module)
target_compile_features(arithmetic_module PUBLIC cxx_std_23)
target_sources(arithmetic_module
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS src
        FILES src/arithmetic_package.ixx
)

install(TARGETS arithmetic_module
    EXPORT arithmetic_module_targets
    FILE_SET CXX_MODULES DESTINATION cxx-modules
)

install(EXPORT arithmetic_module_targets
    NAMESPACE probe::
    DESTINATION lib/cmake/arithmetic_module
    CXX_MODULES_DIRECTORY cxx-modules
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/arithmetic_moduleConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/arithmetic_moduleConfig.cmake"
    INSTALL_DESTINATION lib/cmake/arithmetic_module
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/arithmetic_moduleConfig.cmake"
    DESTINATION lib/cmake/arithmetic_module
)
