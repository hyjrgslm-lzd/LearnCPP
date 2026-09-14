#include "c13/common.hpp"
#include "concurrency_study/benchmark.hpp"
#include "concurrency_study/simd_kernels.hpp"
#include <array>
#include <cstdint>
#include <memory_resource>
#include <numeric>
#include <vector>

class allocation_counter final : public std::pmr::memory_resource {
public:
    std::size_t allocations{},bytes{};
private:
    void* do_allocate(std::size_t count,std::size_t alignment) override {
        void* p=std::pmr::new_delete_resource()->allocate(count,alignment);
        ++allocations; bytes+=count;
        return p;
    }
    void do_deallocate(void* p,std::size_t count,std::size_t alignment) override {
        std::pmr::new_delete_resource()->deallocate(p,count,alignment);
    }
    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override {return this==&other;}
};

int main(int argc,char** argv) {return c13::run([&] {
    cs::bench::arguments args(argc,argv);
    const auto n=args.number("--size",4096);
    args.finish();
    if(n>65536) throw std::invalid_argument("cost observation size cap is 65536");
    for(bool reserve:{false,true}) {
        allocation_counter resource;
        std::pmr::vector<int> values(&resource); // Destroyed before its resource.
        const auto ms=cs::bench::measure_ms([&] {
            if(reserve) values.reserve(n);
            for(std::size_t i=0;i<n;++i) values.push_back(static_cast<int>(i));
        });
        const auto sum=std::accumulate(values.begin(),values.end(),std::int64_t{0});
        check(sum==static_cast<std::int64_t>(n)*(static_cast<std::int64_t>(n)-1)/2,"allocation variants produce same values");
        if(reserve) check(resource.allocations==(n==0?0u:1u),"reserved vector allocates once for this nonempty fill");
        cs::bench::emit_row("allocation",reserve?"reserved":"growth",n,1,ms,n,
            "successful allocations="+std::to_string(resource.allocations)+"; requested bytes="+std::to_string(resource.bytes)
            +"; fill/reserve timed; teardown excluded; counters instrumented");
    }
    const std::array<float,7> input{-2,-0.0f,0,1,2,std::numeric_limits<float>::quiet_NaN(),-1};
    std::array<float,7> out{};
    for(auto backend:{cs::numeric::backend::scalar,cs::numeric::backend::sse2,cs::numeric::backend::xsimd}) {
        if(!cs::numeric::available(backend)) continue;
        out.fill(std::numeric_limits<float>::quiet_NaN());
        cs::numeric::conditional(input,out,cs::numeric::condition::relu,backend);
        for(std::size_t i=0;i<input.size();++i) {
            const float expected=input[i]>0?input[i]:0.0f;
            check(out[i]==expected && (out[i]!=0 || !std::signbit(out[i])),"ReLU preserves the declared NaN and signed-zero policy");
        }
    }
    constexpr std::size_t rows=127,cols=257;
    std::vector<double> matrix(rows*cols);
    for(std::size_t i=0;i<matrix.size();++i) matrix[i]=static_cast<double>(i%17);
    const double expected=std::accumulate(matrix.begin(),matrix.end(),0.0);
    for(bool column_first:{false,true}) {
        double sum=0;
        const double ms=cs::bench::measure_ms([&] {
            if(column_first) {
                for(std::size_t c=0;c<cols;++c) for(std::size_t r=0;r<rows;++r) sum+=matrix[r*cols+c];
            } else {
                for(std::size_t r=0;r<rows;++r) for(std::size_t c=0;c<cols;++c) sum+=matrix[r*cols+c];
            }
        });
        check(sum==expected,"row/column traversal covers same small integer-valued matrix");
        cs::bench::emit_row("stride",column_first?"column_first":"row_first",matrix.size(),1,ms,matrix.size(),
            "same row-major storage; traversal only; not measured cache events");
    }
});}
