// =====================================================================
// 练习 G-3：SPSC 无锁环形缓冲（single-producer single-consumer ring buffer）
//   对应文档：Concurrency_Study/09-模块G-无锁数据结构.md 的 练习 G-3
//
//   学习目标：
//     - 实现单生产者单消费者（single-producer single-consumer，SPSC）的
//       无锁环形缓冲：底层是定长数组 + 两个原子下标 head / tail；
//     - 抓住 SPSC 的关键假设——head 与 tail 各自只被一方独占写入：
//       生产者只写 tail（消费者只读它）、消费者只写 head（生产者只读它），
//       于是【两个写者之间没有竞争】，无需 CAS，只要 load/store 配 release/acquire；
//     - 用 release/acquire 配对传递数据可见性：生产者先写好槽位数据，再以
//       release 提交 tail；消费者以 acquire 读 tail，从而保证“看到新 tail 时，
//       槽位里的数据也已可见”（happens-before）；
//     - 正确做满/空判定（留一格法：满 = (tail+1)%N == head，空 = head == tail）；
//     - 验证 FIFO（先进先出）顺序与不丢数据。
//
//   官方参考：
//     - https://en.cppreference.com/w/cpp/atomic/atomic
//     - https://en.cppreference.com/w/cpp/atomic/memory_order
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 7 章
//     - moodycamel 博客 "A Fast Lock-Free Queue for C++"（SPSC/MPMC 设计讨论）
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target G3_spsc_ringbuffer --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <cstddef>
#include <thread>
#include <vector>

// =====================================================================
// SPSC 无锁环形缓冲。
//
//   容量约定：内部数组 Capacity 个槽位，但“留一格”用于区分满与空，
//   故实际可用容量为 Capacity - 1。
//     - 空：head == tail
//     - 满：(tail + 1) % Capacity == head
//
//   下标所有权（SPSC 的灵魂）：
//     - tail_ 只由【生产者】写，消费者只读；
//     - head_ 只由【消费者】写，生产者只读。
//   因为每个原子各自只有一个写者，所以没有写-写竞争，不需要 CAS。
//   跨线程要传递的是“槽位数据”的可见性，靠 release/acquire 配对完成。
// =====================================================================
// MSVC 会就 alignas 造成的结构填充给出 C4324——这正是我们刻意要的对齐填充
// （让 head_/tail_ 各占一条缓存行），故在本类作用域内显式忽略该警告。
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4324)
#endif
template <class T, std::size_t Capacity>
class SpscRingBuffer {
    static_assert(Capacity >= 2, "Capacity 至少为 2（留一格法需要）。");

public:
    SpscRingBuffer() = default;
    SpscRingBuffer(const SpscRingBuffer&) = delete;
    SpscRingBuffer& operator=(const SpscRingBuffer&) = delete;

    // -----------------------------------------------------------------
    // 必做 1：push（仅生产者调用）。队列满则返回 false。
    //
    //   步骤与内存序：
    //     1) relaxed 读自己的 tail_（生产者是 tail_ 的唯一写者，读自己的写
    //        无需同步）；
    //     2) acquire 读 head_（消费者写的，要 acquire 才能看到消费者最新的
    //        推进，从而正确判满）；
    //     3) 满则返回 false；
    //     4) 先把数据写进槽位 buf_[tail]，
    //     5) 再用 release 把新 tail_ 发布出去 —— release 保证“写槽位”这件事
    //        happens-before 消费者 acquire 读到该 tail，于是数据对消费者可见。
    // -----------------------------------------------------------------
    bool push(const T& value) {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t next = (tail + 1) % Capacity;

        // TODO [必做 1]: 用 acquire 读 head_ 做满判定。
        //   下面已直接给出参考实现以保证可编译运行；理解后请遮住重写一遍。
        if (next == head_.load(std::memory_order_acquire)) {
            return false; // 满（留一格）
        }

        // 先写数据，再发布 tail —— 顺序不能反。
        buf_[tail] = value;

        // TODO [必做 1]: 用 release 提交新 tail，把“槽位已写好”发布给消费者。
        //   release 与消费者 pop 里对 tail_ 的 acquire 配对，建立 happens-before。
        tail_.store(next, std::memory_order_release);
        return true;
    }

