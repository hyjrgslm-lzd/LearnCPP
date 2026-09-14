#ifndef C13_MATRIX_HPP
#define C13_MATRIX_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <mdspan>
#include <span>
#include <stdexcept>
#include <vector>

namespace c13 {

using const_matrix_view = std::mdspan<const double, std::dextents<std::size_t, 2>, std::layout_right>;
using matrix_view = std::mdspan<double, std::dextents<std::size_t, 2>, std::layout_right>;

inline void require_matrix_input(bool condition, const char* message) {
    if (!condition) throw std::invalid_argument(message);
}

inline std::size_t matrix_checked_product(std::size_t a, std::size_t b) {
    if (b != 0 && a > std::numeric_limits<std::size_t>::max() / b)
        throw std::length_error("shape product overflows size_t");
    return a * b;
}

inline void check_matrix_shape(std::span<const double> data, std::size_t rows, std::size_t cols) {
    require_matrix_input(matrix_checked_product(rows, cols) == data.size(), "matrix input shape mismatch");
}

inline const_matrix_view as_matrix(std::span<const double> data, std::size_t rows, std::size_t cols) {
    check_matrix_shape(data, rows, cols);
    return const_matrix_view{data.data(), rows, cols};
}

inline matrix_view as_writable_matrix(std::span<double> data, std::size_t rows, std::size_t cols) {
    require_matrix_input(matrix_checked_product(rows, cols) == data.size(), "matrix output shape mismatch");
    return matrix_view{data.data(), rows, cols};
}

inline bool overlaps(std::span<const double> a, std::span<const double> b) noexcept {
    if (a.empty() || b.empty()) return false;
    const auto less = std::less<const void*>{};
    return less(a.data(), b.data()+b.size()) && less(b.data(), a.data()+a.size());
}

inline void check_no_output_overlap(std::span<const double> a, std::span<const double> b, std::span<const double> c) {
    require_matrix_input(!overlaps(c, a) && !overlaps(c, b), "matrix output overlaps input");
}

inline void matmul_tiled(const_matrix_view a, const_matrix_view b, matrix_view c, std::size_t tile) {
    require_matrix_input(a.extent(1) == b.extent(0), "matrix inner shape mismatch");
    require_matrix_input(c.extent(0) == a.extent(0) && c.extent(1) == b.extent(1), "matrix output shape mismatch");
    require_matrix_input(tile > 0, "tile must be positive");
    check_no_output_overlap({a.data_handle(),a.size()}, {b.data_handle(),b.size()}, {c.data_handle(),c.size()});
    for (std::size_t i = 0; i < c.extent(0); ++i) {
        for (std::size_t j = 0; j < c.extent(1); ++j) c[i, j] = 0.0;
    }
    for (std::size_t ii = 0; ii < a.extent(0); ii += std::min(tile, a.extent(0) - ii)) {
        for (std::size_t kk = 0; kk < a.extent(1); kk += std::min(tile, a.extent(1) - kk)) {
            for (std::size_t jj = 0; jj < b.extent(1); jj += std::min(tile, b.extent(1) - jj)) {
                const auto i_end = ii + std::min(tile, a.extent(0) - ii);
                const auto k_end = kk + std::min(tile, a.extent(1) - kk);
                const auto j_end = jj + std::min(tile, b.extent(1) - jj);
                for (std::size_t i = ii; i < i_end; ++i) {
                    for (std::size_t k = kk; k < k_end; ++k) {
                        const double aik = a[i, k];
                        for (std::size_t j = jj; j < j_end; ++j) c[i, j] += aik * b[k, j];
                    }
                }
            }
        }
    }
}

inline void matmul_naive(const_matrix_view a, const_matrix_view b, matrix_view c) {
    require_matrix_input(a.extent(1) == b.extent(0), "matrix inner shape mismatch");
    require_matrix_input(c.extent(0) == a.extent(0) && c.extent(1) == b.extent(1), "matrix output shape mismatch");
    check_no_output_overlap({a.data_handle(),a.size()}, {b.data_handle(),b.size()}, {c.data_handle(),c.size()});
    for (std::size_t i = 0; i < c.extent(0); ++i) {
        for (std::size_t j = 0; j < c.extent(1); ++j) {
            double sum = 0.0;
            for (std::size_t k = 0; k < a.extent(1); ++k) sum += a[i, k] * b[k, j];
            c[i, j] = sum;
        }
    }
}

inline void matmul_tiled(std::span<const double> a, std::size_t ar, std::size_t ac,
                         std::span<const double> b, std::size_t br, std::size_t bc,
                         std::span<double> c, std::size_t cr, std::size_t cc,
                         std::size_t tile) {
    require_matrix_input(ac == br, "matrix inner shape mismatch");
    require_matrix_input(ar == cr && bc == cc, "matrix output shape mismatch");
    check_matrix_shape(a, ar, ac);
    check_matrix_shape(b, br, bc);
    require_matrix_input(matrix_checked_product(cr, cc) == c.size(), "matrix output shape mismatch");
    check_no_output_overlap(a, b, c);
    matmul_tiled(as_matrix(a, ar, ac), as_matrix(b, br, bc), as_writable_matrix(c, cr, cc), tile);
}

inline std::vector<double> matmul_tiled(std::span<const double> a, std::size_t ar, std::size_t ac,
                                        std::span<const double> b, std::size_t br, std::size_t bc,
                                        std::size_t tile) {
    require_matrix_input(ac == br, "matrix inner shape mismatch");
    check_matrix_shape(a, ar, ac);
    check_matrix_shape(b, br, bc);
    require_matrix_input(tile > 0, "tile must be positive");
    std::vector<double> c(matrix_checked_product(ar, bc));
    matmul_tiled(as_matrix(a, ar, ac), as_matrix(b, br, bc), as_writable_matrix(c, ar, bc), tile);
    return c;
}

} // namespace c13
#endif
