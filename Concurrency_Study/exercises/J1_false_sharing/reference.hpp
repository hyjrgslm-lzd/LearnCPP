#pragma once
#include "concurrency_study/numeric_kernels.hpp"
#include <atomic>
#include <new>
#include <string_view>

namespace cs::layout {
#ifdef __cpp_lib_hardware_interference_size
inline constexpr auto destructive = std::hardware_destructive_interference_size;
inline constexpr auto constructive = std::hardware_constructive_interference_size;
inline constexpr bool implementation_hint = true;
#else
// Teaching layout only: 64 is an experiment parameter, not a hardware discovery.
inline constexpr std::size_t destructive = 64, constructive = 64;
inline constexpr bool implementation_hint = false;
#endif
struct alignas(destructive) packed_pair { std::atomic<std::uint64_t> value[2]{}; };
struct alignas(destructive) padded_counter { std::atomic<std::uint64_t> value{0}; };
struct hot_record { std::uint32_t id, count; std::uint64_t bytes; };
enum class variant { packed, padded, shared, batched };
inline variant parse(std::string_view s) {
    if (s=="packed") return variant::packed;
    if (s=="padded") return variant::padded;
    if (s=="shared") return variant::shared;
    if (s=="batched") return variant::batched;
    throw std::invalid_argument("unknown layout variant");
}
struct counters {
    packed_pair packed;
    padded_counter padded[2];
    std::atomic<std::uint64_t> shared{0};
    std::uint64_t published[2]{};

    void run(variant v,std::size_t iterations,std::size_t batch=256) {
        cs::check(batch>0,"batch must be positive");
        cs::check(iterations<=std::numeric_limits<std::uint64_t>::max()/2,"count overflow");
        for (int i=0;i<2;++i) { packed.value[i]=0; padded[i].value=0; published[i]=0; }
        shared=0;
        cs::numeric::parallel_chunks(2,2,[&](auto t,auto,auto) {
            auto* target = v==variant::packed ? &packed.value[t] :
                           v==variant::padded ? &padded[t].value : &shared;
            if (v==variant::batched) {
                // Automatic storage owned by this worker; not TLS lookup overhead.
                std::uint64_t local=0;
                for (std::size_t i=0;i<iterations;++i) {
                    if (++local==batch) { target->fetch_add(local,std::memory_order_relaxed); local=0; ++published[t]; }
                }
                if (local) { target->fetch_add(local,std::memory_order_relaxed); ++published[t]; }
            } else {
                for (std::size_t i=0;i<iterations;++i) target->fetch_add(1,std::memory_order_relaxed);
                published[t]=iterations;
            }
        });
    }
    void verify(variant v,std::size_t iterations,std::size_t batch=256) const {
        cs::check(batch>0,"verification batch must be positive");
        if (v==variant::packed || v==variant::padded) {
            for (int t=0;t<2;++t)
                cs::check((v==variant::packed ? packed.value[t].load() : padded[t].value.load())==iterations,
                          "each private counter completes every event");
        } else cs::check(shared.load()==2*iterations,"shared total after join");
        const auto expected=v==variant::batched ? iterations/batch+(iterations%batch!=0) : iterations;
        cs::check(published[0]==expected && published[1]==expected,"publication count incl tail");
    }
};
} // namespace cs::layout
