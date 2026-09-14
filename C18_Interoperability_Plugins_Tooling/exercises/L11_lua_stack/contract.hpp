#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace c18_lua_l11 {

struct Result {
  std::vector<std::uint8_t> bytes;
  int top_before{};
  int top_after{};
};

Result transform(std::vector<std::uint8_t> input);

}
