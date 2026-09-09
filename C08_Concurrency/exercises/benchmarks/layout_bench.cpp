#include "../J1_false_sharing/reference.hpp"
#include "concurrency_study/benchmark.hpp"
#include <iostream>
int main(int argc,char** argv) try {
    cs::bench::arguments args(argc,argv);
    const auto n=args.number("--size",100000),batch=args.number("--batch",256),threads=args.number("--threads",2);
    const auto selected=args.text("--variant","all");
    args.finish();
    cs::check(n<=100'000'000 && batch>0 && threads==2,"layout requires size<=100M, batch>0, threads=2");
    if (selected!="all") (void)cs::layout::parse(selected);
    for (auto name:{"packed","padded","shared","batched"}) {
        if (selected!="all" && selected!=name) continue;
        cs::layout::counters counters;
        const auto variant=cs::layout::parse(name);
        const double ms=cs::bench::measure_ms([&] { counters.run(variant,n,batch); });
        counters.verify(variant,n,batch);
        const auto p0=reinterpret_cast<std::uintptr_t>(&counters.packed.value[0]);
        const auto p1=reinterpret_cast<std::uintptr_t>(&counters.packed.value[1]);
        cs::bench::emit_row("layout",name,n,2,ms,n*2,
            "includes zeroing/spawn/join; final-total contract; batch="+std::to_string(batch)+
            "; atomic_updates="+std::to_string(counters.published[0]+counters.published[1])+
            "; hint="+std::to_string(cs::layout::destructive)+"; packed_same_hint_region="+std::to_string(p0/cs::layout::destructive==p1/cs::layout::destructive)+
            "; atomic_lock_free="+std::to_string(counters.shared.is_lock_free()));
    }
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
