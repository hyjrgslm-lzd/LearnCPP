#pragma once
#include "c13/pipeline.hpp"
namespace student {
inline c13::pipeline_result run_pipeline(std::size_t n,unsigned seed,c13::milliseconds dt) {
    return c13::run_pipeline(c13::make_particles(n,seed),dt);
}
}
