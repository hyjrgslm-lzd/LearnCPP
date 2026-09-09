cmake_minimum_required(VERSION 3.28)
project(probe_l09_fake LANGUAGES CXX)
add_executable(probe_l09_fake "F:/CPPTrain/LearnCPP/Core_Study/exercises/L09_shared/checks/validation_check.cpp")
target_compile_features(probe_l09_fake PRIVATE cxx_std_23)
target_include_directories(probe_l09_fake PRIVATE
  "F:/CPPTrain/LearnCPP/Core_Study/exercises/L09_shared/checks"
  "F:/CPPTrain/LearnCPP/Core_Study/exercises/L09_shared/checks/support"
  "F:/CPPTrain/LearnCPP/Core_Study/references/validation/reviews/c02-independent-evidence/owner-08-10-r1/probe-l09-fake-results-r1b/fake"
  "F:/CPPTrain/LearnCPP/Engineering_Study/exercises/include"
)
if(MSVC)
  target_compile_options(probe_l09_fake PRIVATE /permissive- /W4 /utf-8)
endif()
