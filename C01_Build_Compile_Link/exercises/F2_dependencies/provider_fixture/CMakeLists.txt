cmake_minimum_required(VERSION 3.28)
project(F2Provider VERSION 1.0.0 LANGUAGES CXX)

add_library(f2_provider_fixture src/provider.cpp)
add_library(F2Provider::provider ALIAS f2_provider_fixture)

target_include_directories(f2_provider_fixture
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_compile_definitions(f2_provider_fixture PUBLIC F2_PROVIDER_VERSION=100)
target_compile_features(f2_provider_fixture PUBLIC cxx_std_23)
set_target_properties(f2_provider_fixture PROPERTIES
    CXX_EXTENSIONS OFF
    EXPORT_NAME provider
)

include(CMakePackageConfigHelpers)

install(TARGETS f2_provider_fixture
    EXPORT F2ProviderTargets
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)
install(DIRECTORY include/ DESTINATION include)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/F2ProviderConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)
configure_package_config_file(
    "${CMAKE_CURRENT_LIST_DIR}/cmake/F2ProviderConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/F2ProviderConfig.cmake"
    INSTALL_DESTINATION lib/cmake/F2Provider
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/F2ProviderConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/F2ProviderConfigVersion.cmake"
    DESTINATION lib/cmake/F2Provider
)
install(EXPORT F2ProviderTargets
    NAMESPACE F2Provider::
    DESTINATION lib/cmake/F2Provider
)
