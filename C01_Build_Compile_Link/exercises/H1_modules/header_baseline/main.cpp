#include <check.hpp>
#include "geometry.hpp"

int main() {
    check(header_square_area(6) == 36, "header baseline should keep the same public contract");
}
