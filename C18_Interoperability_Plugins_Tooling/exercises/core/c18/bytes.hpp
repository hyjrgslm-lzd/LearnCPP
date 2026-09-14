#pragma once
#include "c18/abi.h"
#include <cstring>

namespace c18 {
// input/output may overlap; written must be separate from both payload regions.
inline c18_status transform_bytes(const uint8_t* input, size_t length, uint8_t* output,
                                  size_t capacity, size_t* written) noexcept {
    if (!written) return C18_STATUS_BAD_ARGUMENT;
    *written = 0;
    if (length && !input) return C18_STATUS_BAD_ARGUMENT;
    if (capacity < length) { *written = length; return C18_STATUS_BUFFER_TOO_SMALL; }
    if (length && !output) return C18_STATUS_BAD_ARGUMENT;
    if (length) std::memmove(output, input, length);
    for (size_t i = 0; i < length; ++i)
        if (output[i] >= 'a' && output[i] <= 'z') output[i] = static_cast<uint8_t>(output[i] - 32);
    *written = length;
    return C18_STATUS_OK;
}
}
