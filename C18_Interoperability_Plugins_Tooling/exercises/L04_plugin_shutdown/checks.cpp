#include "solution.hpp"

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <string_view>
#include <thread>
#include <vector>

namespace {

auto fail(std::string_view message) -> int {
  std::cout << "check failed: " << message << '\n';
  return 1;
}

auto expect(c18_status actual, c18_status expected, std::string_view label) -> int {
  if (actual != expected) {
    std::cout << "check failed: " << label << " expected " << static_cast<int>(expected) << " got "
              << static_cast<int>(actual) << '\n';
    return 1;
  }
  return 0;
}

auto bytes(std::initializer_list<uint8_t> values) -> std::vector<uint8_t> {
  return std::vector<uint8_t>(values);
}

template <class Predicate>
auto wait_for_event(std::condition_variable& cv, std::unique_lock<std::mutex>& lock, Predicate predicate) -> bool {
  return cv.wait_for(lock, std::chrono::seconds(10), predicate);
}

auto check_basic_contracts(const std::filesystem::path& good_plugin,
                           const std::filesystem::path& missing_symbol,
                           const std::filesystem::path& version_mismatch,
                           const std::filesystem::path& throwing_entry) -> int {
  {
    c18_l04::PluginSession session;
    if (auto rc = expect(session.open(missing_symbol), C18_STATUS_MISSING_SYMBOL, "missing symbol")) {
      return rc;
    }
  }
  {
    c18_l04::PluginSession session;
    if (auto rc = expect(session.open(version_mismatch), C18_STATUS_UNSUPPORTED_VERSION, "version mismatch")) {
      return rc;
    }
  }
  {
    c18_l04::PluginSession session;
    if (auto rc = expect(session.open(throwing_entry), C18_STATUS_PLUGIN_ERROR, "throwing entry")) {
      return rc;
    }
  }

  c18_l04::PluginSession session;
  if (auto rc = expect(session.open(good_plugin), C18_STATUS_OK, "open good plugin")) {
    return rc;
  }

  const auto input = bytes({'a', 'z', 'A', 0, 0xffu, '!', 'm'});
  auto transformed = session.process(input, input.size());
  if (auto rc = expect(transformed.status, C18_STATUS_OK, "transform status")) {
    return rc;
  }
  if (transformed.required != input.size()) {
    return fail("written must equal input size");
  }
  if (transformed.bytes != bytes({'A', 'Z', 'A', 0, 0xffu, '!', 'M'})) {
    return fail("ASCII transform changed the wrong bytes");
  }

  auto empty = session.process({}, 0);
  if (auto rc = expect(empty.status, C18_STATUS_OK, "empty input")) {
    return rc;
  }
  if (!empty.bytes.empty() || empty.required != 0) {
    return fail("empty input must write zero bytes");
  }

  auto small = session.process(input, input.size() - 1);
  if (auto rc = expect(small.status, C18_STATUS_BUFFER_TOO_SMALL, "small output")) {
    return rc;
  }
  if (small.required != input.size()) {
    return fail("small output must report required capacity");
  }

  bool callback_saw_unlocked_session = false;
  bool callback_requested_stop = false;
  session.set_callback([&](std::string_view event_name) {
    if (event_name == "process:bang") {
      callback_saw_unlocked_session = session.try_lock_probe();
      callback_requested_stop = session.request_stop() == C18_STATUS_OK;
    }
  });

  auto stopped = session.process(bytes({'x', '!'}), 2);
  if (auto rc = expect(stopped.status, C18_STATUS_OK, "callback-triggering process")) {
    return rc;
  }
  if (!callback_saw_unlocked_session) {
    return fail("callback invoked while holding the session lock");
  }
  if (!callback_requested_stop) {
    return fail("callback could not request stop");
  }
  auto after_stop = session.process(bytes({'a'}), 1);
  if (auto rc = expect(after_stop.status, C18_STATUS_CLOSING, "process after closing")) {
    return rc;
  }
  return expect(session.close(), C18_STATUS_OK, "close");
}

auto check_controlled_drain(const std::filesystem::path& good_plugin) -> int {
  c18_l04::PluginSession session;
  if (auto rc = expect(session.open(good_plugin), C18_STATUS_OK, "open controlled plugin")) {
    return rc;
  }

  std::mutex mutex;
  std::condition_variable cv;
  bool callback_entered = false;
  bool callback_released = false;
  bool close_waiting = false;
  bool close_done = false;
  bool stop_called = false;
  bool destroy_called = false;
  bool unlocked_in_callback = false;
  c18_l04::ProcessResult process_result;
  c18_status close_status = C18_STATUS_PLUGIN_ERROR;

  session.set_observer([&](std::string_view event_name) {
    if (event_name == "close:waiting") {
      std::scoped_lock guard(mutex);
      close_waiting = true;
      cv.notify_all();
    }
  });

  session.set_callback([&](std::string_view event_name) {
    if (event_name == "stop:called") {
      std::scoped_lock guard(mutex);
      stop_called = true;
      cv.notify_all();
      return;
    }
    if (event_name == "destroy:called") {
      std::scoped_lock guard(mutex);
      destroy_called = true;
      cv.notify_all();
      return;
    }
    if (event_name != "process:bang") {
      return;
    }

    std::unique_lock guard(mutex);
    unlocked_in_callback = session.try_lock_probe();
    callback_entered = true;
    cv.notify_all();
    cv.wait(guard, [&] { return callback_released; });
  });

  std::thread process_thread([&] { process_result = session.process(bytes({'a', '!'}), 2); });

  {
    std::unique_lock guard(mutex);
    if (!wait_for_event(cv, guard, [&] { return callback_entered; })) {
      callback_released = true;
      cv.notify_all();
      guard.unlock();
      process_thread.join();
      return fail("callback did not enter controlled barrier");
    }
  }

  if (!unlocked_in_callback) {
    {
      std::scoped_lock guard(mutex);
      callback_released = true;
    }
    cv.notify_all();
    process_thread.join();
    return fail("callback invoked while holding the session lock");
  }
  if (auto rc = expect(session.request_stop(), C18_STATUS_OK, "request_stop while callback active")) {
    {
      std::scoped_lock guard(mutex);
      callback_released = true;
    }
    cv.notify_all();
    process_thread.join();
    return rc;
  }
  {
    std::unique_lock guard(mutex);
    if (!wait_for_event(cv, guard, [&] { return stop_called; })) {
      callback_released = true;
      cv.notify_all();
      guard.unlock();
      process_thread.join();
      return fail("request_stop did not reach plugin");
    }
  }

  auto rejected = session.process(bytes({'b'}), 1);
  if (auto rc = expect(rejected.status, C18_STATUS_CLOSING, "process rejected while closing")) {
    {
      std::scoped_lock guard(mutex);
      callback_released = true;
    }
    cv.notify_all();
    process_thread.join();
    return rc;
  }

  std::thread close_thread([&] {
    close_status = session.close();
    std::scoped_lock guard(mutex);
    close_done = true;
    cv.notify_all();
  });

  {
    std::unique_lock guard(mutex);
    if (!wait_for_event(cv, guard, [&] { return close_waiting || close_done; })) {
      callback_released = true;
      cv.notify_all();
      guard.unlock();
      process_thread.join();
      close_thread.join();
      return fail("close did not reach drain wait or return");
    }
    if (close_done) {
      callback_released = true;
      cv.notify_all();
      guard.unlock();
      process_thread.join();
      close_thread.join();
      return fail("close returned before callback drained");
    }
    if (destroy_called) {
      callback_released = true;
      cv.notify_all();
      guard.unlock();
      process_thread.join();
      close_thread.join();
      return fail("destroy called before callback drained");
    }
    callback_released = true;
    cv.notify_all();
  }

  process_thread.join();
  close_thread.join();

  if (auto rc = expect(process_result.status, C18_STATUS_OK, "controlled process result")) {
    return rc;
  }
  if (auto rc = expect(close_status, C18_STATUS_OK, "close after drain")) {
    return rc;
  }
  if (!destroy_called) {
    return fail("destroy not called after drain");
  }
  return 0;
}

auto check_destroy_retry(const std::filesystem::path& busy_destroy_plugin) -> int {
  c18_l04::PluginSession session;
  if (auto rc = expect(session.open(busy_destroy_plugin), C18_STATUS_OK, "open busy destroy plugin")) {
    return rc;
  }
  int destroy_events = 0;
  session.set_callback([&](std::string_view event_name) {
    if (event_name == "destroy:called") {
      ++destroy_events;
    }
  });

  auto result = session.process(bytes({'q'}), 1);
  if (auto rc = expect(result.status, C18_STATUS_OK, "busy destroy transform")) {
    return rc;
  }
  if (auto rc = expect(session.close(), C18_STATUS_BUSY, "first destroy returns busy")) {
    return rc;
  }
  if (auto rc = expect(session.request_stop(), C18_STATUS_OK, "request_stop after busy destroy")) {
    return rc;
  }
  if (auto rc = expect(session.close(), C18_STATUS_OK, "retry close after busy destroy")) {
    return rc;
  }
  if (destroy_events != 1) {
    return fail("retry destroy must destroy the retained context exactly once");
  }
  return 0;
}

auto check_release_retry(const std::filesystem::path& good_plugin) -> int {
  c18_l04::PluginSession session;
  if (auto rc = expect(session.open(good_plugin), C18_STATUS_OK, "open release retry plugin")) {
    return rc;
  }
  int destroy_events = 0;
  session.set_callback([&](std::string_view event_name) {
    if (event_name == "destroy:called") {
      ++destroy_events;
    }
  });

  session.fail_next_release_for_test();
  if (auto rc = expect(session.close(), C18_STATUS_BUSY, "simulated loader release failure")) {
    return rc;
  }
  if (destroy_events != 1) {
    return fail("release failure must happen after one successful destroy");
  }
  if (auto rc = expect(session.close(), C18_STATUS_OK, "retry loader release")) {
    return rc;
  }
  if (destroy_events != 1) {
    return fail("release retry must not destroy the old context again");
  }
  return expect(session.open(good_plugin), C18_STATUS_OK, "open after release retry");
}

auto run_checks(const std::filesystem::path& good_plugin,
                const std::filesystem::path& busy_destroy_plugin,
                const std::filesystem::path& missing_symbol,
                const std::filesystem::path& version_mismatch,
                const std::filesystem::path& throwing_entry) -> int {
  if (auto rc = check_basic_contracts(good_plugin, missing_symbol, version_mismatch, throwing_entry)) {
    return rc;
  }
  if (auto rc = check_controlled_drain(good_plugin)) {
    return rc;
  }
  if (auto rc = check_destroy_retry(busy_destroy_plugin)) {
    return rc;
  }
  if (auto rc = check_release_retry(good_plugin)) {
    return rc;
  }

  std::cout << "L04 plugin shutdown PASS\n";
  return 0;
}

}  // namespace

auto main(int argc, char** argv) -> int {
  if (argc != 6) {
    return fail("usage: checks good busy_destroy missing_symbol version_mismatch throwing_entry");
  }
  return run_checks(argv[1], argv[2], argv[3], argv[4], argv[5]);
}
