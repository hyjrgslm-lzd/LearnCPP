#include "c13/matrix.hpp"
#include "solution.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {
void check(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error("check failed: " + std::string{message});
}

void check_close(double actual, double expected, double tolerance, std::string_view message) {
    check(std::isfinite(actual) && std::abs(actual - expected) <= tolerance, message);
}
}

int main() try {
    const std::vector<double> a{1, 2, 3, 4, 5, 6};
    const std::vector<double> b{1, 2, 3, 4, 5, 6};
    const auto c = student::multiply(a, 2, 3, b, 3, 2, 2);
    check(c.size() == 4, "matrix output shape mismatch");
    check_close(c[0], 22, 0, "c00");
    check_close(c[1], 28, 0, "c01");
    check_close(c[2], 49, 0, "c10");
    check_close(c[3], 64, 0, "c11");

    const std::vector<double> tail_a{
        1, 2, 3, 4, 5,
        6, 7, 8, 9, 10,
        11, 12, 13, 14, 15
    };
    const std::vector<double> tail_b{
        1, 2, 3, 4,
        5, 6, 7, 8,
        9, 10, 11, 12,
        13, 14, 15, 16,
        17, 18, 19, 20
    };
    const auto tail_c = student::multiply(tail_a, 3, 5, tail_b, 5, 4, 2);
    const std::vector<double> expected_tail{
        175, 190, 205, 220,
        400, 440, 480, 520,
        625, 690, 755, 820
    };
    check(tail_c.size() == expected_tail.size(), "matrix tail output size");
    for (std::size_t i = 0; i < expected_tail.size(); ++i) {
        check_close(tail_c[i], expected_tail[i], 0.0, "matrix tail tile");
    }

    bool rejected = false;
    try { (void)student::multiply(a, 2, 3, b, 2, 3, 2); } catch (const std::exception&) { rejected = true; }
    check(rejected, "matrix inner shape mismatch");

    rejected = false;
    try { (void)student::multiply(a, 2, 4, b, 3, 2, 2); } catch (const std::exception&) { rejected = true; }
    check(rejected, "matrix input shape mismatch");

    rejected = false;
    try { (void)student::multiply(a, 2, 3, b, 3, 2, 0); } catch (const std::exception&) { rejected = true; }
    check(rejected, "tile must be positive");

    rejected = false;
    try {
        const auto huge = std::numeric_limits<std::size_t>::max();
        (void)student::multiply({}, huge, 2, {}, 2, 1, 1);
    } catch (const std::exception&) { rejected = true; }
    check(rejected, "shape product overflows size_t");

    std::vector<double> overlap_a{1, 2, 3, 4};
    const auto unchanged=overlap_a;
    rejected = false;
    try {
        c13::matmul_tiled(overlap_a, 2, 2, overlap_a, 2, 2, overlap_a, 2, 2, 2);
    } catch (const std::exception&) { rejected = true; }
    check(rejected, "matrix output overlaps input");
    check(overlap_a==unchanged,"overlap rejection precedes all writes");
    rejected=false;
    try {
        c13::matmul_tiled(c13::as_matrix(overlap_a,2,2),c13::as_matrix(overlap_a,2,2),
            c13::as_writable_matrix(overlap_a,2,2),2);
    } catch(const std::invalid_argument&) { rejected=true; }
    check(rejected && overlap_a==unchanged,"mdspan overload enforces the same overlap contract");

    std::cout << "matrix checks passed\n";
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
