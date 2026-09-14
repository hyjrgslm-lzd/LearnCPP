#pragma once

#include <thread>
#include <vector>

namespace c18_lua_l14 {

struct CoroutineResult {
  std::vector<int> yielded;
  bool queued_from_worker{};
  bool resumed_on_foreign_thread{};
  bool continuation_seen{};
  std::thread::id owner_thread{};
  std::thread::id worker_thread{};
  std::thread::id last_resume_thread{};
  int final_status{};
};

CoroutineResult run_coroutine();

}
