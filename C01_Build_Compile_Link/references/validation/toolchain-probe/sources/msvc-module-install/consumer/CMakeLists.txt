cmake_minimum_required(VERSION 4.2)
project(msvc_module_consumer_probe LANGUAGES CXX)

find_package(arithmetic_module CONFIG REQUIRED)

add_executable(module_consumer_probe src/main.cpp)
target_compile_features(module_consumer_probe PRIVATE cxx_std_23)
target_link_libraries(module_consumer_probe PRIVATE probe::arithmetic_module)
