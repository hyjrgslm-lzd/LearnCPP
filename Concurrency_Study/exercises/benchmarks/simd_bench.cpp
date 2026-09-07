#include "concurrency_study/simd_kernels.hpp"
#include "concurrency_study/benchmark.hpp"
#include <iostream>
int main(int argc,char** argv) try {
    using namespace cs::numeric;
    cs::bench::arguments args(argc,argv);
    const auto n=args.number("--size",262147);
    const auto selected=args.text("--variant","all");
    args.finish();
    cs::check(n<=16'000'000,"SIMD size cap is 16 million");
    const std::array<std::string_view,11> names{"scalar","sse2","xsimd","std_simd","tail_mask","dot_scalar","dot_sse2","dot_xsimd","dot_std_simd","aos","soa"};
    cs::check(selected=="all" || std::find(names.begin(),names.end(),selected)!=names.end(),"unknown SIMD variant");
    std::vector<float> a(n),b(n,0.5f),out(n);
    for (std::size_t i=0;i<n;++i) a[i]=float(i%17);
    std::vector<particle> records;
    if (selected=="all" || selected=="aos") {
        records.resize(n);
        for (std::size_t i=0;i<n;++i) records[i]={a[i],1,2,b[i]};
    }
    bool ran=false;
    for (auto name:names) {
        if (selected!="all" && selected!=name) continue;
        const bool dot=name.starts_with("dot_");
        const bool layout=name=="soa" || name=="aos";
        const auto kind=name=="tail_mask" ? backend::sse2 : layout ? backend::scalar :
                        dot ? parse_backend(name.substr(4)) : parse_backend(name);
        if (!available(kind)) { std::cerr << "SKIP " << name << '\n'; continue; }
        double result=0;
        if (name=="soa") out=a; // Initialization excluded for the update operation.
        const double ms=cs::bench::measure_ms([&] {
            if (dot) result=cs::numeric::dot(a,b,kind);
            else if (name=="soa") update_soa(out,b,1);
            else if (name=="aos") update_aos(records,1);
            else if (name=="tail_mask") add_sse2_tail_mask(a,b,out);
            else add(a,b,out,kind);
        });
        if (dot) {
            const auto groups=n/17,remainder=n%17;
            const double exact=double(groups)*68+double(remainder)*double(remainder?remainder-1:0)/4;
            cs::check(result==exact,"measured dot analytic oracle");
        } else for (std::size_t i=0;i<n;++i) {
            cs::check((name=="aos"?records[i].x:out[i])==float(i%17)+0.5f,"measured add/update");
            if (name=="aos") cs::check(records[i].y==1 && records[i].z==2 && records[i].mass==0.5f,"AoS untouched fields");
        }
        cs::bench::emit_row(dot?"simd_dot":layout?"layout_update":"simd_add",name,n,1,ms,n,
            dot?"double accumulation; logical read bytes=8*n; allocation/check excluded":
                name=="aos"?"update x += mass; record stride=16; allocation/init/check excluded":
                "logical read+write bytes=12*n; allocation/init/check excluded; includes shape checks");
        ran=true;
    }
    return ran?0:77;
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
