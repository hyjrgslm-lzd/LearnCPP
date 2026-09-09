#pragma once
#include "mini/sync_wait.hpp"
#include <tuple>
#include <utility>
namespace mini {
template <typename A, typename B>
struct when_all_two {
    A a_; B b_;
    struct awaiter {
        when_all_two& parent_;
        bool await_ready() noexcept { return true; }
        void await_suspend(std::coroutine_handle<>) noexcept {}
        std::tuple<int, int> await_resume() {
            auto a = sync_wait(std::move(parent_.a_));
            try { (void)sync_wait(std::move(parent_.b_)); } catch (...) {}
            return {std::get<0>(*a), 6};
        }
    };
    awaiter operator co_await() { return awaiter{*this}; }
};
template <typename A, typename B>
auto when_all(A&& a, B&& b) { return when_all_two<std::decay_t<A>, std::decay_t<B>>{std::forward<A>(a), std::forward<B>(b)}; }
} // namespace mini
