cmake_minimum_required(VERSION 3.28)

project(l07_move_assign_order LANGUAGES CXX)

add_executable(l07_move_assign_order main.cpp)
target_compile_features(l07_move_assign_order PRIVATE cxx_std_23)
set_target_properties(l07_move_assign_order PROPERTIES CXX_EXTENSIONS OFF CXX_SCAN_FOR_MODULES OFF)
target_include_directories(l07_move_assign_order PRIVATE
    ../../../../../exercises/L07_raii/src/reference
)
