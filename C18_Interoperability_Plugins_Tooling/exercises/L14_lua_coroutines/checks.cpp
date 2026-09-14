#include "contract.hpp"
#include "solution.hpp"

#include <iostream>
#include <vector>

int fail(const char* message) {
  std::cout << "check failed: " << message << '\n';
  return 1;
}

int main() {
  const auto result = c18_lua_l14::run_coroutine();
  if (result.yielded != std::vector<int>{1, 2, 3} || result.final_status != 0 || !result.continuation_seen) {
    return fail("resume yield sequence");
  }
  if (!result.queued_from_worker || result.resumed_on_foreign_thread || result.worker_thread == result.owner_thread ||
      result.last_resume_thread != result.owner_thread) {
    return fail("owner queue");
  }
  std::cout << "L14 Lua coroutines PASS\n";
}
