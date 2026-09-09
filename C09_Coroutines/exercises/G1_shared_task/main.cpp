// G-1 从 task 到 shared_task
// 文档参考：09-模块G-symmetric_transfer与高级task.md 「练习 G-1」
// 官方参考：
//   - Lewis Baker "C++ coroutines: Sharing coroutines"
//   - cppcoro shared_task.hpp
//   - P3552R3 prior work: cppcoro::shared_task 与 sender split 对照
//
// 目标：把 lazy_task<T> 升级为 shared_task<T>——拷贝即增加引用计数；多个协程
//      可以同时 co_await 同一个 shared_task，每个等待者各拿到一份结果（值拷贝）。
//      实现 control_block + 引用计数 + 多 awaiter 的 intrusive 链表。

#include <atomic>
#include <coroutine>
#include <cstdio>
#include <exception>
#include <utility>

// ============ 先复用一个 lazy_task<T>（用作 waiter 协程的返回类型） ============
template <typename T>
struct lazy_task {
    struct promise_type {
        T result_value{};
        std::exception_ptr result_exception;
        std::coroutine_handle<> continuation;

        lazy_task get_return_object() noexcept {
            return lazy_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }

        auto final_suspend() noexcept {
            struct final_awaiter {
                bool await_ready() noexcept { return false; }
                std::coroutine_handle<> await_suspend(
                    std::coroutine_handle<promise_type> h) noexcept
                {
                    auto cont = h.promise().continuation;
                    return cont ? cont : std::noop_coroutine();
                }
                void await_resume() noexcept {}
            };
            return final_awaiter{};
        }
        void return_value(T v) noexcept { result_value = std::move(v); }
        void unhandled_exception() noexcept { result_exception = std::current_exception(); }
    };

