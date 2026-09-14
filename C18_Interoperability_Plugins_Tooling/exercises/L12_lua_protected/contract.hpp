#pragma once

#include <string>

namespace c18_lua_l12 {

struct RunResult {
  int status{};
  bool protected_call{};
  bool setup_checked{};
  int cleanup_count{};
  int top_after{};
  std::string message;
};

RunResult run_case(bool fail);

}
