#include <c10/test.hpp>
#include <exec/task.hpp>
#include <stdexec/execution.hpp>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace ex = stdexec;

namespace {
void check_simple_scope_close_and_join() {
  ex::simple_counting_scope scope;
  bool first = false;
  bool second = false;
  ex::sync_wait(ex::associate(ex::just(&first), scope.get_token()) |
                ex::then([](bool *flag) noexcept { *flag = true; }));
  scope.close();
  auto late = ex::sync_wait(ex::associate(ex::just(&second), scope.get_token()) |
                            ex::then([](bool *flag) noexcept { *flag = true; }));
  ex::sync_wait(scope.join());
  c10::require(first && !second && !late.has_value(),
               "simple_counting_scope close refuses late work and join drains");
}

void check_counting_scope_stop_token() {
  ex::counting_scope scope;
  scope.request_stop();
  auto result = ex::sync_wait(ex::associate(ex::read_env(ex::get_stop_token), scope.get_token()) |
                              ex::then([](auto token) noexcept { return token.stop_requested(); }));
  ex::sync_wait(scope.join());
  c10::require(result && std::get<0>(*result),
               "counting_scope request_stop reaches associated sender");
}

void check_spawn_runs_associated_work() {
  ex::simple_counting_scope scope;
  std::atomic<int> count = 0;
  ex::spawn(ex::just() | ex::then([&] noexcept { count.fetch_add(1); }), scope.get_token());
  ex::sync_wait(scope.join());
  c10::require(count.load() == 1, "spawn runs associated work");
}

void check_spawn_future_returns_value() {
  ex::simple_counting_scope scope;
  auto future = ex::spawn_future(
      ex::just(41) | ex::then([](int value) noexcept { return value + 1; }), scope.get_token());
  auto result = ex::sync_wait(std::move(future));
  ex::sync_wait(scope.join());
  c10::require(result && std::get<0>(*result) == 42,
               "spawn_future returns value through future sender");
}

void check_deferred_join_waits_for_late_completion() {
  ex::simple_counting_scope scope;
  auto assoc = scope.get_token().try_associate();
  c10::require(static_cast<bool>(assoc), "token association succeeds before close");
  std::mutex mutex;
  std::condition_variable cv;
  bool started = false;
  bool finished = false;
  std::jthread joiner([&] {
    {
      std::lock_guard lock(mutex);
      started = true;
    }
    cv.notify_one();
    ex::sync_wait(scope.join());
    {
      std::lock_guard lock(mutex);
      finished = true;
    }
    cv.notify_one();
  });
  {
    std::unique_lock lock(mutex);
    cv.wait(lock, [&] { return started; });
    c10::require(!finished, "join waits while association is alive");
  }
  assoc = decltype(assoc){};
  {
    std::unique_lock lock(mutex);
    cv.wait(lock, [&] { return finished; });
  }
}

void check_extension_boundary_is_named() {
  static_assert(requires { typename exec::task<int>; });
}
} // namespace

int main() {
  return c10::test_main([] {
    check_simple_scope_close_and_join();
    check_counting_scope_stop_token();
    check_spawn_runs_associated_work();
    check_spawn_future_returns_value();
    check_deferred_join_waits_for_late_completion();
    check_extension_boundary_is_named();
  });
}
