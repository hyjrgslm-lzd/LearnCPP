#include "concurrency_study/simd_kernels.hpp"
#include "../J1_false_sharing/reference.hpp"
#include <array>
#include <bit>
#include <cfenv>
#include <iostream>
#include <memory>
#if defined(__FAST_MATH__) || defined(_M_FP_FAST)
#error numeric_test contains a strict oracle; compile it without fast-math
#endif

namespace {
using namespace cs::numeric;
template<class F> void rejects(F&& f) {
    bool rejected=false;
    try { f(); } catch (const std::exception&) { rejected=true; }
    cs::check(rejected,"invalid input must be rejected before accessing memory");
}
void same_value(float got,float expected) {
    cs::check(std::isnan(expected)?std::isnan(got):got==expected,"IEEE value/classification");
    if (expected==0) cs::check(std::signbit(got)==std::signbit(expected),"zero sign");
}
template<class F,class V> void checked_write(output out,F&& operation,V&& verify) {
    // -12345 is outside every expected output domain below, including IEEE tables.
    std::fill(out.begin(),out.end(),-12345.0f);
    operation();
    verify();
}
// An independent compensated oracle. MSVC long double may equal double, so
// increased precision is NOT assumed. Binary32 products are exact in binary64.
long double compensated_dot(input a,input b) {
    long double s=0,c=0;
    for (std::size_t i=0;i<a.size();++i) {
        const long double product=static_cast<long double>(a[i])*static_cast<long double>(b[i]);
        const long double t=s+product;
        c+=std::abs(s)>=std::abs(product)?(s-t)+product:(product-t)+s;
        s=t;
    }
    return s+c;
}
long double dot_bound(input a,input b) {
    long double magnitude=0;
    for (std::size_t i=0;i<a.size();++i) magnitude+=std::abs(static_cast<long double>(a[i])*b[i]);
    const long double u=std::numeric_limits<double>::epsilon()/2;
    const long double k=static_cast<long double>(a.size()+8);
    cs::check(k*u<0.5L,"gamma bound domain");
    return 2*k*u/(1-k*u)*magnitude; // Also budgets rounding in the compensated oracle.
}
void memory_and_add() {
    for (auto kind:{backend::scalar,backend::sse2,backend::xsimd,backend::std_simd}) {
        if (!available(kind)) { std::cout << "SKIP backend " << int(kind) << '\n'; continue; }
        for (std::size_t n:{0,1,2,3,4,5,7,8,9,15,16,17,31,33,257})
            for (std::size_t offset:{0,1,2,3}) {
                // Exact end of allocation exposes overreads to ASan; canaries expose writes.
                auto a=std::make_unique<float[]>(n+offset);
                auto b=std::make_unique<float[]>(n+offset);
                std::vector<float> storage(n+2,-12345);
                for (std::size_t i=0;i<n;++i) { a[offset+i]=float(i); b[offset+i]=0.25f; }
                input av(a.get()+offset,n),bv(b.get()+offset,n);
                output out(storage.data()+1,n);
                auto verify=[&] {
                for (std::size_t i=0;i<n;++i) cs::check(out[i]==float(i)+0.25f,"analytic add all lengths/offsets");
                cs::check(storage.front()==-12345 && storage.back()==-12345,"write canaries");
                };
                checked_write(out,[&] { add(av,bv,out,kind); },verify);
                if (kind==backend::sse2) {
                    checked_write(out,[&] { add_sse2_tail_mask(av,bv,out); },verify);
                    if (n>0) rejects([&] { checked_write(out,[] {},verify); });
                }
            }
    }
    alignas(16) float a[12]{},b[12]{},c[12]{};
    rejects([&]{ add(input(a,4),input(b,3),output(c,4),backend::scalar); });
    rejects([&]{ add(input(a,4),input(b,4),output(c,3),backend::scalar); });
    rejects([&]{ add(input(a,4),input(b,4),output(a,4),backend::scalar); });
    rejects([&]{ add(input(a,4),input(b,4),output(a+1,4),backend::scalar); });
    if (available(backend::sse2)) {
        checked_write(c,[&] { add(a,b,c,backend::sse2,true); },[&] {
            for (float value:c) cs::check(value==0,"aligned zero output overwrites sentinel");
        });
        rejects([&]{ add(input(a+1,4),input(b,4),output(c,4),backend::sse2,true); });
        rejects([&]{ add(input(a,4),input(b,4),output(c+1,4),backend::sse2,true); });
    }
}
void permutations() {
    const std::array<float,4> values{10,20,30,40};
    const std::array<std::size_t,4> order{3,0,2,1},duplicate{0,1,0,3},bad{0,1,4,3};
    std::array<float,4> out{};
    out.fill(-12345);
    gather(values,order,out);
    cs::check(out==std::array<float,4>{40,10,30,20},"gather oracle");
    out.fill(-12345);
    gather(values,duplicate,out);
    cs::check(out==std::array<float,4>{10,20,10,40},"duplicate gather permitted");
    out.fill(-1);
    rejects([&]{ gather(values,bad,out); });
    cs::check(out==std::array<float,4>{-1,-1,-1,-1},"gather fails before writes");
    rejects([&]{ scatter_unique(values,duplicate,out); });
    rejects([&]{ scatter_unique(values,bad,out); });
    cs::check(out==std::array<float,4>{-1,-1,-1,-1},"scatter rejects entire index set before writes");
    out.fill(-12345);
    scatter_unique(values,order,out);
    cs::check(out==std::array<float,4>{20,40,30,10},"scatter oracle");
    gather({}, {}, {}); scatter_unique({}, {}, {});
    if (available(backend::sse2)) {
        out.fill(-12345);
        gather_add_sse2(values,order,out);
        cs::check(out==std::array<float,4>{41,11,31,21},"gather then real SIMD arithmetic");
    }
}
void precision() {
    for (std::size_t n:{0,1,3,4,5,31,257,1003}) {
        std::vector<float> a(n),b(n);
        for (std::size_t i=0;i<n;++i) {
            a[i]=std::ldexp(float(int(i%19)-9),int(i%31)-15);
            b[i]=float(int(i%13)-6)/7;
        }
        const auto truth=compensated_dot(a,b),bound=dot_bound(a,b);
        cs::check(std::abs(static_cast<long double>(dot_scalar(a,b))-truth)<=bound,"scalar operation-aware bound");
        if (available(backend::sse2))
            cs::check(std::abs(static_cast<long double>(dot_sse2(a,b))-truth)<=bound,"SIMD operation-aware bound");
        for (auto kind:{backend::xsimd,backend::std_simd})
            if (available(kind)) cs::check(std::abs(static_cast<long double>(dot(a,b,kind))-truth)<=bound,"optional dot operation-aware bound");
    }
    // Exact cancellation oracle survives even on implementations with 64-bit long double.
    const std::array<float,8> a{0x1p60f,0x1p-60f,-0x1p60f,3,0x1p40f,2,-0x1p40f,-1};
    const std::array<float,8> b{0x1p-60f,0x1p60f,0x1p-60f,1,0x1p-40f,1,0x1p-40f,1};
    cs::check(dot_scalar(a,b)==5,"large range analytic dot");
    if (available(backend::sse2)) cs::check(dot_sse2(a,b)==5,"large range SIMD oracle");
    std::array<float,1> maximum{std::numeric_limits<float>::max()};
    const double exact=double(maximum[0])*double(maximum[0]);
    cs::check(std::isfinite(exact) && dot_scalar(maximum,maximum)==exact,"widen before product avoids float overflow");
    if (available(backend::sse2)) {
        const std::array<float,4> wide{maximum[0],maximum[0],maximum[0],maximum[0]};
        cs::check(dot_sse2(wide,wide)==4*exact,"wide vector product before sum");
    }
}
void ieee_policy() {
    cs::check(std::numeric_limits<float>::is_iec559 && std::numeric_limits<double>::digits>=53,"IEEE binary32/64 test domain");
    cs::check(std::fegetround()==FE_TONEAREST,"strict suite requires round-to-nearest");
#if CS_NUMERIC_SSE2
    cs::check((_mm_getcsr() & ((1u<<15)|(1u<<6)))==0,"strict suite requires FTZ/DAZ off");
#endif
    const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    const float tiny=std::numeric_limits<float>::denorm_min();
    const std::array<float,12> a{nan,inf,-inf,-0.0f,0,tiny,-tiny,-2,2,0.5f,-0.5f,nan};
    const std::array<float,12> ab{nan,inf,inf,0,0,tiny,tiny,2,2,0.5f,0.5f,nan};
    const std::array<float,12> cl{nan,1,-1,-0.0f,0,tiny,-tiny,-1,1,0.5f,-0.5f,nan};
    const std::array<float,12> re{0,inf,0,0,0,tiny,0,0,2,0.5f,0,0};
    for (auto kind:{backend::scalar,backend::sse2,backend::xsimd,backend::std_simd}) {
        if (!available(kind)) continue;
        for (std::size_t n=0;n<=a.size();++n) {
            std::vector<float> out(n);
            for (auto op:{condition::absolute,condition::clamp,condition::relu}) {
                std::fill(out.begin(),out.end(),-12345.0f);
                conditional(input(a).first(n),out,op,kind);
                const auto& expected=op==condition::absolute?ab:op==condition::clamp?cl:re;
                for (std::size_t i=0;i<n;++i) same_value(out[i],expected[i]);
            }
        }
        const std::array<float,8> x{inf,inf,nan,-0.0f,tiny,-tiny,0,1},y{1,-inf,0,-0.0f,tiny,-tiny,-0.0f,inf};
        const std::array<float,8> expected{inf,nan,nan,-0.0f,2*tiny,-2*tiny,0,inf};
        std::array<float,8> out{};
        out.fill(-12345);
        add(x,y,out,kind);
        for (std::size_t i=0;i<8;++i) same_value(out[i],expected[i]);
    }
    const std::array<float,4> nonfinite{inf,-inf,1,1},ones{1,1,1,1};
    cs::check(std::isnan(dot_scalar(nonfinite,ones)),"nonfinite dot classification only");
    if (available(backend::sse2)) cs::check(std::isnan(dot_sse2(nonfinite,ones)),"nonfinite SIMD dot classification");
}
void matrix_and_workers() {
    for (std::size_t n:{0,1,3,5,17}) {
        std::vector<float> a(square_size(n)),b(a.size()),out(a.size());
        for (std::size_t i=0;i<a.size();++i) { a[i]=float(int(i%7)-3)/7; b[i]=float(int(i%5)-2)/3; }
        auto verify=[&] {
            for (std::size_t i=0;i<n;++i) for (std::size_t j=0;j<n;++j) {
                long double truth=0,magnitude=0;
                for (std::size_t k=0;k<n;++k) { const long double term=static_cast<long double>(a[i*n+k])*b[k*n+j]; truth+=term; magnitude+=std::abs(term); }
                const long double u=std::numeric_limits<float>::epsilon()/2,k=2*static_cast<long double>(n)+4;
                cs::check(std::abs(static_cast<long double>(out[i*n+j])-truth)<=k*u/(1-k*u)*magnitude,"GEMM per-output forward error bound");
            }
        };
        checked_write(out,[&] { gemm_naive(a,b,out,n); },verify);
        for (std::size_t block:{1,4,32}) {
            checked_write(out,[&] { gemm_tiled(a,b,out,n,block); },verify);
            checked_write(out,[&] { gemm_threaded(a,b,out,n,block,3); },verify);
            if (policy_available(policy::par)) checked_write(out,[&] { gemm_par(a,b,out,n,block); },verify);
            if (available(backend::sse2)) checked_write(out,[&] { gemm_sse2(a,b,out,n,block); },verify);
        }
        if (n>0) rejects([&] { checked_write(out,[] {},verify); });
    }
    rejects([]{ gemm_tiled({}, {}, {},0,0); });
    rejects([]{ gemm_threaded({}, {}, {},0,1,0); });
    rejects([]{ square_size(std::numeric_limits<std::size_t>::max()); });
    rejects([]{ parallel_chunks(7,2,[](auto,auto,auto) { throw std::runtime_error("worker"); }); });
    std::array<float,5> values{1,2,3,4,5};
    for (std::size_t t:{1,2,8}) cs::check(sum_threaded(values,t)==15,"thread-local sums, short chunks");
    cs::check(sum_threaded({},2)==0,"empty threaded reduction");
    std::array<particle,3> aos{{{1,2,3,2},{2,3,4,4},{3,4,5,6}}};
    std::array<float,3> x{1,2,3},mass{2,4,6};
    update_aos(aos,0.5f); update_soa(x,mass,0.5f);
    for (std::size_t i=0;i<3;++i) cs::check(x[i]==float(2*(i+1)) && aos[i].x==x[i],"AoS/SoA same update");
}
}
int main() try {
    memory_and_add(); permutations(); precision(); ieee_policy(); matrix_and_workers();
    std::cout << "numeric_test OK: lengths, tails, alignment, permutations, precision, IEEE policy, GEMM, workers\n";
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
