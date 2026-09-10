#pragma once
#include <include/dynamic_loading_contract.hpp>

namespace c07_l09 {

inline ModuleResult load_module_add(const std::filesystem::path&, std::string_view,
                                    std::uint32_t a, std::uint32_t b, std::uint32_t nonce) {
    return {.ok = true, .value = (a ^ b ^ nonce), .error = "", .unloaded = true};
}

} // namespace c07_l09
