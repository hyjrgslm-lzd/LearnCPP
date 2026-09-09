cmake_minimum_required(VERSION 4.2)
project(msvc_named_modules_probe LANGUAGES CXX)

add_executable(named_modules_probe)
target_compile_features(named_modules_probe PRIVATE cxx_std_23)
target_sources(named_modules_probe
    PRIVATE
        src/main.cpp
    PRIVATE
        FILE_SET CXX_MODULES
        FILES src/arithmetic.ixx
)
