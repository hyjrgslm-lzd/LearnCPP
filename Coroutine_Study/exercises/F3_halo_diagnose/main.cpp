// F-3 HALO 触发与失败诊断
// 文档参考：08-模块F-协程帧与allocator.md 「练习 F-3」
// 官方参考：
//   - Gor Nishanov "HALO: Heap Allocation eLision Optimization" CppCon 2018
//   - Clang `-Rpass=coroutine-elide` 文档
//   - libstdc++ / libc++ std::generator 实现源码
//
// 目标：写两版 generator 消费代码——一版可触发 HALO，一版故意破坏 HALO 前提。
//      用编译器 flag 诊断 HALO 是否生效，并实测两版的每次迭代耗时差异。

#include <coroutine>
#include <chrono>
#include <cstdio>
#include <cstdint>
#include <utility>
#include <exception>

// 注：std::generator (P2502) 在 libstdc++ 14+ / libc++ 19+ / MSVC STL 最新版中可用
// 此处提供一个最小 generator 仿制以便在尚不支持的环境也能编译运行
namespace demo {

template <typename T>
struct generator {
    struct promise_type {
        T current_value_;
        std::exception_ptr exc_;

        generator get_return_object() noexcept {
            return generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend()   noexcept { return {}; }
        std::suspend_always yield_value(T v) noexcept {
            current_value_ = std::move(v);
            return {};
        }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { exc_ = std::current_exception(); }
    };

    struct iterator {
        std::coroutine_handle<promise_type> h_;

        iterator& operator++() {
            h_.resume();
            if (h_.done()) h_ = nullptr;
            return *this;
        }
        T operator*() const { return h_.promise().current_value_; }
        bool operator==(std::default_sentinel_t) const { return !h_ || h_.done(); }
    };

    iterator begin() {
        if (h_) {
            h_.resume();
            if (h_.done()) return {nullptr};
        }
        return {h_};
    }
    std::default_sentinel_t end() { return {}; }

    explicit generator(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    generator(generator&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~generator() { if (h_) h_.destroy(); }

    std::coroutine_handle<promise_type> h_;
};

} // namespace demo

// ========== 版本 A：HALO 可触发 ==========
// 关键：generator 帧地址从不逃逸——begin/end 迭代中没有把 handle 抛到外部
demo::generator<int> simple_range(int n) {
    for (int i = 0; i < n; ++i) {
        co_yield i;
    }
}

// volatile sink 防止编译器把整段消费完全优化掉
static volatile std::int64_t g_sink = 0;

void consumer_A() {
    for (int v : simple_range(10)) {   // 范围 for —— 局部消费，不存任何引用
        g_sink += v;
    }
}

// ========== 版本 B：故意破坏 HALO 前提 ==========
// 把 generator 的指针存到全局——帧地址直接逃逸
demo::generator<int>* g_storage = nullptr;

demo::generator<int> leaker() {
    for (int i = 0; i < 10; ++i) {
        co_yield i * 10;
    }
}

void consumer_B() {
    auto g = leaker();
    g_storage = &g;             // 帧地址逃逸到全局变量！HALO 阻断
    for (int v : g) {
        g_sink += v;
    }
    g_storage = nullptr;
}

// ========== 性能实测 ==========
template <typename Fn>
double bench_ns(Fn&& fn, int iters) {
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < iters; ++i) fn();
    auto end = std::chrono::high_resolution_clock::now();
    auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    return static_cast<double>(ns) / iters;
}

int main()
{
    std::printf("===== F-3: HALO 触发与失败诊断 =====\n\n");

    // 预热
    consumer_A();
    consumer_B();

    constexpr int iters = 1'000'000;

    double ns_a = bench_ns(consumer_A, iters);
    double ns_b = bench_ns(consumer_B, iters);

    std::printf("consumer_A (HALO 可触发)     每次约 %.1f ns\n", ns_a);
    std::printf("consumer_B (HALO 被阻断)     每次约 %.1f ns\n", ns_b);
    std::printf("ratio B/A                    %.2fx\n", ns_b / ns_a);

    // TODO [必做]：用编译器 flag 诊断 HALO
    //   Clang: clang++ -std=c++23 -O2 -Rpass=coroutine-elide main.cpp
    //          期望 consumer_A 出现 "coroutine frame elided" remark
    //   GCC  : g++   -std=c++23 -O2 -fdump-tree-coro main.cpp
    //          检查 dump 里 simple_range 是否还有 operator new
    //   MSVC : Release build + 反汇编看 frame 分配点
    //
    // TODO [必做]：实测 ns_a 和 ns_b 是否有数量级差异
    //   预期：HALO 触发时 ratio > 5x；未触发时 < 2x
    //
    // TODO [进阶]：在 -O0/-O1/-O2/-O3 下分别跑一次，看 HALO 行为。
    // TODO [进阶]：再写一个版本 C——把 coroutine_handle 存到 std::vector 中，
    //   验证它也会破坏 HALO。

    // 防止 sink 被优化掉
    std::printf("\n[sink] %lld\n", static_cast<long long>(g_sink));
    return 0;
}
