#pragma once

#include <check.hpp>

#include <stdexcept>
#include <string_view>

inline void check_parse_two_digits_contract(int (*parse)(std::string_view)) {
    for (char tens = '0'; tens <= '9'; ++tens) {
        for (char ones = '0'; ones <= '9'; ++ones) {
            char input[] = {tens, ones};
            const int expected = (tens - '0') * 10 + (ones - '0');
            try {
                check(parse(std::string_view(input, 2)) == expected,
                      "parser must accept every two-digit ASCII input from 00 to 99");
            } catch (...) {
                check(false, "parser must not throw for valid two-digit ASCII input");
            }
        }
    }

    constexpr std::string_view invalid_inputs[] = {
        "",      // empty
        "0",     // short
        "123",   // long
        "4x",    // non-digit second character
        "x4",    // non-digit first character
        " 4",    // whitespace first character
        "4 ",    // whitespace second character
        "-1",    // sign is not a digit
        "+1",    // sign is not a digit
        "４2",   // non-ASCII digit-like text
    };

    for (std::string_view input : invalid_inputs) {
        bool rejected = false;
        try {
            (void)parse(input);
        } catch (const std::invalid_argument&) {
            rejected = true;
        } catch (...) {
            check(false, "parser must reject invalid input with invalid_argument, not another exception");
        }
        check(rejected, "parser must reject non-two-digit ASCII input with invalid_argument");
    }
}
