#include <execution>
#include <thread>

int main() {
  namespace ex = std::execution;
  ex::counting_scope scope;
  int value = 0;
  ex::spawn(ex::just(41) | ex::then([&](int input) { value = input + 1; }), scope.get_token());
  std::this_thread::sync_wait(scope.join());
  return value == 42 ? 0 : 1;
}
