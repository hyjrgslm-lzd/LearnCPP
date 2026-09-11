#include <c10/test.hpp>
#include <stdexec/execution.hpp>
#include <exec/async_scope.hpp>
#include <exec/static_thread_pool.hpp>
#include <exec/task.hpp>
#include <atomic>
#include <iostream>
#include <string>
#include <tuple>

namespace ex = stdexec;

ex::task<int> standard_model_task() {
  const int value = co_await ex::just(40);
  co_return value + 2;
}
exec::task<int> extension_task() {
  const int value = co_await ex::just(20);
  co_return value * 2 + 2;
}
int main() {
  return c10::test_main([] {
    int calls = 0;
    auto hello = ex::just(std::string{"hello"}) | ex::then([&](std::string text) {
                   ++calls;
                   return text + " sender";
                 });
    c10::require(calls == 0, "building a sender does not execute then");
    auto value = ex::sync_wait(std::move(hello));
    c10::require(value && std::get<0>(*value) == "hello sender" && calls == 1,
                 "sync_wait connects and starts exactly once");
    auto standard_task = ex::sync_wait(standard_model_task());
    auto legacy_task = ex::sync_wait(extension_task());
    c10::require(standard_task && legacy_task && std::get<0>(*standard_task) == 42 &&
                     std::get<0>(*legacy_task) == 42,
                 "actual reference and extension coroutine tasks run");
    std::atomic<int> completed{0};
    exec::static_thread_pool pool(2);
    exec::async_scope scope;
    try {
      for (int i = 0; i != 3; ++i) {
        scope.spawn(ex::schedule(pool.get_scheduler()) | ex::then([&]() noexcept {
                      completed.fetch_add(1, std::memory_order_relaxed);
                    }));
      }
    } catch (...) {
      ex::sync_wait(scope.on_empty());
      throw;
    }
    ex::sync_wait(scope.on_empty());
    c10::require(completed.load() == 3, "async_scope really retires all three launched operations");
    std::cout
        << "observed: lazy then; stdexec::task and exec::task; three async_scope completions.\n";
  });
}
