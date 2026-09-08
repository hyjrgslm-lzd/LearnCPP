#include "parser.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string_view>

namespace {

bool is_digit(char value) {
    return value >= '0' && value <= '9';
}

int oracle_value(std::string_view text) {
    if (text.size() != 2 || !is_digit(text[0]) || !is_digit(text[1])) {
        throw std::invalid_argument("expected exactly two decimal digits");
    }
    return (text[0] - '0') * 10 + (text[1] - '0');
}

[[noreturn]] void reject(const char* reason) {
    std::fprintf(stderr, "G1_ORACLE_MISMATCH:%s\n", reason);
    std::fflush(stderr);
    std::abort();
}

}  // namespace

extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    std::string_view text(reinterpret_cast<const char*>(data), size);
    bool expected_valid = true;
    int expected = 0;
    try {
        expected = oracle_value(text);
    } catch (const std::invalid_argument&) {
        expected_valid = false;
    }

    if (expected_valid) {
        try {
            int actual = parse_two_digits(text);
            if (actual != expected) {
                reject("valid-value");
            }
        } catch (...) {
            reject("valid-threw");
        }
    } else {
        try {
            (void)parse_two_digits(text);
            reject("invalid-accepted");
        } catch (const std::invalid_argument&) {
        } catch (...) {
            reject("invalid-wrong-exception");
        }
    }
    return 0;
}
