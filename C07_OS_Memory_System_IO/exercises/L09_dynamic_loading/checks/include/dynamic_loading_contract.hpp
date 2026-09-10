#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace c07_l09 {

inline constexpr char module_symbol[] = "c07_module_mix";
inline constexpr char module_calls_symbol[] = "c07_module_calls";

struct ModuleResult {
    bool ok = false;
    std::uint32_t value = 0;
    std::string error;
    bool unloaded = false;
};

using module_mix_fn = std::uint32_t (*)(std::uint32_t, std::uint32_t, std::uint32_t);
using module_calls_fn = std::uint32_t (*)();

} // namespace c07_l09
