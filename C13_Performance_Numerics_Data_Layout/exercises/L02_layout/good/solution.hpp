#pragma once
#include "c13/layout.hpp"
namespace solution {
inline c13::particles_soa convert(std::span<const c13::particle> input) {
    c13::particles_soa r;
    for (auto p : input) {
        r.x.push_back(p.x); r.y.push_back(p.y); r.z.push_back(p.z);
        r.vx.push_back(p.vx); r.vy.push_back(p.vy); r.vz.push_back(p.vz);
    }
    return r;
}
inline void advance(c13::particles_soa& p, float dt) {
    c13::validate(p); c13::validate_dt(dt);
    auto update = [dt](auto& x, const auto& v) {
        for (std::size_t i=0; i<x.size(); ++i) x[i] = x[i] + dt*v[i];
    };
    update(p.x,p.vx); update(p.y,p.vy); update(p.z,p.vz);
}
}
