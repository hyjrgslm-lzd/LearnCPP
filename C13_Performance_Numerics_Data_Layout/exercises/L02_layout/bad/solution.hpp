#pragma once
#include "c13/layout.hpp"
namespace solution {
inline c13::particles_soa convert(std::span<const c13::particle> input) { return c13::to_soa(input); }
inline void advance(c13::particles_soa& p, float dt) {
    c13::validate(p); c13::validate_dt(dt);
    // Deliberate semantic defect: loses every incomplete group of four.
    for (std::size_t i=0; i<p.x.size()/4*4; ++i) {
        p.x[i]+=p.vx[i]*dt; p.y[i]+=p.vy[i]*dt; p.z[i]+=p.vz[i]*dt;
    }
}
}
