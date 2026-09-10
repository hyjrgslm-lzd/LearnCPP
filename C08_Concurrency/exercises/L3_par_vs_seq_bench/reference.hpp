#pragma once
#include "checks.hpp"
#include "concurrency_study/benchmark.hpp"
#include <iostream>
namespace cs::policy_bench {
inline int run(int argc,char** argv) {
    cs::bench::arguments args(argc,argv);
    const auto n=args.number("--size",65537);
    const auto variant=args.text("--variant","all");
    const auto workload=args.text("--workload","light");
    args.finish();
    cs::check(n<=16'000'000,"size cap is 16 million elements");
    cs::check(workload=="light" || workload=="heavy","workload must be light or heavy");
    const bool heavy=workload=="heavy";
    if (variant!="all") (void)cs::numeric::parse_policy(variant);
    std::vector<float> input(n),out(n);
    for (std::size_t i=0;i<n;++i) input[i]=heavy?float(int(i%65)-32)/64.0f:float(int(i%33)-16);
    bool ran=false;
    for (const auto name : {"plain","seq","par","unseq","par_unseq"}) {
        if (variant!="all" && variant!=name) continue;
        const auto p=cs::numeric::parse_policy(name);
        if (!cs::numeric::policy_available(p)) { std::cerr << "SKIP " << name << '\n'; continue; }
        std::fill(out.begin(),out.end(),-12345.0f);
        const double ms=cs::bench::measure_ms([&] { cs::numeric::map(input,out,p,heavy); });
        verify(input,out,heavy);
        cs::bench::emit_row(heavy?"policy_map_heavy":"policy_map_light",name,n,
            (p==cs::numeric::policy::par || p==cs::numeric::policy::par_unseq)?0:1,ms,n,
            "map and contract validation included; allocation/init/oracle excluded; workload="+workload+
            "; threads=0: backend unmeasured; threads=1: caller; geometric_degree=96 for heavy");
        ran=true;
    }
    return ran?0:77;
}
}
