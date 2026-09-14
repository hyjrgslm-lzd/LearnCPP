#pragma once
#include "c13/pipeline.hpp"
namespace student {
inline c13::pipeline_result run_pipeline(std::size_t n,unsigned seed,c13::milliseconds duration) {
    const float dt=c13::seconds_value(duration);
    const auto input=c13::make_particles(n,seed);
    c13::pipeline_result out{std::vector<double>(c13::checked_product(n,3)),0};
    for(std::size_t i=0;i<n;++i) {
        const auto p=input[i];
        const float x=p.x+p.vx*dt, y=p.y+p.vy*dt, z=p.z+p.vz*dt;
        out.positions[3*i]=-double(y);
        out.positions[3*i+1]=double(x);
        out.positions[3*i+2]=double(z);
    }
    out.sum=c13::neumaier_sum(out.positions);
    return out;
}
}
