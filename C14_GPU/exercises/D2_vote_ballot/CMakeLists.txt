cmake_minimum_required(VERSION 3.28)
project(D2_vote_ballot LANGUAGES CXX CUDA)

add_executable(D2_vote_ballot main.cu)
target_link_libraries(D2_vote_ballot PRIVATE gpu_study_common)
set_target_properties(D2_vote_ballot PROPERTIES
    CUDA_SEPARABLE_COMPILATION ON)
