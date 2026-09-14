#pragma once
#include "c13/layout.hpp"
#include "c13/numerics.hpp"
#include <array>
#include <mdspan>
namespace c13 {
struct milliseconds { double value; };
inline float seconds_value(milliseconds dt) {
    if(!std::isfinite(dt.value) || dt.value<0 || dt.value>1000)
        throw std::invalid_argument("duration must be finite and in [0,1000] milliseconds");
    return static_cast<float>(dt.value/1000.0);
}
struct pipeline_result {
    std::vector<double> positions; // Row-major N x 3, owns the transformed output.
    double sum{};
};
inline pipeline_result run_pipeline(std::span<const particle> input,milliseconds duration) {
    const float dt=seconds_value(duration);
    if(input.size()>1'000'000) throw std::length_error("pipeline input exceeds teaching memory budget");
    for(auto p:input)
        if(!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)
           || !std::isfinite(p.vx) || !std::isfinite(p.vy) || !std::isfinite(p.vz))
            throw std::invalid_argument("pipeline requires finite input components");
    auto soa=to_soa(input);
    advance_soa(soa,dt);
    constexpr std::array<double,9> rotation{0,-1,0,1,0,0,0,0,1};
    const std::mdspan<const double,std::extents<std::size_t,3,3>> R(rotation.data());
    pipeline_result result{std::vector<double>(checked_product(input.size(),3)),0};
    std::mdspan<double,std::dextents<std::size_t,2>> out(result.positions.data(),input.size(),3);
    for(std::size_t i=0;i<input.size();++i) {
        const std::array<double,3> p{soa.x[i],soa.y[i],soa.z[i]};
        for(std::size_t row=0;row<3;++row) {
            double value=0;
            for(std::size_t k=0;k<3;++k) value+=R[row,k]*p[k];
            if(!std::isfinite(value)) throw std::overflow_error("updated position is nonfinite");
            out[i,row]=value;
        }
    }
    result.sum=neumaier_sum(result.positions);
    return result;
}
}
