#pragma once

#include "05_adaptors.hpp"

namespace capstone4 {
void run_mini_ranges_checks();
}

namespace my::ranges {

template<class C>
struct to_closure : my::ranges::range_adaptor_closure<to_closure<C>> {
    template<my::ranges::input_range R>
    C operator()(R&& r) const {
        C out;
        for (auto&& value : r) {
            out.push_back(std::forward<decltype(value)>(value));
        }
        return out;
    }
};

template<class C>
constexpr auto to() {
    return to_closure<C>{};
}

} // namespace my::ranges
