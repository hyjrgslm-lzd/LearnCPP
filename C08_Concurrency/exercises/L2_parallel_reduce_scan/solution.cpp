#include "concurrency_study/numeric_kernels.hpp"
#include <array>
#include <iostream>
int main() {
    using cs::check;
    for (std::size_t n : {0,1,7,4097}) {
        std::vector<std::int64_t> x(n),inc_storage(n+2,-777),exc_storage(n+2,-777);
        std::span inc(inc_storage.data()+1,n),exc(exc_storage.data()+1,n);
        std::iota(x.begin(),x.end(),std::int64_t{1});
        const auto exact=std::int64_t(n)*std::int64_t(n+1)/2;
        check(std::accumulate(x.begin(),x.end(),std::int64_t{0})==exact,"left fold");
        check(std::reduce(x.begin(),x.end(),std::int64_t{0})==exact,"nonpolicy reduce");
        auto verify=[&] {
            check(inc_storage.front()==-777 && inc_storage.back()==-777 &&
                  exc_storage.front()==-777 && exc_storage.back()==-777,"scan write boundaries");
            for (std::size_t i=0;i<n;++i) {
                check(inc[i]==std::int64_t(i+1)*std::int64_t(i+2)/2,"every inclusive prefix");
                check(exc[i]==10+std::int64_t(i)*std::int64_t(i+1)/2,"every exclusive prefix");
            }
        };
        auto run_scans=[&](auto operation) {
            std::fill(inc.begin(),inc.end(),-12345);
            std::fill(exc.begin(),exc.end(),-12345);
            operation();
            verify();
        };
        run_scans([&] {
        std::inclusive_scan(x.begin(),x.end(),inc.begin());
        std::exclusive_scan(x.begin(),x.end(),exc.begin(),std::int64_t{10});
        });
#if CS_HAS_PARALLEL_ALGORITHMS
        check(std::reduce(std::execution::par,x.begin(),x.end(),std::int64_t{0})==exact,"par reduce");
        run_scans([&] {
        std::inclusive_scan(std::execution::par,x.begin(),x.end(),inc.begin());
        std::exclusive_scan(std::execution::par,x.begin(),x.end(),exc.begin(),std::int64_t{10});
        });
#endif
        if (n>0) {
            for (int skipped=0;skipped<2;++skipped) {
                bool detected=false;
                try { run_scans([&] {
                    if (skipped!=0) std::inclusive_scan(x.begin(),x.end(),inc.begin());
                    if (skipped!=1) std::exclusive_scan(x.begin(),x.end(),exc.begin(),std::int64_t{10});
                }); } catch (const std::runtime_error&) { detected=true; }
                check(detected,"skipping either scan must fail the same checker");
            }
        }
    }
    const std::array<float,5> a{1,2,3,4,5},b{2,3,4,5,6};
    const auto mul=[](float x,float y) noexcept { return double(x)*double(y); };
    check(std::transform_reduce(a.begin(),a.end(),b.begin(),0.0,std::plus<>{},mul)==70,"dot map then reduce");
#if CS_HAS_PARALLEL_ALGORITHMS
    check(std::transform_reduce(std::execution::par,a.begin(),a.end(),b.begin(),0.0,std::plus<>{},mul)==70,"par dot");
#endif
    // Exact affine composition is associative but not commutative; scan preserves order.
    struct affine { std::int64_t a,b; bool operator==(const affine&) const=default; };
    const auto compose=[](affine left,affine right) noexcept {
        return affine{right.a*left.a,right.a*left.b+right.b}; // right(left(x))
    };
    const std::array<affine,3> f{{{2,1},{3,4},{1,-2}}};
    std::array<affine,5> scan_storage{};
    std::span scan(scan_storage.data()+1,3);
    const std::array<affine,3> expected{{{2,1},{6,7},{6,5}}};
    auto run_affine=[&](auto operation) {
        scan_storage.fill(affine{-777,-777});
        std::fill(scan.begin(),scan.end(),affine{-12345,-12345});
        operation();
        check(scan_storage.front()==affine{-777,-777} && scan_storage.back()==affine{-777,-777},"affine boundaries");
        check(std::equal(scan.begin(),scan.end(),expected.begin()),"every affine prefix");
    };
    run_affine([&] { std::inclusive_scan(f.begin(),f.end(),scan.begin(),compose); });
#if CS_HAS_PARALLEL_ALGORITHMS
    run_affine([&] { std::inclusive_scan(std::execution::par,f.begin(),f.end(),scan.begin(),compose); });
#endif
    bool skipped_affine=false;
    try { run_affine([] {}); } catch (const std::runtime_error&) { skipped_affine=true; }
    check(skipped_affine,"skip-write affine fault must fail");
    volatile double large=1e20, minus=-1e20, one=1;
    const double left=(large+minus)+one, right=large+(minus+one);
    check(left==1 && right==0,"floating association can lose entire result");
    std::cout << "L2 Reference OK; policy capability=" << CS_HAS_PARALLEL_ALGORITHMS << '\n';
}
