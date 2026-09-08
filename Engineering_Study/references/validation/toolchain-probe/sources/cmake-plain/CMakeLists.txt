cmake_minimum_required(VERSION 4.2)
project(cmake_plain_probe LANGUAGES CXX)

add_executable(cmake_plain_probe src/main.cpp)
target_compile_features(cmake_plain_probe PRIVATE cxx_std_23)
