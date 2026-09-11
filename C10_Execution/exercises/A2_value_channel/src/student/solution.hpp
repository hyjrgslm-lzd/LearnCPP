#pragma once
#include <c10/test.hpp>
#include <memory>
#include <string>
#include <vector>
namespace c10_a2 {
struct TaskInput {
  int base{};
  std::string label;
};
struct Trace {
  std::vector<std::string> events;
};
inline int run_int_pipeline(Trace &) { throw c10::unfinished("A2: implement run_int_pipeline"); }
inline TaskInput run_struct_pipeline(TaskInput) {
  throw c10::unfinished("A2: implement run_struct_pipeline");
}
template <class F> int run_void_pipeline(F &&) {
  throw c10::unfinished("A2: implement run_void_pipeline");
}
inline int run_move_only(std::unique_ptr<int>) {
  throw c10::unfinished("A2: implement run_move_only");
}
} // namespace c10_a2
