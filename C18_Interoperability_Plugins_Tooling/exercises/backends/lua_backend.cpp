#include "lua_backend.hpp"

#include "lua_boundary.h"

#include <stdexcept>
#include <string>

namespace c18 {

std::vector<std::uint8_t> lua_transform(std::span<const std::uint8_t> input) {
  std::vector<std::uint8_t> output(input.size());
  c18_lua_result result{};
  const int status = c18_lua_transform_once(input.data(), input.size(), output.data(), output.size(), &result);
  if (status == C18_LUA_BUFFER_TOO_SMALL) {
    throw std::runtime_error("Lua backend reported an impossible output capacity mismatch");
  }
  if (status != C18_LUA_OK || result.written != input.size()) {
    throw std::runtime_error(std::string("Lua backend failed: ") + result.message);
  }
  output.resize(result.written);
  return output;
}

}
