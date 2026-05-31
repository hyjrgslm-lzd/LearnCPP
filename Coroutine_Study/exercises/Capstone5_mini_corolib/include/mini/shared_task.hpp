// =============================================================================
// mini/shared_task.hpp —— mini::shared_task<T>，多 awaiter 共享结果
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"必须包含的组件 #3"
//
// 设计要点（参考 cppcoro shared_task）：
//   - 引用计数 + 多等待者链表 + 析构竞态 —— 是 mini 库中**最复杂**的组件；
//   - 多个 awaiter 通过链表挂在 promise 上；
//   - promise 完成时遍历链表唤醒所有 awaiter；
//   - 14-mini §"提示"：建议把 shared_task 留到进阶任务。
//
// 本骨架只给出接口形状与字段布局；实现留作 §"必做任务"中的扩展任务。
// =============================================================================

#pragma once

#include <atomic>
#include <coroutine>
#include <exception>
#include <utility>
#include <variant>

namespace mini {

template <typename T>
struct shared_task {
    struct promise_type;

    struct awaiter_node {
        std::coroutine_handle<> caller_{};
        awaiter_node*           next_{nullptr};
    };

    struct promise_type {
        std::atomic<int>       refcount_{1};
        std::atomic<awaiter_node*> awaiters_{nullptr};
        std::variant<std::monostate, T, std::exception_ptr> result_{};

        // TODO[必做]: 仿 task<T> 实现 8 个 hook；区别：
        //   - get_return_object 返回 shared_task；
        //   - final_suspend 的 awaitable：
        //       从 awaiters_ 链表逐个 resume 等待者；
        //       注意线程安全：用 atomic exchange 一次性取走整个链表。
        shared_task get_return_object();
        std::suspend_always initial_suspend() noexcept { return {}; }

        struct final_awaiter {
            bool await_ready() noexcept { return false; }
            void await_suspend(std::coroutine_handle<promise_type>) noexcept {
                // TODO: 从 awaiters_ exchange(nullptr) 拿到链表头，
                //       逐个 resume。注意：resume 后链表节点的内存归属由 awaiter 自己管。
            }
            void await_resume() noexcept {}
        };
        final_awaiter final_suspend() noexcept { return {}; }

        template <typename U>
        void return_value(U&& v) {
            result_.template emplace<1>(std::forward<U>(v));
        }
        void unhandled_exception() {
            result_.template emplace<2>(std::current_exception());
        }
    };

    std::coroutine_handle<promise_type> h_{};
    explicit shared_task(std::coroutine_handle<promise_type> h) : h_(h) {}

    shared_task(const shared_task& o) noexcept : h_(o.h_) {
        if (h_) ++h_.promise().refcount_;
    }
    shared_task(shared_task&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    shared_task& operator=(const shared_task&) = delete;
    shared_task& operator=(shared_task&&)      = delete;

    ~shared_task() {
        if (h_ && --h_.promise().refcount_ == 0) h_.destroy();
    }

    // TODO[必做]: awaiter 接口
    //   await_ready: h_.done()
    //   await_suspend: 把 caller 封装为 awaiter_node 推到 awaiters_ 链表
    //                  返回 noop_coroutine 或 h_（若尚未启动）
    //   await_resume: 取 result_，若是 exception_ptr 则 rethrow
};

template <typename T>
shared_task<T> shared_task<T>::promise_type::get_return_object() {
    return shared_task<T>{std::coroutine_handle<promise_type>::from_promise(*this)};
}

} // namespace mini
