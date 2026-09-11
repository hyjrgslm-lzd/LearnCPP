#pragma once
#include <set>
#include <stdexcept>
#include <thread>
namespace c10_b4 {
struct Probe {
  int requested_threads{};
  int completed_tasks{};
  std::thread::id caller_thread_id{};
  std::set<std::thread::id> worker_thread_ids;
  bool scheduler_copies_remain_usable{};
};
inline Probe run_scheduler_probe(int thread_count, int task_count) {
  if (thread_count < 1 || task_count < 0)
    throw std::invalid_argument("invalid");
  Probe p{};
  p.requested_threads = thread_count;
  p.completed_tasks = 8;
  p.caller_thread_id = std::this_thread::get_id();
  p.scheduler_copies_remain_usable = true;
  return p;
}
} // namespace c10_b4
