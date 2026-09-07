#pragma once
#include "concurrency_study/simd_kernels.hpp"
#include "concurrency_study/benchmark.hpp"
#include <string>

namespace cs::cap3 {
inline void fill_matrices(cs::numeric::output a,cs::numeric::output b,std::size_t n) {
    for (std::size_t i=0;i<n;++i)
        for (std::size_t j=0;j<n;++j) {
            a[i*n+j]=float(int((i+2*j)%7)-3);
            b[i*n+j]=float(int((3*i+j)%5)-2);
        }
}
// Independent integer arithmetic oracle for the generated small-integer domain.
// This validates all outputs, not only C[0] or a checksum which can cancel errors.
inline void verify_matrix(cs::numeric::input c,std::size_t n) {
    cs::check(c.size()==cs::numeric::square_size(n),"oracle extent");
    for (std::size_t i=0;i<n;++i)
        for (std::size_t j=0;j<n;++j) {
            std::int64_t expected=0;
            for (std::size_t k=0;k<n;++k)
                expected+=(int((i+2*k)%7)-3)*(int((3*k+j)%5)-2);
            cs::check(c[i*n+j]==float(expected),"GEMM integer oracle");
        }
}
inline void compute(std::string_view variant,cs::numeric::input a,cs::numeric::input b,
                    cs::numeric::output c,std::size_t n,std::size_t block,std::size_t threads) {
    using namespace cs::numeric;
    if (variant=="naive") gemm_naive(a,b,c,n);
    else if (variant=="tiled") gemm_tiled(a,b,c,n,block);
    else if (variant=="threaded") gemm_threaded(a,b,c,n,block,threads);
    else if (variant=="par") gemm_par(a,b,c,n,block);
    else if (variant=="sse2") gemm_sse2(a,b,c,n,block);
    else throw std::invalid_argument("unknown GEMM variant");
}
inline int run(int argc,char** argv) {
    using namespace cs::numeric;
    cs::bench::arguments args(argc,argv);
    const auto n=args.number("--size",48),block=args.number("--block",16);
    const auto threads=args.number("--threads",2),items=args.number("--items",65537);
    const auto selected=args.text("--variant","all");
    args.finish();
    cs::check(n<=512 && items<=16'000'000 && block>0 && threads>0 && threads<=32,"Cap3 argument limit");
    const std::array<std::string_view,10> names{"naive","tiled","threaded","par","sse2",
        "reduce_plain","reduce_par","reduce_threaded","sort_plain","sort_par"};
    cs::check(selected=="all" || std::find(names.begin(),names.end(),selected)!=names.end(),"unknown Cap3 variant");
    std::vector<float> a(square_size(n)),b(a.size()),c(a.size());
    fill_matrices(a,b,n);
    std::vector<float> values(items,0.5f);
    std::vector<int> base(items);
    for (std::size_t i=0;i<items;++i) base[i]=int((i*17+3)%101);
    std::array<std::size_t,101> histogram{};
    for (int x:base) ++histogram[std::size_t(x)];
    bool ran=false;
    for (auto name:names) {
        if (selected!="all" && selected!=name) continue;
        if (((name=="par" || name=="reduce_par" || name=="sort_par") && !policy_available(policy::par)) ||
            (name=="sse2" && !available(backend::sse2))) { std::cerr << "SKIP " << name << '\n'; continue; }
        if (name.starts_with("reduce_")) {
            double result=0;
            const double ms=cs::bench::measure_ms([&] {
                result=name=="reduce_threaded" ? sum_threaded(values,threads) : sum_policy(values,name=="reduce_par");
            });
            cs::check(result==double(items)*0.5,"measured reduction analytic oracle");
            const auto workers=name=="reduce_threaded"?std::min(items,threads):0;
            cs::bench::emit_row("cap3_reduce",name,items,name=="reduce_par"?0:std::max<std::size_t>(workers,1),
                ms,items,"double accumulation; creation/join included; validation excluded; caller=1; workers="+
                (name=="reduce_par"?std::string("unmeasured"):std::to_string(workers))+
                "; threads counts workers when nonzero, otherwise caller (backend unknown=0)");
        } else if (name.starts_with("sort_")) {
            auto work=base; // Same unsorted input; copy excluded for every version.
            const double ms=cs::bench::measure_ms([&] { sort_values(work,name=="sort_par"); });
            cs::check(std::is_sorted(work.begin(),work.end()),"sort order");
            std::array<std::size_t,101> got{};
            for (int x:work) { cs::check(x>=0 && x<=100,"sort range"); ++got[std::size_t(x)]; }
            cs::check(got==histogram,"sort multiplicities");
            cs::bench::emit_row("cap3_sort",name,items,name=="sort_par"?0:1,ms,items,
                name=="sort_par"?"sort only; copy/check excluded; caller=1; workers=unmeasured; threads=0: backend unmeasured":
                "sort only; copy/check excluded; caller=1; workers=0; threads=1: caller");
        } else {
            const double ms=cs::bench::measure_ms([&] { compute(name,a,b,c,n,block,threads); });
            verify_matrix(c,n);
            const auto workers=name=="threaded"?std::min(n,threads):0;
            cs::bench::emit_row("cap3_gemm",name,n,name=="par"?0:std::max<std::size_t>(workers,1),
                ms,n*n,"C overwrite/zeroing and worker lifecycle included; block="+std::to_string(block)+
                "; FLOP=2*n^3; caller=1; workers="+(name=="par"?std::string("unmeasured"):std::to_string(workers))+
                "; threads counts workers when nonzero, otherwise caller (backend unknown=0)");
        }
        ran=true;
    }
    return ran?0:77;
}
}