    // -----------------------------------------------------------------
    // 必做 1：pop（仅消费者调用）。队列空则返回 false。
    //
    //   步骤与内存序：
    //     1) relaxed 读自己的 head_（消费者是 head_ 的唯一写者）；
    //     2) acquire 读 tail_（生产者写的，acquire 才能看到生产者刚发布的
    //        数据；这正是与 push 中 release 配对的那一半）；
    //     3) 空（head == tail）则返回 false；
    //     4) 从槽位 buf_[head] 取数据；
    //     5) release 提交新 head_ —— 让生产者 acquire 读到时，知道这一格已空出。
    // -----------------------------------------------------------------
    bool pop(T& out) {
        const std::size_t head = head_.load(std::memory_order_relaxed);

        // TODO [必做 1]: 用 acquire 读 tail_ 做空判定，并与 push 的 release 配对。
        if (head == tail_.load(std::memory_order_acquire)) {
            return false; // 空
        }

        out = buf_[head]; // acquire 保证：能读到生产者写进这一格的数据
        const std::size_t next = (head + 1) % Capacity;

        // TODO [必做 1]: 用 release 提交新 head，把“这一格已读完、可复用”发布给生产者。
        head_.store(next, std::memory_order_release);
        return true;
    }

private:
    // 让 head_ 与 tail_ 落在不同缓存行可减少伪共享（false sharing），
    // 伪共享是模块 J 的主题；这里先用 alignas 点到，不展开。
    alignas(64) std::atomic<std::size_t> head_{0}; // 仅消费者写
    alignas(64) std::atomic<std::size_t> tail_{0}; // 仅生产者写
    alignas(64) T buf_[Capacity]{};
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

int main() {
    cs::println("==== G3_spsc_ringbuffer：SPSC 无锁环形缓冲 ====\n");

    constexpr std::size_t kCapacity = 1024; // 实际可用 1023（留一格）
    constexpr int kTotal = 200000;          // 生产/消费的元素总数

    SpscRingBuffer<int, kCapacity> ring;

    // 必做 2：单生产者单消费者跑通，验证 FIFO 顺序与不丢数据。
    //   生产者按 0,1,2,...,kTotal-1 顺序 push；
    //   消费者按收到顺序检查“值是否严格等于期望的递增序列”。
    std::atomic<bool> order_ok{true};
    std::atomic<int> consumed{0};

    std::thread producer([&] {
        for (int i = 0; i < kTotal; ++i) {
            // 满则自旋等待消费者腾出空间（单生产者，无需 CAS）。
            while (!ring.push(i)) {
                std::this_thread::yield();
            }
        }
        cs::logf("[producer] 完成 ", kTotal, " 次 push（按 0..N-1 顺序）。");
    });

    std::thread consumer([&] {
        int expected = 0;
        int value = 0;
        while (expected < kTotal) {
            if (ring.pop(value)) {
                if (value != expected) {
                    order_ok.store(false, std::memory_order_relaxed);
                }
                ++expected;
                consumed.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield(); // 空则等生产者
            }
        }
        cs::logf("[consumer] 完成 ", kTotal, " 次 pop。");
    });

    producer.join();
    consumer.join();

    const int got = consumed.load();
    cs::logf("[main] 消费总数 = ", got, "（期望 ", kTotal, "）-- ",
             (got == kTotal ? "不丢数据 OK" : "丢数据 MISMATCH"));
    cs::logf("[main] FIFO 顺序检查 -- ",
             (order_ok.load() ? "严格递增、顺序正确 OK" : "顺序错乱 FAIL"));

    cs::println("\n要点：tail 只由生产者写、head 只由消费者写 → 两个写者无竞争，");
    cs::println("      无需 CAS；靠 release(写) / acquire(读) 配对传递数据可见性。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
