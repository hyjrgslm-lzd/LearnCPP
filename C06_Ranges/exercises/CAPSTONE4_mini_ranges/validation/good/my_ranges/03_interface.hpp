#pragma once

#include "02_concepts.hpp"

namespace my::ranges {

template<class D>
class view_interface {};

template<class D>
struct range_adaptor_closure {
    template<class R>
    friend constexpr decltype(auto) operator|(R&& r, const range_adaptor_closure& self) {
        return static_cast<const D&>(self)(std::forward<R>(r));
    }
};

} // namespace my::ranges
