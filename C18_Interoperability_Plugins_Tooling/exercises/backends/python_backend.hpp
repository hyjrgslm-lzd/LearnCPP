#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace c18 {
std::vector<std::uint8_t> python_transform(std::span<const std::uint8_t> input);
}
