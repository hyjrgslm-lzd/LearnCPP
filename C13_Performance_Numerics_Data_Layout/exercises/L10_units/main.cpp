#include "c13/common.hpp"
#include <mp-units/systems/isq.h>
#include <mp-units/systems/si.h>
using namespace mp_units;
using namespace mp_units::si::unit_symbols;

template<QuantityOf<isq::length> Distance, QuantityOf<isq::time> Time>
constexpr auto speed(Distance d,Time t) { return d/t; }

int main() { return c13::run([] {
    constexpr auto dt=16.0*ms;
    constexpr auto elapsed=dt.in(s);
    constexpr auto v=speed(120.0*m,10.0*s);
    constexpr auto displacement=v*elapsed;
    static_assert(v.numerical_value_in(m/s)==12.0);
    static_assert(!QuantityOf<decltype(dt),isq::length>);
    check(c13::near(elapsed.numerical_value_in(s),0.016),"milliseconds convert to seconds");
    check(c13::near(displacement.numerical_value_in(m),0.192),"speed times duration yields length");
    std::cout << "dt=" << elapsed << "; displacement=" << displacement << '\n';
}); }
