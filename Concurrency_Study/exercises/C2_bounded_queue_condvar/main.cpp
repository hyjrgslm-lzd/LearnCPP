// =====================================================================
// 练习 C-2：用条件变量实现有界阻塞队列（bounded blocking queue）
//   对应文档：Concurrency_Study/04-模块C-条件变量.md 的 练习 C-2
//
//   学习目标：
//     - 用一把 mutex + 两个 condition_variable（not_full / not_empty）
//       实现有界阻塞队列：push 满则等、pop 空则等；
//     - 理解“为什么是两个条件变量”——生产者与消费者等待的是两个
//       不同的谓词（“非满”与“非空”），分开通知可避免无谓唤醒；
//     - 正确选择 notify_one：每次状态变化只让一个对侧等待者前进；
//     - 进阶：加 close() 语义让消费者在队列耗尽后优雅退出。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/thread/condition_variable
//     - https://en.cppreference.com/w/cpp/thread/condition_variable/notify_one
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 4 章
//       （4.1.1 “用条件变量等待条件” 中的线程安全队列）
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target C2_bounded_queue_condvar --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>

// =====================================================================
// 有界阻塞队列。
//   不变式（invariant）：任何时刻 q_.size() <= cap_。
//   生产者等待的谓词：!满（或已关闭）；消费者等待的谓词：!空（或已关闭）。
// =====================================================================
template <class T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t cap) : cap_(cap) {}

    // -----------------------------------------------------------------
    // 必做 1：push —— 满则等待 not_full，入队后通知 not_empty。
    // 返回 false 表示队列已关闭、拒绝继续入队。
    // -----------------------------------------------------------------
    bool push(T value) {
        std::unique_lock<std::mutex> lk(mtx_);

        // TODO [必做 1]: 队列满时等待“非满”谓词。
        //   要写：not_full_.wait(lk, [&]{ return q_.size() < cap_ || closed_; });
        //   谓词同时把“已关闭”纳入唤醒条件，否则 close() 无法唤醒
        //   卡在满队列上的生产者。
        not_full_.wait(lk, [&] { return q_.size() < cap_ || closed_; });

        if (closed_) return false; // 已关闭：不再接收新元素

        q_.push(std::move(value));

        // 入队后“多了一个元素”这一事件，只需唤醒一个消费者。
        // TODO [必做 1]: 用 not_empty_.notify_one() 通知一个消费者。
        //   思考：为什么是 notify_one 而不是 notify_all？
        //   （因为只新增了 1 个可消费元素，唤醒多个消费者会让 N-1 个
        //    醒来发现队列又空了、重新睡——纯属浪费，即“惊群/thundering
        //    herd”。一次状态变化只放行一个对侧等待者。）
        not_empty_.notify_one();
        return true;
    }

    // -----------------------------------------------------------------
    // 必做 1：pop —— 空则等待 not_empty，出队后通知 not_full。
    // 返回 std::nullopt 表示队列已关闭且已排空（消费者据此优雅退出）。
    // -----------------------------------------------------------------
    std::optional<T> pop() {
        std::unique_lock<std::mutex> lk(mtx_);

        // TODO [必做 1]: 队列空时等待“非空”谓词（或已关闭）。
        not_empty_.wait(lk, [&] { return !q_.empty() || closed_; });

        if (q_.empty()) {
            // 走到这里必然是 closed_ 且已排空 → 通知“没有更多了”。
            return std::nullopt;
        }

        T value = std::move(q_.front());
        q_.pop();

        // 出队后“空出一个位置”，唤醒一个生产者即可。
        not_full_.notify_one();
        return value;
    }

    // -----------------------------------------------------------------
    // 进阶 1：close() —— 关闭队列。
    //   语义：不再接受新元素；已入队的元素仍可被取走（drain）；
    //   待队列排空后，pop 返回 nullopt 让消费者优雅退出。
    //   必须 notify_all：所有卡在 wait 上的生产者与消费者都要被唤醒，
    //   重新检查谓词后据 closed_ 决定退出。
    // -----------------------------------------------------------------
    void close() {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            closed_ = true;
        }
        // TODO [进阶 1]: 唤醒所有等待者，让它们重新检查“已关闭”谓词。
        //   关闭是“广播型”事件 → 必须 notify_all（两个 cv 都要）。
        not_full_.notify_all();
        not_empty_.notify_all();
    }

private:
    const std::size_t cap_;
    std::mutex mtx_;
    std::condition_variable not_full_;  // 生产者等它：队列“非满”
    std::condition_variable not_empty_; // 消费者等它：队列“非空”
    std::queue<T> q_;
    bool closed_ = false;
};

int main() {
    cs::println("==== C2_bounded_queue_condvar：有界阻塞队列 ====\n");

    constexpr int kCapacity = 4;
    constexpr int kProducers = 3;
    constexpr int kConsumers = 2;
    constexpr int kItemsPerProducer = 10;

    BoundedQueue<int> queue(kCapacity);

    std::atomic<int> consumed_total{0};

    // 必做 2：多生产者多消费者跑通。
    std::vector<std::thread> producers;
    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([&queue, p] {
            for (int i = 0; i < kItemsPerProducer; ++i) {
                int item = p * 100 + i;
                queue.push(item);
                cs::logf("[producer ", p, "] push ", item);
            }
            cs::logf("[producer ", p, "] 全部入队完毕。");
        });
    }

    std::vector<std::thread> consumers;
    for (int c = 0; c < kConsumers; ++c) {
        consumers.emplace_back([&queue, &consumed_total, c] {
            // pop 返回 nullopt → 队列已关闭且排空 → 优雅退出。
            while (auto item = queue.pop()) {
                consumed_total.fetch_add(1, std::memory_order_relaxed);
                cs::logf("[consumer ", c, "] pop ", *item);
            }
            cs::logf("[consumer ", c, "] 检测到队列关闭且排空，退出。");
        });
    }

    // 等所有生产者把东西放完。
    for (auto& t : producers) t.join();
    cs::logf("[main] 全部生产者结束，关闭队列以让消费者排空后退出。");

    // 进阶 1：关闭队列触发消费者优雅退出。
    queue.close();

    for (auto& t : consumers) t.join();

    const int expected = kProducers * kItemsPerProducer;
    const int actual = consumed_total.load();
    cs::logf("[main] 期望消费 ", expected, " 个，实际消费 ", actual, " 个 -- ",
             (actual == expected ? "一致 OK" : "不一致 MISMATCH"));

    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
