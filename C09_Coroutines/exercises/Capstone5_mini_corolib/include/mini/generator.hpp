// =============================================================================
// mini/generator.hpp —— mini::generator<T>，同步惰性序列生成器
//
// 对应文档：14-第三阶段结课-mini协程库实现.md  §"第四层：generator<T>"
//
// 设计要点：
//   - promise yield_value 必须 *按值* 存储，避免 std::generator vs 自写 generator
//     的临时量陷阱（见 J-1 陷阱 8）；
//   - initial_suspend / final_suspend 都返回 suspend_always；
//   - 提供 begin() / end() 迭代器，operator++ 调用 resume()。
// =============================================================================

#pragma once

#include <coroutine>
#include <exception>
#include <iterator>
#include <utility>

namespace mini {

template <typename T>
struct generator {
    struct promise_type {
        T                      current_{}; // ← 按值存储，安全
        std::exception_ptr     error_{};

        generator get_return_object() {
            return generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept { return {}; }
        std::suspend_always final_suspend()   noexcept { return {}; }

        // 同时接受 prvalue / lvalue —— 按值存
        template <typename U>
        std::suspend_always yield_value(U&& v) {
            current_ = std::forward<U>(v);
            return {};
        }
        void return_void() noexcept {}
        void unhandled_exception() { error_ = std::current_exception(); }
    };

    struct iterator {
        using iterator_category = std::input_iterator_tag;
        using value_type        = T;
        using reference         = const T&;
        using pointer           = const T*;
        using difference_type   = std::ptrdiff_t;

        std::coroutine_handle<promise_type> h_{};

        iterator& operator++() {
            h_.resume();
            if (h_.done()) {
                if (h_.promise().error_) std::rethrow_exception(h_.promise().error_);
                h_ = nullptr;
            }
            return *this;
        }
        void operator++(int) { ++(*this); }

        const T& operator*() const noexcept { return h_.promise().current_; }
        const T* operator->() const noexcept { return &h_.promise().current_; }

        bool operator==(std::default_sentinel_t) const noexcept {
            return !h_ || h_.done();
        }
    };

    std::coroutine_handle<promise_type> h_{};
    explicit generator(std::coroutine_handle<promise_type> h) : h_(h) {}
    generator(generator&& o) noexcept : h_(std::exchange(o.h_, {})) {}
    ~generator() { if (h_) h_.destroy(); }

    iterator begin() {
        if (h_) {
            h_.resume();
            if (h_.done()) {
                if (h_.promise().error_) std::rethrow_exception(h_.promise().error_);
                return {nullptr};
            }
        }
        return {h_};
    }
    std::default_sentinel_t end() noexcept { return {}; }
};

} // namespace mini
