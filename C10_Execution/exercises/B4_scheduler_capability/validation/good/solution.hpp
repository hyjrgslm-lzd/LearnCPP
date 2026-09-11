#pragma once
#include <exec/single_thread_context.hpp>
#include <stdexec/execution.hpp>

#include <exception>
#include <latch>
#include <list>
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

struct counting_receiver {
  using receiver_concept = ex::receiver_tag;
  Probe& probe;
  std::mutex& lock;
  std::latch& completion;

  auto get_env() const noexcept { return ex::env{}; }
  void set_value() && noexcept {
    {
      std::scoped_lock guard(lock);
      probe.completed_tasks += 1;
      probe.worker_thread_ids.emplace(std::this_thread::get_id());
    }
    completion.count_down();
  }
  template <class Error>
  void set_error(Error&&) && noexcept { completion.count_down(); }
  void set_stopped() && noexcept { completion.count_down(); }
};

struct copy_receiver {
  using receiver_concept = ex::receiver_tag;
  Probe& probe;
  std::latch& completion;

  auto get_env() const noexcept { return ex::env{}; }
  void set_value() && noexcept {
    probe.scheduler_copies_remain_usable = true;
    completion.count_down();
  }
  template <class Error>
  void set_error(Error&&) && noexcept { completion.count_down(); }
  void set_stopped() && noexcept { completion.count_down(); }
};

struct context_group {
  std::vector<std::unique_ptr<exec::single_thread_context>> contexts;

  explicit context_group(int count) {
    contexts.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
      contexts.push_back(std::make_unique<exec::single_thread_context>());
    }
  }

  auto scheduler_for(int index) noexcept {
    return contexts[static_cast<std::size_t>(index) % contexts.size()]->get_scheduler();
  }
};

inline Probe run_scheduler_probe(int thread_count, int task_count) {
  constexpr int max_threads = 64;
  constexpr int max_tasks = 1024;
  if (thread_count < 1 || thread_count > max_threads) {
    throw std::invalid_argument("thread_count out of supported range");
  }
  if (task_count < 0 || task_count > max_tasks) {
    throw std::invalid_argument("task_count out of supported range");
  }

  Probe out;
  out.requested_threads = thread_count;
  out.caller_thread_id = std::this_thread::get_id();

  std::latch all_done(task_count == 0 ? 1 : task_count);
  std::latch copied_done(1);
  std::mutex seen;
  context_group group(thread_count);

  if (task_count > 0) {
    auto make_op = [&](int index) {
      return ex::connect(ex::schedule(group.scheduler_for(index)), counting_receiver{out, seen, all_done});
    };
    using op_t = decltype(make_op(0));
    std::list<std::unique_ptr<op_t>> ops;
    for (int index = 0; index < task_count; ++index) {
      ops.push_back(std::unique_ptr<op_t>(new op_t(make_op(index))));
    }
    for (auto& op : ops) {
      ex::start(*op);
    }
    all_done.wait();
  }

  auto copied_scheduler = group.scheduler_for(0);
  auto copied_op = ex::connect(ex::schedule(copied_scheduler), copy_receiver{out, copied_done});
  ex::start(copied_op);
  copied_done.wait();
  return out;
}
} // namespace c10_b4