    std::coroutine_handle<promise_type> h_;
    explicit lazy_task(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    lazy_task(lazy_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    lazy_task(const lazy_task&) = delete;
    ~lazy_task() { if (h_) h_.destroy(); }

    bool await_ready() noexcept { return false; }
    auto await_suspend(std::coroutine_handle<> caller) noexcept {
        h_.promise().continuation = caller;
        return h_;
    }
    T await_resume() {
        auto& p = h_.promise();
        if (p.result_exception) std::rethrow_exception(p.result_exception);
        return std::move(p.result_value);
    }

    void resume() { if (h_) h_.resume(); }
    bool done() const noexcept { return h_.done(); }
};

// 特化 void：避免 T = void 时 result_value 为 void 的报错
template <>
struct lazy_task<void> {
    struct promise_type {
        std::exception_ptr result_exception;
        std::coroutine_handle<> continuation;

        lazy_task get_return_object() noexcept {
            return lazy_task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        auto final_suspend() noexcept {
            struct final_awaiter {
                bool await_ready() noexcept { return false; }
                std::coroutine_handle<> await_suspend(
                    std::coroutine_handle<promise_type> h) noexcept
                {
                    auto cont = h.promise().continuation;
                    return cont ? cont : std::noop_coroutine();
                }
                void await_resume() noexcept {}
            };
            return final_awaiter{};
        }
        void return_void() noexcept {}
        void unhandled_exception() noexcept { result_exception = std::current_exception(); }
    };

    std::coroutine_handle<promise_type> h_;
    explicit lazy_task(std::coroutine_handle<promise_type> h) noexcept : h_(h) {}
    lazy_task(lazy_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~lazy_task() { if (h_) h_.destroy(); }

    void resume() { if (h_) h_.resume(); }
    bool done() const noexcept { return h_.done(); }
};

// ============ shared_task<T> ============
template <typename T>
struct shared_task {
    // forward declare awaiter
    struct awaiter;

    struct promise_type {
        T          result_value{};
        std::exception_ptr result_exception;
        bool       done_flag = false;

        // intrusive 等待者链表头（单线程版）
        awaiter* waiters_head = nullptr;

        shared_task get_return_object() noexcept {
            return shared_task{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        void return_value(T v) noexcept {
            result_value = std::move(v);
            done_flag = true;
        }
        void unhandled_exception() noexcept {
            result_exception = std::current_exception();
            done_flag = true;
        }

        // final_suspend：唤醒全部等待者
        // 关键：前 N-1 个用 .resume()，最后一个用 symmetric transfer 返回 handle，
        //       这样头节点不会被 resume 两次（UB）。
        auto final_suspend() noexcept {
            struct final_awaiter {
                bool await_ready() noexcept { return false; }
                std::coroutine_handle<> await_suspend(
                    std::coroutine_handle<promise_type> h) noexcept
                {
                    auto& p = h.promise();
                    auto* node = p.waiters_head;
                    p.waiters_head = nullptr;

                    std::coroutine_handle<> last{};
                    while (node) {
                        auto* next = node->next;
                        if (!next) {
                            last = node->caller;
                            break;
                        }
                        node->caller.resume();
                        node = next;
                    }
                    return last ? last : std::noop_coroutine();
                }
                void await_resume() noexcept {}
            };
            return final_awaiter{};
        }
    };

    // shared 控制块：把引用计数和帧解耦
    struct control_block {
        std::coroutine_handle<promise_type> h;
        std::atomic<int> ref_count{1};

        explicit control_block(std::coroutine_handle<promise_type> hh) noexcept : h(hh) {}

        void add_ref() noexcept { ref_count.fetch_add(1, std::memory_order_relaxed); }
        void release_ref() noexcept {
            if (ref_count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
                if (h) h.destroy();
                delete this;
            }
        }
    };

    control_block* cb_ = nullptr;

    // 构造：从 promise 路径走
    explicit shared_task(std::coroutine_handle<promise_type> h)
        : cb_(new control_block(h)) {}

    // 拷贝增加引用
    shared_task(const shared_task& o) noexcept : cb_(o.cb_) {
        if (cb_) cb_->add_ref();
    }
    shared_task& operator=(const shared_task& o) noexcept {
        if (this != &o) {
            if (cb_) cb_->release_ref();
            cb_ = o.cb_;
            if (cb_) cb_->add_ref();
        }
        return *this;
    }
    // 移动：转移所有权
    shared_task(shared_task&& o) noexcept : cb_(std::exchange(o.cb_, nullptr)) {}
    shared_task& operator=(shared_task&& o) noexcept {
        if (this != &o) {
            if (cb_) cb_->release_ref();
            cb_ = std::exchange(o.cb_, nullptr);
        }
        return *this;
    }
    ~shared_task() { if (cb_) cb_->release_ref(); }

    // 显式启动
    void resume() { if (cb_ && cb_->h) cb_->h.resume(); }
    bool done()   const noexcept { return cb_ && cb_->h && cb_->h.done(); }

    // ========== awaiter ==========
    // awaiter 自身就是 intrusive list node——next 指针和 caller 都嵌在 awaiter 里
    struct awaiter {
        std::coroutine_handle<promise_type> h;
        std::coroutine_handle<>             caller;
        awaiter*                            next = nullptr;

        bool await_ready() noexcept {
            return h.promise().done_flag;
        }
        bool await_suspend(std::coroutine_handle<> c) noexcept {
            caller = c;
            auto& p = h.promise();
            // 双重检查：在 ready 与 suspend 之间 task 可能完成
            if (p.done_flag) return false;
            // 头插法
            next = p.waiters_head;
            p.waiters_head = this;
            return true;
        }
        // 关键：返回值的拷贝而非移动——其它等待者也要读
        T await_resume() {
            auto& p = h.promise();
            if (p.result_exception) std::rethrow_exception(p.result_exception);
            return p.result_value;   // 拷贝
        }
    };

    awaiter operator co_await() const noexcept {
        return awaiter{cb_->h, {}, nullptr};
    }
};

// ============ 测试场景 ============
shared_task<int> shared_compute() {
    // 简单：直接返回；done_flag 在 return_value 中置位
    co_return 42;
}

shared_task<int> slow_compute() {
    co_await std::suspend_always{};   // 模拟异步操作
    co_return 100;
}

// 两个等待者协程，演示多 awaiter 共享同一 shared_task
lazy_task<void> waiter_a(shared_task<int> st) {
    int v = co_await st;
    std::printf("[waiter_a] got %d\n", v);
    co_return;
}

lazy_task<void> waiter_b(shared_task<int> st) {
    int v = co_await st;
    std::printf("[waiter_b] got %d\n", v);
    co_return;
}

int main()
{
    std::printf("===== G-1: shared_task =====\n\n");

    // --- 测试 1：基本拷贝语义 + 引用计数 ---
    std::printf("--- 测试 1：基本 shared 语义 ---\n");
    {
        auto st = shared_compute();
        st.resume();   // 启动；task 立即完成，done_flag = true
        auto st2 = st; // 引用计数 2
        auto st3 = st; // 引用计数 3
        std::printf("  st/st2/st3 share the same frame, refcount=3\n");
        // 离开作用域：refcount 归 0，control_block 和 frame 一起销毁
    }

    // --- 测试 2：多 awaiter 同时等待 ---
    std::printf("\n--- 测试 2：多 awaiter 同时 co_await ---\n");
    {
        auto st = slow_compute();
        st.resume();   // 跑到 co_await suspend_always{} 暂停（done_flag 仍为 false）

        auto wa = waiter_a(st);
        auto wb = waiter_b(st);
        // 启动两个等待者，它们都会注册到 promise 的 waiter 链表
        wa.resume();
        wb.resume();

        // 现在 resume slow_compute 让它跑到 final_suspend
        st.resume();   // 通过 co_await suspend_always 跳过暂停点
        // final_awaiter 会逐个 resume 链表中的等待者
    }

    // TODO [必做]：在笔记里画出引用计数从 1 → 3 → 0 的生命周期图。
    // TODO [必做]：标注 3 处竞态窗口（拷贝/读取结果/链表插入）。
    // TODO [进阶]：实现线程安全等待者链表（atomic CAS 头插）。
    // TODO [进阶]：实现 result() 阻塞接口（spin 或 condvar）。

    std::printf("\n===== Done =====\n");
    return 0;
}
