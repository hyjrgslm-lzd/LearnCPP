#include "kernel.hpp"
extern "C" __declspec(dllexport) std::uint32_t use_0(std::uint32_t value) {
    return transform<64>(value + 0);
}
