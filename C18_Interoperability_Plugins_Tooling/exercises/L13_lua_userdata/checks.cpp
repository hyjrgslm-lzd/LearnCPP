#include "contract.hpp"
#include "solution.hpp"

#include <iostream>

int fail(const char* message) {
  std::cout << "check failed: " << message << '\n';
  return 1;
}

int main() {
  const auto result = c18_lua_l13::exercise_userdata();
  if (!result.metatable || result.gc_calls < 1) {
    return fail("userdata metatable and gc");
  }
  if (result.closes != 1) {
    return fail("close must be idempotent");
  }
  std::cout << "L13 Lua userdata GC PASS\n";
}
