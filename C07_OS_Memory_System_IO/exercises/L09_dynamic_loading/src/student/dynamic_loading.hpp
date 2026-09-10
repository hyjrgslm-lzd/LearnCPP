#pragma once
#include <include/dynamic_loading_contract.hpp>

namespace c07_l09 {

inline ModuleResult load_module_add(const std::filesystem::path&, std::string_view,
                                    std::uint32_t, std::uint32_t, std::uint32_t) {
    return {.ok = false, .value = 0,
            .error = "student TODO: load absolute module path, resolve C ABI symbol, call, unload",
            .unloaded = false};
}

} // namespace c07_l09
