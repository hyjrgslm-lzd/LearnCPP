#include <execution>
#include <thread>

std::execution::task<int> make_task() { co_return 42; }

int main() {
  auto result = std::this_thread::sync_wait(make_task());
  auto [value] = result.value();
  return value == 42 ? 0 : 1;
}
