#include "api.h"
#include "solution.hpp"
extern "C" int c18_l01_upper(const unsigned char* input, size_t length, unsigned char* output) {
    return solve(input, length, output);
}
