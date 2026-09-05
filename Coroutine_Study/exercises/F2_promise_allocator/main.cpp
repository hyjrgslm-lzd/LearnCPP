// F-2 自定义 promise allocator（P0912）
// 文档参考：08-模块F-协程帧与allocator.md 「练习 F-2」
// 官方参考：
//   - P0912R5 "Merge Coroutines TS into C++20 working draft"
//   - cppreference: coroutine_traits / promise_type::operator new
//   - Lewis Baker "Custom allocators for C++ coroutines"
//
// 目标：在 promise_type 中重载 operator new / operator delete，让协程帧
//      经由一个自定义 bump-pool 分配。统计分配次数 / 总字节数 / 各帧大小，
//      然后实现 get_return_object_on_allocation_failure 兜底。

#include <coroutine>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <new>
#include <utility>

// ========== 简易 bump pool ==========
class coroutine_pool {
public:
    explicit coroutine_pool(std::size_t size = 65536)
        : memory_(static_cast<unsigned char*>(std::malloc(size)))
        , total_size_(size) {}

    ~coroutine_pool() { std::free(memory_); }

    coroutine_pool(const coroutine_pool&) = delete;
    coroutine_pool& operator=(const coroutine_pool&) = delete;

    void* allocate(std::size_t size) noexcept {
        // bump-up：不支持回收，仅用于观测
        if (used_ + size > total_size_) return nullptr;
        void* p = memory_ + used_;
        used_ += size;
        ++alloc_count_;
        last_alloc_size_ = size;
        return p;
    }

    void deallocate(void* /*ptr*/, std::size_t /*size*/) noexcept {
        ++free_count_;
        // bump 不支持真实回收
    }

    void print_stats() const noexcept {
        std::printf("[pool] allocs=%zu frees=%zu used=%zu/%zu bytes  last_alloc=%zu\n",
                    alloc_count_, free_count_, used_, total_size_, last_alloc_size_);
    }

    std::size_t alloc_count() const noexcept { return alloc_count_; }
    std::size_t used()        const noexcept { return used_; }
    std::size_t last_alloc()  const noexcept { return last_alloc_size_; }

private:
    unsigned char* memory_         = nullptr;
    std::size_t    total_size_     = 0;
    std::size_t    used_           = 0;
    std::size_t    alloc_count_    = 0;
    std::size_t    free_count_     = 0;
    std::size_t    last_alloc_size_ = 0;
};

// 全局 pool 单例（练习目的：单线程足够）
static coroutine_pool& global_pool() {
    static coroutine_pool pool{1 << 20}; // 1 MiB
    return pool;
}

// ========== 接入 P0912 的 task ==========
struct pool_task {
    struct promise_type {
        // P0912：自定义 operator new —— 编译器优先调用此重载
        // 正确签名：static void* operator new(std::size_t)
        static void* operator new(std::size_t size) {
            void* p = global_pool().allocate(size);
            if (!p) throw std::bad_alloc{};
            std::printf("  [op new] size=%zu addr=%p\n", size, p);
            return p;
        }

        static void operator delete(void* ptr, std::size_t size) noexcept {
            std::printf("  [op del] size=%zu addr=%p\n", size, ptr);
            global_pool().deallocate(ptr, size);
        }

        // TODO [必做]：实现 nothrow 兜底
        //   分配失败时被自动调用，返回一个"失败态"task 或抛 bad_alloc
        static pool_task get_return_object_on_allocation_failure() {
            std::printf("  [alloc failure tripped]\n");
            return pool_task{nullptr};
        }

        pool_task get_return_object() noexcept {
            return pool_task{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend()   noexcept { return {}; }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { std::terminate(); }
    };

    std::coroutine_handle<promise_type> h_;

    explicit pool_task(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    pool_task(pool_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    pool_task& operator=(pool_task&&) = delete;
    pool_task(const pool_task&) = delete;
    ~pool_task() { if (h_) h_.destroy(); }

    void resume() { if (h_) h_.resume(); }
};

// ========== 不同复杂度的协程，观察帧大小 ==========
pool_task tiny_coro(int x) {
    co_await std::suspend_always{};
    (void)x;
    co_return;
}

pool_task wider_coro(int a, double b) {
    int    local1 = a * 2;
    double local2 = b + 1.0;
    co_await std::suspend_always{};
    int    local3 = local1 + static_cast<int>(local2);
    co_await std::suspend_always{};
    co_await std::suspend_always{};
    (void)local3;
    co_return;
}

// ========== 测试驱动 ==========
int main()
{
    std::printf("===== F-2: 自定义 promise allocator =====\n\n");

    std::printf("--- 单个 tiny_coro 创建并完成 ---\n");
    {
        auto t = tiny_coro(42);
        t.resume();          // 跑过 initial_suspend 后立即暂停在第一个 co_await
        t.resume();          // 跑过协程体，进入 final_suspend
        // ~pool_task → handle.destroy() → 触发 operator delete
    }
    global_pool().print_stats();

    std::printf("\n--- 单个 wider_coro 创建并完成 ---\n");
    {
        auto t = wider_coro(7, 3.14);
        t.resume();
        t.resume();
        t.resume();
        t.resume();
    }
    global_pool().print_stats();

    std::printf("\n--- 10 次循环 tiny_coro，观察 alloc 计数 ---\n");
    for (int i = 0; i < 10; ++i) {
        auto t = tiny_coro(i);
        t.resume();
        t.resume();
    }
    global_pool().print_stats();

    // TODO [必做]：把上面的 pool_task 替换为默认 ::operator new 的版本
    //   测 10000 次创建/销毁的耗时，对比差异。
    // TODO [进阶]：把 bump 升级为 slab，验证 alloc/free 配对。
    // TODO [进阶]：把 pool 改为 thread_local 或加 atomic，支持多线程。

    std::printf("\n===== Done =====\n");
    return 0;
}
