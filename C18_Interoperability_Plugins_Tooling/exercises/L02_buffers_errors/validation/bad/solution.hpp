#pragma once
#include "../../contract.hpp"
#include <algorithm>
inline Result transform(const unsigned char* input, std::size_t length, unsigned char* output, std::size_t capacity) {
    if (length && (!input || !output)) return {Status::invalid, 0};
    // Wrong: commits a prefix before reporting that the complete operation cannot fit.
    for (std::size_t i = 0; i < std::min(length, capacity); ++i)
        output[i] = input[i] >= 'a' && input[i] <= 'z' ? static_cast<unsigned char>(input[i] - 32) : input[i];
    return {capacity < length ? Status::too_small : Status::ok, length};
}
