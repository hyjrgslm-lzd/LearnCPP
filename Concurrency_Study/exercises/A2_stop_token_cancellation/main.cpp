#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <future>
#include <iostream>
#include <memory>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <utility>

constexpr bool part1_polling_done = false;
constexpr bool part2_sources_done = false;
constexpr bool part3_callbacks_done = false;
constexpr bool part4_lifetime_done = false;
constexpr bool part5_inplace_done = false; // 仅探测到 C++26 能力时要求完成。
bool student_poll(std::stop_token token) {
    // TODO Part 1：循环检查 token，看到停止才返回 true。不能固定迭代次数冒充取消。
    // yield 只是调度提示，不能以 sleep 推断请求先后。
    (void)token;
    throw std::logic_error("TODO Part 1: cooperative poll");
}
std::stop_source student_copy_source(const std::stop_source& source) {
    // TODO Part 2：返回控制同一状态的 source 副本，不创建独立取消域。
    (void)source;
    throw std::logic_error("TODO Part 2: shared stop state");
}
template<class F>
auto student_register(std::stop_token token, F fn)
    -> std::unique_ptr<std::stop_callback<F>> {
    // TODO Part 3：构造并返回 RAII 注册对象；已停止 token 会同步调用 fn。
    // 返回对象必须存活到观察结束。callback 不抛出，检查放在 callback 外。
    (void)token; (void)fn;
    throw std::logic_error("TODO Part 3: register callback");
}
template<class F>
void student_unregister(std::unique_ptr<std::stop_callback<F>>& callback) {
    // TODO Part 4：销毁/注销，若其回调在另一线程执行则等其结束。
    // 不要持有回调需要的锁；不可只泄漏指针来绕过析构。
    (void)callback;
    throw std::logic_error("TODO Part 4: callback lifetime");
}
#if CS_HAS_INPLACE_STOP_TOKEN
std::pair<bool, int> student_inplace() {
    // TODO Part 5：局部 inplace_stop_source，注册回调计数，再 request_stop。
    // 返回 {token.stop_requested(),次数}；先销毁 callback，再销毁 source。
    throw std::logic_error("TODO Part 5: inplace source lifetime");
}
#endif

void check_student() {
    std::promise<void> started;
    auto ready = started.get_future();
    std::promise<bool> result;
    auto observed = result.get_future();
    std::jthread worker([&](std::stop_token token) {
        started.set_value();
        try { result.set_value(student_poll(token)); }
        catch (...) { result.set_exception(std::current_exception()); }
    });
    ready.get();
    const bool first = worker.request_stop(), again = worker.request_stop();
    worker.join();
    cs::check(first && !again && observed.get(), "Part 1: observed actual request");
    std::stop_source source;
    auto copy = student_copy_source(source);
    int calls = 0;
    std::thread::id callback_thread;
    auto callback = student_register(source.get_token(), [&]() noexcept {
        ++calls; callback_thread = std::this_thread::get_id();
    });
    cs::check(bool(callback), "Part 3: owns callback registration");
    cs::check(copy.request_stop() && source.stop_requested(), "Part 2: copied source shares state");
    cs::check(calls == 1 && callback_thread == std::this_thread::get_id(), "Part 3: synchronous request callback");
    auto late = student_register(source.get_token(), [&]() noexcept { ++calls; });
    cs::check(bool(late) && calls == 2 && !source.request_stop(), "Part 3: late registration");
    // 已完成的观察例：没有 source 且没有请求的 token 已不可能被请求停止。
    std::stop_token orphan;
    { std::stop_source temporary; orphan = temporary.get_token(); }
    cs::check(!orphan.stop_possible() && !std::stop_token{}.stop_possible(), "Part 2: unavailable sources");
    std::stop_source external;
    auto external_worker = std::async(std::launch::async, [&] { return student_poll(external.get_token()); });
    external.request_stop();
    cs::check(external_worker.get(), "Part 4: source independent of jthread");

    std::stop_source concurrent_source;
    std::promise<void> entered, release;
    auto entry = entered.get_future();
    auto leave = release.get_future().share();
    std::atomic<bool> completed{false};
    auto concurrent = student_register(concurrent_source.get_token(), [&]() noexcept {
        entered.set_value(); leave.wait(); completed.store(true);
    });
    cs::check(bool(concurrent), "Part 4: registration exists");
    auto request = std::async(std::launch::async, [&] { return concurrent_source.request_stop(); });
    entry.get();
    std::future<bool> destroy;
    try {
        destroy = std::async(std::launch::async, [&] {
            student_unregister(concurrent);
            return !concurrent && completed.load();
        });
    } catch (...) { release.set_value(); throw; }
    release.set_value();
    cs::check(request.get() && destroy.get(), "Part 4: deregistration observes completed callback");
#if CS_HAS_INPLACE_STOP_TOKEN
    cs::check(student_inplace() == std::pair{true, 1}, "Part 5: inplace stop and callback");
#else
    std::cout << "SKIP Part 5 inplace_stop_token: capability unavailable\n";
#endif
}
int main() {
    bool incomplete = !(part1_polling_done && part2_sources_done && part3_callbacks_done && part4_lifetime_done);
#if CS_HAS_INPLACE_STOP_TOKEN
    incomplete = incomplete || !part5_inplace_done;
#endif
    if (incomplete) {
        std::cerr << "STARTER INCOMPLETE: A2 Part 1-4 及可用的 Part 5 未完成；未创建任何线程。\n";
        return 1;
    }
    try { check_student(); std::cout << "A2 student OK\n"; return 0; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
