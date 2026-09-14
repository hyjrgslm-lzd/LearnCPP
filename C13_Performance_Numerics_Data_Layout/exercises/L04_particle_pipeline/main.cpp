#include "c13/pipeline.hpp"
#include "concurrency_study/benchmark.hpp"
int main(int argc,char** argv) { return c13::run([&] {
    cs::bench::arguments args(argc,argv);
    const auto n=args.number("--size",10000),seed=args.number("--seed",20260914);
    args.finish();
    if(seed>std::numeric_limits<unsigned>::max()) throw std::invalid_argument("seed exceeds unsigned range");
    const auto input=c13::make_particles(n,static_cast<unsigned>(seed));
    c13::pipeline_result result;
    const auto elapsed=cs::bench::measure_ms([&] {result=c13::run_pipeline(input,c13::milliseconds{16});});
    check(result.positions.size()==c13::checked_product(n,3) && std::isfinite(result.sum),"pipeline integrity");
    // Consume all output outside timing, with a direct scalar oracle independent of the core pipeline.
    for(std::size_t i=0;i<n;++i) {
        auto p=input[i];
        const float x=p.x+p.vx*0.016f,y=p.y+p.vy*0.016f,z=p.z+p.vz*0.016f;
        check(c13::near(result.positions[3*i],-double(y),2e-6,2e-6)
           && c13::near(result.positions[3*i+1],double(x),2e-6,2e-6)
           && c13::near(result.positions[3*i+2],double(z),2e-6,2e-6),"pipeline independent position oracle");
    }
    cs::bench::emit_row("particle_pipeline","input_to_result",n,1,elapsed,n,
        "conversion/update/3x3 transform/reduction and temporary cleanup included; RNG/output destruction excluded");
}); }
