#include "c13/layout.hpp"
#include "c13/common.hpp"
#include "concurrency_study/benchmark.hpp"
int main(int argc,char** argv) { return c13::run([&] {
    cs::bench::arguments args(argc,argv);
    const auto n=args.number("--size",100'000), steps=args.number("--steps",10);
    args.finish();
    if (steps==0 || steps>1000) throw std::invalid_argument("steps must be in [1,1000]");
    const auto source=c13::make_particles(n);
    auto aos=source;
    const double aos_ms=cs::bench::measure_ms([&] {
        for(std::size_t s=0;s<steps;++s) c13::advance_aos(aos,0.01f);
    });
    c13::particles_soa soa;
    const double convert_ms=cs::bench::measure_ms([&] { soa=c13::to_soa(source); });
    const double soa_ms=cs::bench::measure_ms([&] {
        for(std::size_t s=0;s<steps;++s) c13::advance_soa(soa,0.01f);
    });
    for(std::size_t i=0;i<n;++i)
        c13::check(c13::near(aos[i].x,soa.x[i],1e-6,1e-6)
           && c13::near(aos[i].y,soa.y[i],1e-6,1e-6)
           && c13::near(aos[i].z,soa.z[i],1e-6,1e-6),"timed variants agree");
    const auto completed=c13::checked_product(n,steps);
    cs::bench::emit_row("layout","aos_kernel",n,1,aos_ms,completed,"positions updated; allocation outside timing");
    cs::bench::emit_row("layout","aos_to_soa",n,1,convert_ms,n,"six allocations and field copies included");
    cs::bench::emit_row("layout","soa_kernel",n,1,soa_ms,completed,"same steps and dt as aos_kernel");
    cs::bench::emit_row("layout","soa_convert_plus_kernel",n,1,convert_ms+soa_ms,completed,
                       "sum of adjacent phase timings; output stays SoA; not whole process latency");
}); }
