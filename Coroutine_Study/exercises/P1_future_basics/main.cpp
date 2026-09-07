// 对应讲义：00-预备知识-执行模型与标准库.md，练习 P-1。
// 先运行日志，再逐个完成 TODO；solution.cpp 给出对应的完整实验。
#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <print>

using namespace std::chrono_literals;

int main() {
    std::cout << std::boolalpha;
    {
        std::promise<int> producer;
        auto result = producer.get_future();
        std::cout << "P1/1 valid=" << result.valid()
                  << " pending=" << (result.wait_for(0ms) == std::future_status::timeout) << '\n';
        // TODO P1-1：提供 42；观察 wait() 与 get() 各自对 valid() 的影响。
        producer.set_value(42);
        result.wait();
        std::cout << "value=" << result.get() << " valid_after_get=" << result.valid() << '\n';
    }
    {
        std::promise<void> release;
        auto result = std::async(std::launch::async, [gate = release.get_future()]() mutable {
            gate.wait();
            // TODO P1-2：返回 42，并记录此处线程 ID，与 main 的线程 ID 比较。
            std::cout << "\nstd::sync thread id=" << std::this_thread::get_id() << '\n';
            return 42;
        });
        const auto before_release = result.wait_for(0ms);
        release.set_value();
        std::cout << "[" << std::this_thread::get_id() << "]"
                  << "P1/2 pending_before_release="
                  << (before_release == std::future_status::timeout)
                  << " value=" << result.get() << '\n';
    }
    {
        std::promise<int> producer;
        auto result = producer.get_future();
        // TODO P1-3：改为 set_exception(make_exception_ptr(runtime_error(...)))。
        producer.set_exception(std::make_exception_ptr(std::runtime_error("test_exception")));
        try {
            std::cout << "P1/3 value=" << result.get() << '\n';
        } catch (const std::runtime_error& error) {
            std::cout << "P1/3 error=" << error.what() << '\n';
        }
    }
    {
        bool executed = false;
        auto result = std::async(std::launch::deferred, [&] {
            executed = true;
            return 42;
        });
        // TODO P1-4：记录 wait_for(0ms) 的状态，再对比 get() 前后的 executed。
        std::cout << "P1/4 before_get=" << executed << '\n';
        const int value = result.get();
        std::cout << "after_get=" << executed << " value=" << value << '\n';
    }
    {
        std::promise<int> producer;
        auto first = producer.get_future().share();
        auto second = first;
        producer.set_value(7);
        // TODO P1-5：用 const int& 保存两次 get() 的结果，观察值与地址。
        const int& first_value = first.get();
        const int& second_value = second.get();
        std::println("P1/5 first.get()={} first.addr={}", first_value, static_cast<const void*>(&first_value));
        std::println("P1/5 second.get()={} second.addr={}", second_value, static_cast<const void*>(&second_value));
    }
}
