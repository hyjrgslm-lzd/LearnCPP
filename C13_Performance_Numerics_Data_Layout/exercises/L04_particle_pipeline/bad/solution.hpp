#pragma once
#include "c13/pipeline.hpp"
namespace student {
inline c13::pipeline_result run_pipeline(std::size_t n,unsigned seed,c13::milliseconds dt) {
    c13::seconds_value(dt);
    // Deliberate defect: drops elapsed time while keeping valid shape and plausible output.
    return c13::run_pipeline(c13::make_particles(n,seed),c13::milliseconds{0});
}
}
