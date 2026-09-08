cmake_minimum_required(VERSION 4.2)
project(H1ModuleConsumer LANGUAGES CXX)

find_package(h1_geometry_module CONFIG REQUIRED)

add_executable(h1_module_consumer src/main.cpp)
target_compile_features(h1_module_consumer PRIVATE cxx_std_23)
target_link_libraries(h1_module_consumer PRIVATE H1::h1_geometry_module)
