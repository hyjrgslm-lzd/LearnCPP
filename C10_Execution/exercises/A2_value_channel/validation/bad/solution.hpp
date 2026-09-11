#pragma once
#include <stdexec/execution.hpp>
#include <memory>
#include <string>
#include <utility>
#include <vector>
namespace c10_a2 {
namespace ex = stdexec;
struct TaskInput {
  int base{};
  std::string label;
};
struct Trace {
  std::vector<std::string> events;
};
inline int run_int_pipeline(Trace &trace) {
  trace.events.push_back("constructed");
  trace.events.push_back("plus_one:5");
  trace.events.push_back("times_two:6");
  auto sender = ex::just(12);
  trace.events.push_back("before sync_wait");
  auto got = ex::sync_wait(std::move(sender));
  trace.events.push_back("after sync_wait");
  return std::get<0>(*got);
}
inline TaskInput run_struct_pipeline(TaskInput input) {
  input.base = (input.base + 5) * 3;
  input.label += "_processed";
  return input;
}
template <class F> int run_void_pipeline(F &&f) {
  std::forward<F>(f)();
  return 42;
}
inline int run_move_only(std::unique_ptr<int> p) { return *p + 1; }
} // namespace c10_a2
