cmake_minimum_required(VERSION 3.28)
project(capstone1_move_url_repro LANGUAGES CXX)
add_executable(move_url_repro move_url_repro.cpp)
target_compile_features(move_url_repro PRIVATE cxx_std_20)
