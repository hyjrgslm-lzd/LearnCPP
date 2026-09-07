#include "concurrency_study/exercise_check.hpp"
#include <atomic>
#include <chrono>
#include <future>
#include <iostream>
#include <semaphore>
#include <vector>

constexpr bool part1_permits_done = false;
constexpr bool part2_timed_done = false;
constexpr bool part3_handoff_done = false;
constexpr bool part4_failure_done = false;
template<class Work>
void student_with_permit(std::counting_semaphore<3>& permits, Work work) {
    // TODO Part 1/4：先 acquire，再运行 work，正常/异常退出都恰好 release 一份。
    // 异常必须原样传播；可用局部 RAII lease，不要借用答案函数。
    (void)permits; (void)work;
    throw std::logic_error("TODO Part 1/4: permit ownership");
}
bool student_timed_acquire(std::counting_semaphore<3>& permits) {
    // TODO Part 2：try_acquire_for(1ms) 并返回是否获取。测试调用时计数确定为0，
    // 无其他释放方，因此本次应 false。不要断言有许可时 try 必须一次成功。
    (void)permits;
    throw std::logic_error("TODO Part 2: timed permit");
}
void student_send(std::binary_semaphore& ready, std::binary_semaphore& ack, int& payload, int value) {
    // TODO Part 3：写 payload，发布 ready，等 ack 后才允许下次覆盖。
    (void)ready; (void)ack; (void)payload; (void)value;
    throw std::logic_error("TODO Part 3: send and acknowledge");
}
bool student_receive(std::binary_semaphore& ready, std::binary_semaphore& ack,
                     const int& payload, int expected) {
    // TODO Part 3/4：等 ready，记录 payload==expected 的 bool，发 ack，再返回 bool。
    // 内容错误也必须先完成 ack；不要在通知前抛异常让发送方永久等待。
    (void)ready; (void)ack; (void)payload; (void)expected;
    throw std::logic_error("TODO Part 3/4: receive and acknowledge");
}
void check_student() {
    std::counting_semaphore<3> permits(3);
    std::atomic<int> active{0}, peak{0}, completed{0};
    std::vector<std::future<void>> workers;
    for (int t = 0; t < 8; ++t)
        workers.push_back(std::async(std::launch::async, [&] {
            for (int i = 0; i < 100; ++i)
                student_with_permit(permits, [&] {
                    const int now = ++active;
                    struct decrement { std::atomic<int>& n; ~decrement() { --n; } } guard{active};
                    int old = peak.load();
                    while (now > old && !peak.compare_exchange_weak(old, now)) {}
                    cs::check(now <= 3, "Part 1: resource limit");
                    ++completed;
                });
        }));
    for (auto& w : workers) w.get();
    cs::check(active == 0 && completed == 800 && peak >= 1 && peak <= 3, "Part 1: all work and permits");
    bool caught = false;
    try { student_with_permit(permits, [] { throw 17; }); }
    catch (int value) { caught = value == 17; }
    cs::check(caught, "Part 4: user error propagated");
    for (int i = 0; i < 3; ++i) permits.acquire(); // 也检查异常路径归还了许可。
    cs::check(!permits.try_acquire() && !student_timed_acquire(permits), "Part 2: zero permits");
    permits.release(3);
    cs::check(decltype(permits)::max() >= 3, "Part 2: least_max_value");

    std::binary_semaphore ready(0), ack(0);
    int payload = 0;
    auto receiver = std::async(std::launch::async, [&] {
        bool correct = true;
        for (int i = 1; i <= 100; ++i) {
            // 第50次故意要求错误值；仍须 ack，然后返回 false，循环不能短路。
            bool matched = student_receive(ready, ack, payload, i == 50 ? -1 : i);
            correct = correct && (matched == (i != 50));
        }
        return correct;
    });
    for (int i = 1; i <= 100; ++i) student_send(ready, ack, payload, i);
    cs::check(receiver.get(), "Part 3/4: matching and mismatching handoffs finish");
}
int main() {
    if (!(part1_permits_done && part2_timed_done && part3_handoff_done && part4_failure_done)) {
        std::cerr << "STARTER INCOMPLETE: H2 Part 1-4 未完成；未创建任何线程。\n";
        return 1;
    }
    try { check_student(); std::cout << "H2 student OK\n"; return 0; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
