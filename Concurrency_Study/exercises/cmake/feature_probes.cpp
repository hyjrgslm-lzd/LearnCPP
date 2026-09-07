#include <version>

#if defined(CS_HAS_STD_SIMD)
#include <simd>
#if !defined(__cpp_lib_simd) || __cpp_lib_simd < 202603L
#error Full N5050 SIMD interface is required by this lane
#endif
int main() {
    std::simd::vec<float, 4> v(1.0f);
    const auto selected = std::simd::select(v > v, v, v);
    return std::simd::reduce(selected) != 4.0f;
}
#elif defined(CS_HAS_STD_SENDERS)
#include <execution>
#include <tuple>
#if !defined(__cpp_lib_senders) || __cpp_lib_senders < 202506L
#error N5050 senders interface is required
#endif
int main() {
    auto result = std::this_thread::sync_wait(
        std::execution::just(1) | std::execution::then([](int x) { return x + 1; }));
    return !result || std::get<0>(*result) != 2;
}
#elif defined(CS_HAS_STD_HAZARD_POINTER)
#include <atomic>
#include <hazard_pointer>
#if !defined(__cpp_lib_hazard_pointer) || __cpp_lib_hazard_pointer < 202306L
#error Hazard pointer interface is required
#endif
struct node : std::hazard_pointer_obj_base<node> {};
int main() {
    std::atomic<node*> source{new node};
    auto hp = std::make_hazard_pointer();
    auto* protected_node = hp.protect(source);
    source.store(nullptr);
    hp.reset_protection();
    protected_node->retire();
}
#elif defined(CS_HAS_STD_RCU)
#include <rcu>
#if !defined(__cpp_lib_rcu) || __cpp_lib_rcu < 202306L
#error RCU interface is required
#endif
int main() {
    auto& domain = std::rcu_default_domain();
    domain.lock();
    domain.unlock();
    std::rcu_synchronize(domain);
    std::rcu_barrier(domain);
}
#elif defined(CS_HAS_ATOMIC_MIN_MAX)
#include <atomic>
#if !defined(__cpp_lib_atomic_min_max) || __cpp_lib_atomic_min_max < 202506L
#error N5050 atomic min/max interface is required
#endif
int main() {
    std::atomic<int> value{2};
    value.fetch_min(1);
    value.fetch_max(3);
    return value.load() != 3;
}
#elif defined(CS_HAS_INPLACE_STOP_TOKEN)
#include <stop_token>
int main() {
    std::inplace_stop_source source;
    auto token = source.get_token();
    std::inplace_stop_callback callback(token, []() noexcept {});
    source.request_stop();
    return !token.stop_requested() || std::never_stop_token{}.stop_requested();
}
#else
#error Select one feature
#endif
