#pragma once
#include <exec/single_thread_context.hpp>
#include <stdexec/execution.hpp>

#include <memory>
#include <mutex>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

namespace c10_b4 {
namespace ex = stdexec;

struct Probe {
  int requested_threads{};
  int completed_tasks{};
  std::thread::id caller_thread_id{};
  std::set<std::thread::id> worker_thread_ids;
  bool scheduler_copies_remain_usable{};
};

inline Probe run_scheduler_probe(int thread_count, int task_count) {
  if (thread_count < 1 || thread_count > 64) {
    throw std::invalid_argument("thread_count out of range");
  }
  if (task_count < 0 || task_count > 1024) {
    throw std::invalid_argument("task_count out of range");
  }

  Probe probe;
  probe.requested_threads = thread_count;
  probe.caller_thread_id = std::this_thread::get_id();

  std::mutex mutex;
  std::vector<std::unique_ptr<exec::single_thread_context>> contexts;
  contexts.reserve(static_cast<std::size_t>(thread_count));
  for (int i = 0; i < thread_count; ++i) {
    contexts.push_back(std::make_unique<exec::single_thread_context>());
  }

  for (int i = 0; i < task_count; ++i) {
    auto scheduler = contexts[static_cast<std::size_t>(i % thread_count)]->get_scheduler();
    ex::sync_wait(ex::schedule(scheduler) | ex::then([&] {
                    std::lock_guard lock(mutex);
                    ++probe.completed_tasks;
                    probe.worker_thread_ids.insert(std::this_thread::get_id());
                  }));
  }

  auto copy = contexts.front()->get_scheduler();
  ex::sync_wait(ex::schedule(copy) |
                ex::then([&] { probe.scheduler_copies_remain_usable = true; }));
  return probe;
}
} // namespace c10_b4
