#pragma once
#include "c13/check.hpp"
#include <random>
#include <span>
#include <vector>
namespace c13 {
struct particle { float x, y, z, vx, vy, vz; };
struct particles_soa { std::vector<float> x, y, z, vx, vy, vz; };
inline void validate(const particles_soa& p) {
    const auto n=p.x.size();
    if(p.y.size()!=n || p.z.size()!=n || p.vx.size()!=n || p.vy.size()!=n || p.vz.size()!=n)
        throw std::invalid_argument("SoA component lengths differ");
}
inline std::vector<particle> make_particles(std::size_t n,unsigned seed=20260914) {
    if(n>1'000'000) throw std::length_error("particle count exceeds the teaching memory budget");
    std::mt19937 engine(seed);
    // Fix the engine-to-float mapping as well as the engine seed.
    auto next=[&] {return static_cast<float>(engine()>>8)*0x1p-23f-1.0f;};
    std::vector<particle> out;
    out.reserve(n);
    for(std::size_t i=0;i<n;++i) out.push_back({next(),next(),next(),next(),next(),next()});
    return out;
}
inline particles_soa to_soa(std::span<const particle> input) {
    particles_soa out;
    out.x.reserve(input.size()); out.y.reserve(input.size()); out.z.reserve(input.size());
    out.vx.reserve(input.size()); out.vy.reserve(input.size()); out.vz.reserve(input.size());
    for(auto p:input) {
        out.x.push_back(p.x); out.y.push_back(p.y); out.z.push_back(p.z);
        out.vx.push_back(p.vx); out.vy.push_back(p.vy); out.vz.push_back(p.vz);
    }
    return out;
}
inline std::vector<particle> to_aos(const particles_soa& input) {
    validate(input);
    std::vector<particle> out(input.x.size());
    for(std::size_t i=0;i<out.size();++i)
        out[i]={input.x[i],input.y[i],input.z[i],input.vx[i],input.vy[i],input.vz[i]};
    return out;
}
inline void validate_dt(float dt) {
    if(!std::isfinite(dt) || dt<0) throw std::invalid_argument("dt must be finite and nonnegative");
}
inline void advance_aos(std::span<particle> p,float dt) {
    validate_dt(dt);
    for(auto& v:p) {v.x+=v.vx*dt; v.y+=v.vy*dt; v.z+=v.vz*dt;}
}
inline void advance_soa(particles_soa& p,float dt) {
    validate(p); validate_dt(dt);
    for(std::size_t i=0;i<p.x.size();++i) {
        p.x[i]+=p.vx[i]*dt; p.y[i]+=p.vy[i]*dt; p.z[i]+=p.vz[i]*dt;
    }
}
}
