#pragma once
#include "c13/layout.hpp"
namespace solution {
inline c13::particles_soa convert(std::span<const c13::particle> input) { return c13::to_soa(input); }
inline void advance(c13::particles_soa& p, float dt) { c13::advance_soa(p, dt); }
}
