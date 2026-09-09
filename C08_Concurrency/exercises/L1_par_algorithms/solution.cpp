#include "concurrency_study/numeric_kernels.hpp"
#include <array>
#include <iostream>
int main() {
    using namespace cs::numeric;
    for (auto p : {policy::plain,policy::seq,policy::par,policy::unseq,policy::par_unseq}) {
        if (!policy_available(p)) { std::cout << "SKIP policy " << int(p) << '\n'; continue; }
        for (std::size_t n : {0,1,7,4097}) {
            std::vector<float> a(n),out(n);
            for (std::size_t i=0;i<n;++i) a[i]=float(int(i%33)-16);
            map(a,out,p);
            for (std::size_t i=0;i<n;++i) {
                const int x=int(i%33)-16;
                cs::check(out[i]==float(x*x+2),"independent integer oracle");
            }
        }
    }
    for (bool par : {false,true}) {
        if (par && !policy_available(policy::par)) continue;
        std::array<int,7> a{3,-1,3,2,0,-1,2};
        sort_values(a,par);
        cs::check(a==std::array<int,7>{-1,-1,0,2,2,3,3},"sort values and multiplicity");
    }
    std::array<int,4> v{1,2,3,4};
#if CS_HAS_PARALLEL_ALGORITHMS
    std::for_each(std::execution::par,v.begin(),v.end(),[](int& x) noexcept { x*=2; });
#else
    std::for_each(v.begin(),v.end(),[](int& x) noexcept { x*=2; });
#endif
    cs::check(v==std::array<int,4>{2,4,6,8},"disjoint for_each writes");
    bool caught=false;
    try { std::for_each(v.begin(),v.end(),[](int) { throw std::runtime_error("ordinary callback"); }); }
    catch (const std::runtime_error&) { caught=true; }
    cs::check(caught,"ordinary overload propagates callback exception");
    // Policy overload throwing is intentionally not executed: it calls terminate.
    std::cout << "L1 Reference OK\n";
}
