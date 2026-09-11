#pragma once
#include <c10/test.hpp>
#include <set>
#include <thread>
namespace c10_b4 {
struct Probe {
  int requested_threads{};
  int completed_tasks{};
  std::thread::id caller_thread_id{};
  std::set<std::thread::id> worker_thread_ids;
  bool scheduler_copies_remain_usable{};
};
inline Probe run_scheduler_probe(int, int) {
  throw c10::unfinished("B4: implement run_scheduler_probe");
}
} // namespace c10_b4
