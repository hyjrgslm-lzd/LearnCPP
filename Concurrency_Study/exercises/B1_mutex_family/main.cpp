#include "concurrency_study/exercise_check.hpp"
#include <chrono>
#include <future>
#include <iostream>
#include <mutex>
#include <shared_mutex>
#include <vector>

constexpr bool part1_counter_done = false;
constexpr bool part2_snapshot_done = false;
constexpr bool part3_ownership_done = false;
constexpr bool part4_mutex_family_done = false;

void student_increment(int& value, std::mutex& mutex) {
    // TODO Part 1：四线程各调用 2000 次；把完整 ++ 放进 RAII 临界区。
    // 不可先读旧值再加锁；Starter 不运行无锁自增（UB）。
    (void)value; (void)mutex;
    throw std::logic_error("TODO Part 1: increment");
}
struct student_config {
    mutable std::shared_mutex mutex;
    int revision = 0, twice_revision = 0;
    void set(int n) {
        // TODO Part 2：独占更新两个字段，保持 twice_revision == 2*revision。
        (void)n;
        throw std::logic_error("TODO Part 2: set");
    }
    std::pair<int, int> snapshot() const {
        // TODO Part 2：一次共享临界区中复制两个值，不返回内部引用。
        throw std::logic_error("TODO Part 2: snapshot");
    }
};
std::unique_lock<std::mutex> student_acquire_and_move(std::unique_lock<std::mutex>& deferred) {
    // TODO Part 3a：defer_lock 输入；加锁，把所有权移入返回值，源不再拥有锁。
    (void)deferred;
    throw std::logic_error("TODO Part 3a: acquire and move");
}
int student_snapshot_and_unlock(std::unique_lock<std::mutex>& owner, const int& value) {
    // TODO Part 3b：先复制受保护值，再解锁，返回独立快照。
    (void)owner; (void)value;
    throw std::logic_error("TODO Part 3b: snapshot and unlock");
}
bool student_timed_try(std::timed_mutex& mutex) {
    // TODO Part 4a：try_lock_for(1ms)，成功时用 RAII 释放，返回是否获得锁。
    (void)mutex;
    throw std::logic_error("TODO Part 4a: timed mutex");
}
int student_recursive(std::recursive_mutex& mutex, int depth) {
    // TODO Part 4b：depth==0 返回0，否则持锁递归并累计获取次数，输入 depth=2。
    // 每层匹配一次释放；不得把类型换成不可重入的普通 mutex。
    (void)mutex; (void)depth;
    throw std::logic_error("TODO Part 4b: recursive mutex");
}
void check_student() {
    int counter = 0;
    std::mutex mutex;
    student_config config;
    std::vector<std::future<void>> workers;
    for (int i = 0; i < 4; ++i)
        workers.push_back(std::async(std::launch::async, [&] {
            for (int k = 0; k < 2000; ++k) student_increment(counter, mutex);
        }));
    for (auto& w : workers) w.get();
    cs::check(counter == 8000, "Part 1: exact counter");
    workers.clear();
    workers.push_back(std::async(std::launch::async, [&] {
        for (int i = 1; i <= 2000; ++i) config.set(i);
    }));
    for (int i = 0; i < 3; ++i)
        workers.push_back(std::async(std::launch::async, [&] {
            for (int k = 0; k < 2000; ++k) {
                auto [n, twice] = config.snapshot();
                cs::check(twice == 2 * n, "Part 2: coherent snapshot");
            }
        }));
    for (auto& w : workers) w.get();
    cs::check(config.snapshot() == std::pair{2000, 4000}, "Part 2: final version");

    int value = 42;
    std::unique_lock deferred(mutex, std::defer_lock);
    auto moved = student_acquire_and_move(deferred);
    cs::check(!deferred.owns_lock() && moved.owns_lock(), "Part 3: moved ownership");
    int snapshot = student_snapshot_and_unlock(moved, value);
    cs::check(!moved.owns_lock(), "Part 3: unlock before dependent writer");
    auto writer = std::async(std::launch::async, [&] { std::lock_guard lock(mutex); value = 7; });
    writer.get();
    cs::check(snapshot == 42 && value == 7, "Part 3: independent snapshot");
    // 已完成的局部观察例：异常展开依然释放 RAII 锁。
    try { std::lock_guard lock(mutex); throw 1; } catch (int) {}
    { std::lock_guard reacquired(mutex); }

    std::timed_mutex timed;
    std::unique_lock owner(timed);
    auto attempt = std::async(std::launch::async, [&] { return student_timed_try(timed); });
    const bool acquired = attempt.get(); // 主线程至今一直持锁，故不靠 sleep 证明失败。
    owner.unlock();
    cs::check(!acquired, "Part 4: held mutex cannot be acquired");
    std::recursive_mutex recursive;
    cs::check(student_recursive(recursive, 2) == 2, "Part 4: balanced recursion");
}
int main() {
    if (!(part1_counter_done && part2_snapshot_done && part3_ownership_done && part4_mutex_family_done)) {
        std::cerr << "STARTER INCOMPLETE: B1 Part 1-4 未完成；未创建任何线程。\n";
        return 1;
    }
    try { check_student(); std::cout << "B1 student OK\n"; return 0; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
