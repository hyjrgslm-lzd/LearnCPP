#include <coroutine_study/exercise_check.hpp>

#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <utility>

using namespace std::chrono_literals;
using coroutine_study::check;

// 对应讲义：00-预备知识-执行模型与标准库.md，练习 P-1。
int main() {
    {
        std::promise<int> producer;
        auto result = producer.get_future();
        check(result.valid(), "a future can have a state before the value is ready");
        check(result.wait_for(0ms) == std::future_status::timeout, "value is not ready yet");
        producer.set_value(42);
        result.wait();
        check(result.valid(), "wait preserves the future's shared state");
        check(result.get() == 42, "get consumes the stored value");
        check(!result.valid(), "get releases the future's shared state");
        std::cout << "P1/1 value=42 valid_after_get=false\n";
    }
    {
        std::promise<void> release;
        const auto caller = std::this_thread::get_id();
        auto result = std::async(std::launch::async, [gate = release.get_future()]() mutable {
            gate.wait();
            return std::pair{42, std::this_thread::get_id()};
        });
        const auto before_release = result.wait_for(0ms);
        release.set_value();
        const auto [value, worker] = result.get();
        check(before_release == std::future_status::timeout, "the gate controls completion");
        check(value == 42 && worker != caller, "explicit async execution uses another thread");
        std::cout << "P1/2 pending_before_release=true value=42 other_thread=true\n";
    }
    {
        std::promise<int> producer;
        auto result = producer.get_future();
        producer.set_exception(std::make_exception_ptr(std::runtime_error("calculation failed")));
        bool caught = false;
        try {
            (void)result.get();
        } catch (const std::runtime_error&) {
            caught = true;
        }
        check(caught && !result.valid(), "get transports an exception and releases the state");
        std::cout << "P1/3 exception_received=true valid_after_get=false\n";
    }
    {
        bool executed = false;
        const auto caller = std::this_thread::get_id();
        auto result = std::async(std::launch::deferred, [&] {
            executed = true;
            return std::this_thread::get_id();
        });
        check(result.wait_for(0ms) == std::future_status::deferred, "timed waiting reports deferred");
        check(!executed, "a timed wait does not start deferred work");
        const auto execution_thread = result.get();
        check(executed && execution_thread == caller, "get executes deferred work on its calling thread");
        std::cout << "P1/4 deferred=true executed_by_get=true same_thread=true\n";
    }
    {
        std::promise<int> producer;
        auto first = producer.get_future().share();
        auto second = first;
        producer.set_value(7);
        const int& a = first.get();
        const int& b = second.get();
        check(a == 7 && b == 7 && &a == &b, "shared_future consumers refer to the same value");
        check(first.valid() && second.valid() && first.get() == 7, "shared_future supports repeated reads");
        std::cout << "P1/5 shared_value=7 repeated_read=7\n";
    }
    std::cout << "P1_reference OK\n";
}
