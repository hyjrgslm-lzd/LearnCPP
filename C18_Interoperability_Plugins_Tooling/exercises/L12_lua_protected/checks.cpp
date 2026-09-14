#include "contract.hpp"
#include "solution.hpp"

#include <iostream>

int fail(const char* message) {
  std::cout << "check failed: " << message << '\n';
  return 1;
}

int main() {
  const auto ok = c18_lua_l12::run_case(false);
  if (ok.status != 0 || !ok.protected_call || !ok.setup_checked || ok.cleanup_count != 1 || ok.top_after != 0) {
    return fail("protected success cleanup");
  }
  const auto bad = c18_lua_l12::run_case(true);
  if (bad.status == 0 || !bad.protected_call || !bad.setup_checked || bad.cleanup_count != 1 ||
      bad.message.find("helper") == std::string::npos) {
    return fail("protected trampoline error");
  }
  if (bad.top_after != 0) {
    return fail("error stack cleanup");
  }
  std::cout << "L12 Lua protected errors PASS\n";
}
