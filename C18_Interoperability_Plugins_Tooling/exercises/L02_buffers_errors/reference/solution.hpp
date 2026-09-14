#pragma once
#include "../contract.hpp"
#include <cstring>
inline Result transform(const unsigned char* input, std::size_t length, unsigned char* output, std::size_t capacity) {
    if (length && !input) return {Status::invalid, 0};
    if (capacity < length) return {Status::too_small, length};
    if (length && !output) return {Status::invalid, 0};
    if (length) std::memmove(output, input, length);
    for (std::size_t i = 0; i < length; ++i)
        if (output[i] >= 'a' && output[i] <= 'z') output[i] = static_cast<unsigned char>(output[i] - 32);
    return {Status::ok, length};
}
