#pragma once

#include <array>
#include <string_view>

namespace capstone1_data {

inline constexpr std::array<std::string_view, 9> sample_lines{
    "2024-01-01T00:00:01,INFO,alice,login ok",
    "2024-01-01T00:00:02,WARN,bob,disk usage 80%",
    "2024-01-01T00:00:03,ERROR,alice,connection refused",
    "2024-01-01T00:00:04,ERROR,carol,timeout",
    "INVALID_LINE_NO_COMMAS",
    "2024-01-01T00:00:05,INFO,bob,task complete",
    "2024-01-01T00:00:06,ERROR,alice,segfault detected",
    "2024-01-01T00:00:07,BADLEVEL,dave,unknown",
    "2024-01-01T00:00:08,WARN,carol,retry 2",
};

} // namespace capstone1_data
