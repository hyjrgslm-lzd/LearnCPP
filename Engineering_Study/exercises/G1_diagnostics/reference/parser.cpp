#include "parser.hpp"

#include <stdexcept>

int parse_two_digits(std::string_view text) {
    if (text.size() != 2 || text[0] < '0' || text[0] > '9' || text[1] < '0' || text[1] > '9') {
        throw std::invalid_argument("expected exactly two decimal digits");
    }
    return (text[0] - '0') * 10 + (text[1] - '0');
}
