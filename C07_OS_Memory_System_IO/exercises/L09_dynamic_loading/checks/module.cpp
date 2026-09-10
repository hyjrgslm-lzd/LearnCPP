#include <include/dynamic_loading_contract.hpp>

#include <bit>
#include <cstdint>

#ifdef _WIN32
#define C07_EXPORT extern "C" __declspec(dllexport)
#else
#define C07_EXPORT extern "C" __attribute__((visibility("default")))
#endif

namespace {
std::uint32_t g_calls = 0;

std::uint32_t mix_formula(std::uint32_t a, std::uint32_t b, std::uint32_t nonce) {
    return std::rotl(a ^ nonce, 5) ^ (b * 2654435761u) ^ 0xC07D9009u;
}
} // namespace

C07_EXPORT std::uint32_t c07_module_mix(std::uint32_t a, std::uint32_t b, std::uint32_t nonce) {
    ++g_calls;
    return mix_formula(a, b, nonce);
}

C07_EXPORT std::uint32_t c07_module_calls() {
    return g_calls;
}
