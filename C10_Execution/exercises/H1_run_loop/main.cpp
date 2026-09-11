#include <c10/test.hpp>
#include <solution.hpp>
#include <stdexec/execution.hpp>

#include <atomic>
#include <exception>
#include <thread>
#include <tuple>
#include <vector>
#include <type_traits>

namespace ex = stdexec;

namespace {
void check_fifo_and_cross_thread() {
  c10_h1::run_loop loop;
  std::vector<int> seen;
  std::thread::id loop_thread;
  auto make = [&](int id) {
    return loop.schedule() | ex::then([&, id] {
             loop_thread = std::this_thread::get_id();
             seen.push_back(id);
           });
  };
  std::jthread runner([&] { loop.run(); });
  auto result = ex::sync_wait(ex::when_all(make(1), make(2), make(3)));
  loop.close();
  c10::require(result.has_value(), "scheduled work completes");
  c10::require(seen == std::vector<int>({1, 2, 3}), "run_loop keeps FIFO order");
  c10::require(loop_thread == runner.get_id(), "completion runs on loop thread");
}

void check_close_drains_and_rejects() {
  c10_h1::run_loop loop;
  std::atomic<int> count = 0;
  auto first =
      ex::connect(loop.schedule() | ex::then([&] { ++count; }), c10_h1::detached_receiver{});
  auto second =
      ex::connect(loop.schedule() | ex::then([&] { ++count; }), c10_h1::detached_receiver{});
  ex::start(first);
  ex::start(second);
  loop.close();
  loop.run();
  c10::require(count == 2, "close drains accepted work");
  bool rejected = false;
  try {
    (void)ex::sync_wait(loop.schedule());
  } catch (const c10_h1::run_loop_closed &) {
    rejected = true;
  }
  c10::require(rejected, "closed run_loop rejects new work");
}
} // namespace

int main() {
  return c10::test_main([] {
    check_fifo_and_cross_thread();
    check_close_drains_and_rejects();
    c10_h1::run_loop loop;
    auto op = ex::connect(loop.schedule(), c10_h1::detached_receiver{});
    c10::require(!std::is_move_constructible_v<decltype(op)> &&
                     !std::is_copy_constructible_v<decltype(op)>,
                 "queued operation state is nonmovable");
    ex::start(op);
    loop.close();
    loop.run();
  });
}
