cmake_minimum_required(VERSION 3.28)

project(delegating_ctor_throw LANGUAGES CXX)

add_executable(delegating_ctor_throw main.cpp)
target_compile_features(delegating_ctor_throw PRIVATE cxx_std_23)
set_target_properties(delegating_ctor_throw PROPERTIES CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
