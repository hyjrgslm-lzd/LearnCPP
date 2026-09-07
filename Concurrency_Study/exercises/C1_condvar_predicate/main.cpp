#include "concurrency_study/exercise_check.hpp"
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <string_view>
#include <vector>

constexpr bool part1_wait_done = false;
constexpr bool part2_broadcast_done = false;
constexpr bool part3_deadline_done = false;
constexpr bool part4_explanation_done = false;
// TODO Part 4：说明为什么 notify 不保存事件，以及 ready=true 早于 wait 时应怎样处理。
// 不要求执行可能永久等待的裸 wait。诊断路径仅在 Reference 中显式选择。
constexpr std::string_view student_lost_wakeup_explanation = "";

template<class Predicate>
void student_wait(std::condition_variable& cv, std::unique_lock<std::mutex>& lock, Predicate predicate) {
    // TODO Part 1：输入 lock 已持锁；用谓词重载等待或等价 while。
    // 返回时必须仍持锁且 predicate()==true，不能把一次唤醒当成条件满足。
    (void)cv; (void)lock; (void)predicate;
    throw std::logic_error("TODO Part 1: predicate wait");
}
void student_broadcast(std::condition_variable& cv) {
    // TODO Part 2：ready 发布后，让所有已等待线程有机会重查；单次 notify_one 不够。
    (void)cv;
    throw std::logic_error("TODO Part 2: broadcast");
}
template<class Predicate>
bool student_wait_until(std::condition_variable& cv, std::unique_lock<std::mutex>& lock,
                        std::chrono::steady_clock::time_point deadline, Predicate predicate) {
    // TODO Part 3：所有重试使用同一个 deadline；返回最终谓词值，不是通知来源。
    (void)cv; (void)lock; (void)deadline; (void)predicate;
    throw std::logic_error("TODO Part 3: fixed deadline");
}
void check_student() {
    cs::check(!student_lost_wakeup_explanation.empty(), "Part 4: explain lost notification");
    std::mutex mutex;
    std::condition_variable cv, arrived;
    bool ready = true;
    int payload = 42, waiting = 0, predicate_checks = 0;
    cv.notify_one(); // 已完成的观察例：通知早于等待，业务状态仍应被记住。
    {
        std::unique_lock lock(mutex);
        student_wait(cv, lock, [&] { return ready; });
        cs::check(lock.owns_lock() && payload == 42, "Part 1: early publication");
    }
    ready = false;
    std::vector<std::future<int>> workers;
    workers.reserve(3);
    try {
        for (int i = 0; i < 3; ++i)
            workers.push_back(std::async(std::launch::async, [&] {
                std::unique_lock lock(mutex);
                ++waiting;
                arrived.notify_one();
                student_wait(cv, lock, [&] {
                    ++predicate_checks;
                    arrived.notify_one();
                    return ready;
                });
                cs::check(lock.owns_lock() && ready, "Part 1: returning predicate and ownership");
                return payload;
            }));
        {
            std::unique_lock lock(mutex);
            arrived.wait(lock, [&] { return waiting == 3; });
            cv.notify_all(); // 测试故意发送无关通知；ready 仍 false。
            arrived.wait(lock, [&] { return predicate_checks >= 6; });
            payload = 99;
            ready = true;
        }
        student_broadcast(cv);
    } catch (...) {
        { std::lock_guard lock(mutex); ready = true; }
        cv.notify_all(); // 测试收尾独立于学生通知实现。
        throw;
    }
    for (auto& w : workers) cs::check(w.get() == 99, "Part 2: broadcast payload");
    std::unique_lock lock(mutex);
    ready = false;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(2);
    cs::check(!student_wait_until(cv, lock, deadline, [&] { return ready; }), "Part 3: timeout false");
    ready = true;
    cs::check(student_wait_until(cv, lock, deadline, [&] { return ready; }), "Part 3: ready at old deadline");
}
int main() {
    if (!(part1_wait_done && part2_broadcast_done && part3_deadline_done && part4_explanation_done)) {
        std::cerr << "STARTER INCOMPLETE: C1 Part 1-4 未完成；未创建任何线程。\n";
        return 1;
    }
    try { check_student(); std::cout << "C1 student OK\n"; return 0; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
