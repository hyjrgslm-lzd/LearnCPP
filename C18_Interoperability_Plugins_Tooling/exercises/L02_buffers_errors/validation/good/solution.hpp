#pragma once
#include "../../contract.hpp"
#include <algorithm>
#include <vector>
inline Result transform(const unsigned char* input, std::size_t length, unsigned char* output, std::size_t capacity) {
    if (!length) return {Status::ok, 0};
    if (!input) return {Status::invalid, 0};
    if (length > capacity) return {Status::too_small, length};
    if (!output) return {Status::invalid, 0};
    const std::vector<unsigned char> snapshot(input, input + length);
    std::transform(snapshot.begin(), snapshot.end(), output, [](unsigned char byte) {
        return byte >= 97 && byte <= 122 ? static_cast<unsigned char>(byte - 32) : byte;
    });
    return {Status::ok, length};
}
