#pragma once

#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace c18_l08 {

enum class EventKind {
  main_released_gil,
  worker_started,
  worker_gil_acquired,
  python_result,
  py_refs_released,
  stop_observed,
  worker_joined,
  early_finalize_rejected,
  finalized
};

struct Event {
  EventKind kind{};
  std::thread::id thread{};
  std::string detail;
};

class EmbeddedPython {
public:
  auto transform(const std::vector<std::uint8_t>&) -> std::vector<std::uint8_t> { return {}; }
  void stop() {}
  void join() {}
  auto released_before_shutdown() const -> bool { return false; }
  auto events() const -> std::vector<Event> { return {}; }
};

}  // namespace c18_l08
