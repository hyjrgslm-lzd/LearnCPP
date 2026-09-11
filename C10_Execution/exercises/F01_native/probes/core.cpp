#include <execution>
#include <thread>
#include <utility>

int main() {
  namespace ex = std::execution;
  auto sender =
      ex::when_all(ex::just(20), ex::just(22)) | ex::then([](int a, int b) { return a + b; });
  auto result = std::this_thread::sync_wait(std::move(sender));
  auto [value] = result.value();
  return value == 42 ? 0 : 1;
}
