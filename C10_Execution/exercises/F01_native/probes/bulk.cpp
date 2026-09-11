#include <execution>
#include <thread>
#include <utility>

int main() {
  namespace ex = std::execution;
  auto sender = ex::just(0) | ex::bulk(ex::par, 4, [](int, int &) {});
  auto result = std::this_thread::sync_wait(std::move(sender));
  return result ? 0 : 1;
}
