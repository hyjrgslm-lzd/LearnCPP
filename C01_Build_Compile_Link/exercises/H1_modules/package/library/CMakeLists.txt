cmake_minimum_required(VERSION 4.2)
project(H1ModulePackage LANGUAGES CXX)

add_library(h1_geometry_module)
target_compile_features(h1_geometry_module PUBLIC cxx_std_23)
target_sources(h1_geometry_module
    PUBLIC
        FILE_SET CXX_MODULES
        BASE_DIRS src
        FILES src/h1_geometry.ixx
)

install(TARGETS h1_geometry_module
    EXPORT h1_geometry_module_targets
    FILE_SET CXX_MODULES DESTINATION cxx-modules
)
install(EXPORT h1_geometry_module_targets
    NAMESPACE H1::
    DESTINATION lib/cmake/h1_geometry_module
    CXX_MODULES_DIRECTORY cxx-modules
)

include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/h1_geometry_moduleConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/h1_geometry_moduleConfig.cmake"
    INSTALL_DESTINATION lib/cmake/h1_geometry_module
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/h1_geometry_moduleConfig.cmake"
    DESTINATION lib/cmake/h1_geometry_module
)
