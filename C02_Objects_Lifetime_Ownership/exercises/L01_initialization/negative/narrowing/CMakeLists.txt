cmake_minimum_required(VERSION 3.28)
project(L01_negative_narrowing LANGUAGES CXX)
add_executable(L01_negative_narrowing main.cpp)
target_compile_features(L01_negative_narrowing PRIVATE cxx_std_23)
