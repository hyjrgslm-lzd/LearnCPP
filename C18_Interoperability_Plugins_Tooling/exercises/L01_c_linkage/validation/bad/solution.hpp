#pragma once
#include <cstddef>
inline int solve(const unsigned char* input, std::size_t length, unsigned char* output) {
    // Wrong contract: stops at a NUL as if the input were a C string.
    for (std::size_t i = 0; i < length && input[i] != 0; ++i)
        output[i] = input[i] >= 'a' && input[i] <= 'z' ? static_cast<unsigned char>(input[i] - 32) : input[i];
    return 0;
}
