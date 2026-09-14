#pragma once
#include "c18/abi.h"

extern "C" c18_status C18_CALL c18_extra_ping(c18_context* ctx, const uint8_t* input, size_t input_size,
                                              uint8_t* output, size_t output_capacity, size_t* written);
