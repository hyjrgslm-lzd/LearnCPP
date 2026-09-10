#pragma once

#include "02_concepts.hpp"
#include <utility>

namespace my::ranges {

template<class D>
struct view_interface {
    bool empty() { auto& d = static_cast<D&>(*this); return my::ranges::begin(d) == my::ranges::end(d); }
    decltype(auto) front() { return *my::ranges::begin(static_cast<D&>(*this)); }
};

template<class D>
struct range_adaptor_closure {
    template<my::ranges::range R>
    friend auto operator|(R&& r, const D& d) { return d(std::forward<R>(r)); }
};

} // namespace my::ranges
