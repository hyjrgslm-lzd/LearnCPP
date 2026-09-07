#include "concurrency_study/bounded_channel.hpp"
#include "concurrency_study/exercise_check.hpp"
#include <chrono>
#include <condition_variable>
#include <future>
#include <iostream>
#include <mutex>
#include <optional>
#include <stop_token>
#include <utility>

constexpr bool part1_mailbox_done = false;
constexpr bool part2_stop_done = false;
constexpr bool part3_priority_done = false;
constexpr bool part4_timeout_done = false;
class student_mailbox {
public:
    bool put(int value) {
        // TODO Part 1/3：单槽已占用返回 false；否则持锁发布并通知，禁止覆盖。
        (void)value;
        throw std::logic_error("TODO Part 1: put");
    }
    std::optional<int> take(std::stop_token token) {
        // TODO Part 1/2/3：_any 的 token+predicate wait，返回 false 才放弃等待。
        // 数据优先：token 已停止且 slot 有值仍取走；移出后 reset。
        (void)token;
        throw std::logic_error("TODO Part 2: interruptible take");
    }
private:
    std::mutex mutex_;
    std::condition_variable_any cv_;
    std::optional<int> slot_;
};
bool student_expired_wait(std::condition_variable_any& cv, std::unique_lock<std::mutex>& lock,
                          std::stop_token token) {
    // TODO Part 4：以 steady_clock::now() 为已到期限，谓词 false，token 尚未请求。
    // 返回 bool 应是 false；不要把这个 false 错报成 token 已取消。
    (void)cv; (void)lock; (void)token;
    throw std::logic_error("TODO Part 4: timed interruptible wait");
}
void check_student() {
    student_mailbox box;
    std::stop_source source;
    auto first = std::async(std::launch::async, [&] { return box.take(source.get_token()); });
    try { cs::check(box.put(42), "Part 1: put"); }
    catch (...) { source.request_stop(); throw; }
    cs::check(first.get() == 42, "Part 1: delivered payload");
    auto stopped = std::async(std::launch::async, [&] { return box.take(source.get_token()); });
    source.request_stop();
    cs::check(!stopped.get(), "Part 2: stop empty wait");
    cs::check(!box.take(source.get_token()), "Part 2: pre-requested stop");
    cs::check(box.put(7) && !box.put(8), "Part 3: reject overwrite");
    cs::check(box.take(source.get_token()) == 7, "Part 3: data wins over stop");
    std::mutex mutex;
    std::condition_variable_any cv;
    std::unique_lock lock(mutex);
    std::stop_source live;
    cs::check(!student_expired_wait(cv, lock, live.get_token()) && !live.stop_requested(),
              "Part 4: false may mean timeout without cancellation");
    lock.unlock();
    // 已完成的比较例：复用 C2 通道，只演示 close/drain 与取消单次 take 的差别。
    cs::bounded_channel<int> stream(2);
    cs::check(stream.push(1) && stream.push(2), "comparison: accepted values");
    stream.close();
    cs::check(stream.pop() == 1 && stream.pop() == 2 && !stream.pop(), "comparison: drain");
}
int main() {
    if (!(part1_mailbox_done && part2_stop_done && part3_priority_done && part4_timeout_done)) {
        std::cerr << "STARTER INCOMPLETE: C3 Part 1-4 未完成；未创建任何线程。\n";
        return 1;
    }
    try { check_student(); std::cout << "C3 student OK\n"; return 0; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
