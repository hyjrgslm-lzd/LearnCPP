cmake_minimum_required(VERSION 3.28)
project(l09_old_fake_repro LANGUAGES CXX)
add_executable(l09_old_fake_repro "F:/CPPTrain/LearnCPP/Core_Study/exercises/L09_shared/checks/validation_check.cpp")
target_compile_features(l09_old_fake_repro PRIVATE cxx_std_23)
target_include_directories(l09_old_fake_repro PRIVATE
  "F:/CPPTrain/LearnCPP/Core_Study/exercises/L09_shared/checks"
  "F:/CPPTrain/LearnCPP/Core_Study/exercises/L09_shared/checks/support"
  "F:/CPPTrain/LearnCPP/Core_Study/references/validation/reviews/c02-independent-evidence/owner-08-10-r1/probe-l09-fake-results-r1b/fake"
  "F:/CPPTrain/LearnCPP/Engineering_Study/exercises/include"
)
if(MSVC)
  target_compile_options(l09_old_fake_repro PRIVATE /permissive- /W4 /utf-8)
endif()
