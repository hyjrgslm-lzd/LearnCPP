// F-3 HALO diagnosis starter.
// Timing is a signal only. HALO claims need compiler output.

#include <chrono>
#include <charconv>
#include <coroutine>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <iterator>
#include <new>
#include <string_view>
#include <utility>
#include "coroutine_study/exercise_check.hpp"

using coroutine_study::check;

namespace demo {

template <class T>
struct generator {
    struct promise_type {
        T current{};
        std::exception_ptr error;
#ifndef F3_UNINSTRUMENTED_HALO
        static inline std::uint64_t allocations = 0;
        static inline std::uint64_t deallocations = 0;

        static void* operator new(std::size_t n) {
            ++allocations;
            return ::operator new(n);
        }
        static void operator delete(void* p, std::size_t) noexcept {
            ++deallocations;
            ::operator delete(p);
        }
#endif
        generator get_return_object() noexcept {
            return generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend() noexcept { return {}; }
        std::suspend_always yield_value(T v) noexcept {
            current = std::move(v);
            return {};
        }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { error = std::current_exception(); }
    };

    struct iterator {
        std::coroutine_handle<promise_type> h{};
        iterator& operator++() {
            h.resume();
            if (h.done()) {
                if (h.promise().error) std::rethrow_exception(h.promise().error);
                h = {};
            }
            return *this;
        }
        T operator*() const noexcept { return h.promise().current; }
        bool operator==(std::default_sentinel_t) const noexcept { return !h; }
    };

    explicit generator(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    generator(generator&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    generator(const generator&) = delete;
    ~generator() { if (h_) h_.destroy(); }

    iterator begin() {
        if (!h_) return {};
        h_.resume();
        if (h_.done()) return {};
        return {h_};
    }
    std::default_sentinel_t end() const noexcept { return {}; }

    std::coroutine_handle<promise_type> h_{};
};

} // namespace demo

static volatile std::int64_t sink = 0;
static demo::generator<int>* escaped_generator = nullptr;

demo::generator<int> range_values(int n) {
    for (int i = 0; i < n; ++i) co_yield i;
}

std::int64_t local_consume(int n) {
    std::int64_t sum = 0;
    for (int v : range_values(n)) sum += v;
    return sum;
}

std::int64_t escaped_consume(int n) {
    auto g = range_values(n);
    escaped_generator = &g;
    std::int64_t sum = 0;
    for (int v : g) sum += v;
    escaped_generator = nullptr;
    return sum;
}

using clock_type = std::chrono::steady_clock;
using bench_fn = std::int64_t (*)(int);

double bench_ns(bench_fn fn, int n, int iterations) {
    auto start = clock_type::now();
    std::int64_t total = 0;
    for (int i = 0; i < iterations; ++i) total += fn(n);
    auto end = clock_type::now();
    const auto expected = static_cast<std::int64_t>(n - 1) * n / 2 * iterations;
    check(total == expected, "timed workload checksum mismatch");
    sink += total;
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    return static_cast<double>(ns) / static_cast<double>(iterations);
}

bench_fn select_version(std::string_view version) {
    if (version == "local") return local_consume;
    if (version == "escaped") return escaped_consume;
    return nullptr;
}

int run_one(std::string_view version, int n, int iterations) {
    auto fn = select_version(version);
    check(fn != nullptr, "version must be local or escaped");
    auto expected = static_cast<std::int64_t>(n - 1) * n / 2;
    check(fn(n) == expected, "selected version must compute the same sum");
    double ns = bench_ns(fn, n, iterations);
    std::printf("version=%.*s,n=%d,iterations=%d,ns_per_iter=%.3f,sink=%lld\n",
                static_cast<int>(version.size()), version.data(), n, iterations, ns,
                static_cast<long long>(sink));
    return 0;
}

int positive_argument(std::string_view text, int maximum) {
    int value = 0;
    auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    check(error == std::errc{} && end == text.data() + text.size() && value > 0 && value <= maximum,
          "benchmark arguments must be bounded positive integers");
    return value;
}

int main(int argc, char** argv) try {
    int n = 32;
    int iterations = 200000;

    if (argc == 5 && std::string_view{argv[1]} == "--bench") {
        auto version = std::string_view{argv[2]};
        n = positive_argument(argv[3], 4096);
        iterations = positive_argument(argv[4], 10000000);
        check(static_cast<std::int64_t>(n) * iterations <= 100000000,
              "benchmark workload exceeds the 100 million element budget");
        return run_one(version, n, iterations);
    }
    check(argc == 1, "usage: F3_halo_diagnose [--bench local|escaped n iterations]");

    auto expected = static_cast<std::int64_t>(n - 1) * n / 2;
    check(local_consume(n) == expected, "local version computes expected sum");
    check(escaped_consume(n) == expected, "escaped version computes expected sum");

    std::printf("F3 starter correctness OK\n");
#ifndef F3_UNINSTRUMENTED_HALO
    check(demo::generator<int>::promise_type::allocations == demo::generator<int>::promise_type::deallocations,
          "instrumented generator allocations must be paired");
    std::printf("instrumented generator allocations=%llu deallocations=%llu\n",
                static_cast<unsigned long long>(demo::generator<int>::promise_type::allocations),
                static_cast<unsigned long long>(demo::generator<int>::promise_type::deallocations));
#endif
    std::printf("Run sample_f3.py with this executable for 1 warmup + 5 independent processes.\n");
    std::printf("Use compiler remark/IR/assembly for HALO; timing alone is not proof.\n");
    return 0;
} catch (const std::exception& e) {
    std::fprintf(stderr, "starter check failed: %s\n", e.what());
    return 1;
}
