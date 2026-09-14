#pragma once
#include <cstddef>
inline int solve(const unsigned char* input, std::size_t length, unsigned char* output) {
    for (std::size_t i = 0; i < length; ++i) {
        const auto byte = input[i];
        output[i] = byte >= 'a' && byte <= 'z' ? static_cast<unsigned char>(byte - ('a' - 'A')) : byte;
    }
    return 0;
}
