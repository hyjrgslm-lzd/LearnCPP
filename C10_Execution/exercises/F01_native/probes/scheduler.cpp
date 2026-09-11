#include <execution>
#include <thread>
#include <utility>

int main() {
  namespace ex = std::execution;
  ex::scheduler auto scheduler = ex::inline_scheduler{};
  auto result = std::this_thread::sync_wait(ex::schedule(scheduler));
  return result ? 0 : 1;
}
