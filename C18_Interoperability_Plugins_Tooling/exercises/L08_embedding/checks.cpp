#include "solution.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace {
int fail(std::string_view message) {
  std::cout << "check failed: " << message << '\n';
  return 1;
}

auto find_event(const std::vector<c18_l08::Event>& events, c18_l08::EventKind kind) -> std::size_t {
  for (std::size_t i = 0; i < events.size(); ++i) {
    if (events[i].kind == kind) {
      return i;
    }
  }
  return events.size();
}

auto has_event(const std::vector<c18_l08::Event>& events, c18_l08::EventKind kind) -> bool {
  return find_event(events, kind) != events.size();
}
}  // namespace

int main() {
  const auto main_thread = std::this_thread::get_id();
  c18_l08::EmbeddedPython worker;
  const std::vector<std::uint8_t> input{'a', 'z', 'A', 0, 0xffu, 'm'};
  const std::vector<std::uint8_t> expected{'A', 'Z', 'A', 0, 0xffu, 'M'};
  const auto output = worker.transform(input);
  if (output != expected) {
    return fail("embedded Python broke the byte contract");
  }

  worker.stop();
  worker.join();
  const auto events = worker.events();
  if (has_event(events, c18_l08::EventKind::early_finalize_rejected)) {
    return fail("finalize was attempted before joining the Python worker");
  }

  const auto released_main_gil = find_event(events, c18_l08::EventKind::main_released_gil);
  const auto started = find_event(events, c18_l08::EventKind::worker_started);
  const auto acquired_gil = find_event(events, c18_l08::EventKind::worker_gil_acquired);
  const auto python_result = find_event(events, c18_l08::EventKind::python_result);
  const auto refs_released = find_event(events, c18_l08::EventKind::py_refs_released);
  const auto stop_observed = find_event(events, c18_l08::EventKind::stop_observed);
  const auto joined = find_event(events, c18_l08::EventKind::worker_joined);
  const auto finalized = find_event(events, c18_l08::EventKind::finalized);
  if (finalized == events.size() || joined == events.size() || refs_released == events.size() ||
      python_result == events.size() || acquired_gil == events.size() || started == events.size() ||
      released_main_gil == events.size() || stop_observed == events.size()) {
    return fail("embedding trace is missing a required real event");
  }
  if (events[started].thread == main_thread || events[acquired_gil].thread == main_thread ||
      events[python_result].thread == main_thread) {
    return fail("Python work did not run on the worker thread");
  }
  if (events[python_result].detail != std::string(reinterpret_cast<const char*>(expected.data()), expected.size())) {
    return fail("trace did not record the actual Python byte result");
  }
  if (!(released_main_gil < acquired_gil && acquired_gil < python_result && python_result < refs_released &&
        stop_observed < joined && refs_released < joined && joined < finalized)) {
    return fail("embedding shutdown order is not worker refs -> join -> finalize");
  }
  std::cout << "L08 embedding PASS\n";
  return 0;
}
