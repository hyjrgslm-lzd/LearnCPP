#pragma once
#include "c13/layout.hpp"
namespace solution {
inline c13::particles_soa convert(std::span<const c13::particle>) {
    throw std::runtime_error("student unfinished: implement AoS to SoA conversion");
}
inline void advance(c13::particles_soa&, float) {
    throw std::runtime_error("student unfinished: validate shape and update all positions");
}
}
