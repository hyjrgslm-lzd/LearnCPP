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
  auto make = [&] {
    trace.events.push_back("constructed");
    return ex::then(ex::then(ex::just(5),
                             [&](int v) {
                               trace.events.push_back("plus_one:" + std::to_string(v));
                               return v + 1;
                             }),
                    [&](int v) {
                      trace.events.push_back("times_two:" + std::to_string(v));
                      return v * 2;
                    });
  };
  auto sender = make();
  trace.events.push_back("before sync_wait");
  auto got = ex::sync_wait(std::move(sender));
  trace.events.push_back("after sync_wait");
  return std::get<0>(*got);
}
inline TaskInput run_struct_pipeline(TaskInput input) {
  auto first = [](TaskInput t) { return TaskInput{t.base + 5, t.label + "_processed"}; };
  auto second = [](TaskInput t) { return TaskInput{t.base * 3, std::move(t.label)}; };
  auto got = ex::sync_wait(ex::then(ex::then(ex::just(std::move(input)), first), second));
  return std::move(std::get<0>(*got));
}
template <class F> int run_void_pipeline(F &&f) {
  auto got = ex::sync_wait(
      ex::then(ex::then(ex::just(), [&] { std::forward<F>(f)(); }), [] { return 42; }));
  return std::get<0>(*got);
}
inline int run_move_only(std::unique_ptr<int> p) {
  auto got = ex::sync_wait(ex::then(ex::just(std::move(p)), [](auto q) { return *q + 1; }));
  return std::get<0>(*got);
}
} // namespace c10_a2
