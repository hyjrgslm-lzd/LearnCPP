#include "contract.hpp"
#include "solution.hpp"

#include <cstdint>
#include <iostream>
#include <vector>

namespace {

int fail(const char* message) {
  std::cout << "check failed: " << message << '\n';
  return 1;
}

}

int main() {
  const auto result = c18_lua_l11::transform({'a', 0, 'z', 0xffu, 'A'});
  const std::vector<std::uint8_t> expected{'A', 0, 'Z', 0xffu, 'A'};
  if (result.bytes != expected) {
    return fail("explicit byte length");
  }
  if (result.top_before != result.top_after) {
    return fail("stack balance");
  }
  const auto empty = c18_lua_l11::transform({});
  if (!empty.bytes.empty() || empty.top_before != empty.top_after) {
    return fail("empty input preserves stack");
  }
  std::cout << "L11 Lua stack PASS\n";
}
