// =====================================================================
// 练习 Capstone2_lockfree_queue：第二阶段结课 · 无锁队列
//   对应文档：Concurrency_Study/12-第二阶段结课-无锁队列.md
//
//   学习目标（综合阶段二 模块 E/F/G 的全部核心）：
//     - std::atomic / atomic<T>::load/store/compare_exchange（模块 E）；
//     - memory_order：release / acquire / relaxed 各自的适用处与理由（模块 F）；
//     - 无锁数据结构与进展保证（lock-free / wait-free / obstruction-free，模块 G）；
//     - 用【有界】设计绕开“节点回收（reclamation）”这一无锁编程最难的坑：
//       两种队列都建在【定长数组】上，槽位永远复用、从不释放，于是天然
//       规避 ABA 与 use-after-free —— 因此本文件【不依赖】模块 I 的
//       hazard_pointer.hpp / rcu.hpp，完全自包含。
//
//   两种队列：
//     (1) SpscRingBuffer —— 单生产者单消费者环形缓冲（复习练习 G-3）：
//         tail 只由生产者写、head 只由消费者写 → 两个写者无竞争 → 无需 CAS，
//         只用 release/acquire 配对传递“槽位数据”的可见性。
//     (2) MpmcBoundedQueue —— 多生产者多消费者有界队列（Dmitry Vyukov 算法）：
//         固定容量数组，【每个槽位带一个 atomic<size_t> sequence】作为“票据闸门”。
//         enqueue：读 tail → 比对该槽 sequence → CAS 抢 tail 这张 ticket → 写数据
//         → 把 sequence 推进到“可被消费”；sequence 落后即判满。
//         dequeue：读 head → 比对该槽 sequence → CAS 抢 head ticket → 取数据
//         → 把 sequence 推进到“下一轮可被生产”；sequence 落后即判空。
//         全程只在定长数组上 CAS ticket，【无需任何节点回收】。
//
//   基准对比：把上面两种无锁队列与“std::mutex + std::queue”的加锁版做
//     多生产者多消费者吞吐对比，打印 ops/sec，亲身体会无锁的收益与代价。
//
//   骨架说明：关键实现处用 // TODO [必做 N]: / // TODO [进阶 N]: 标记。
//   未填 TODO 处给了“最小占位实现”以保证本文件在 MSVC(VS2026, C++20) 下
//   可直接编译运行——MPMC 的占位实现退化为“内部加一把 mutex 串行化”，
//   因此输出仍然正确（不丢、不重、FIFO 大体保持），只是【不是无锁】、吞吐也不高。
//   真正该实现的 Vyukov CAS 协议见各 TODO 注释，参考代码以注释形式给出，
//   填写时把占位段替换为它即可得到真正的无锁队列。
//
//   官方参考：
//     - 《C++ Concurrency in Action, 2nd ed.》(Anthony Williams) 第 7 章
//       （无锁并发数据结构、memory_order、无锁队列设计）；
//     - Dmitry Vyukov, "Bounded MPMC queue"：
//       https://www.1024cores.net/home/lock-free-algorithms/queues/bounded-mpmc-queue
//     - moodycamel, "A Fast General Purpose Lock-Free Queue for C++"：
//       https://moodycamel.com/blog/2014/a-fast-general-purpose-lock-free-queue-for-c++
//     - https://en.cppreference.com/w/cpp/atomic/atomic
//     - https://en.cppreference.com/w/cpp/atomic/memory_order
//
//   编译运行（VS2026, C++20）：
//     cmake --build build-vs2026 --target Capstone2_lockfree_queue --config Release
// =====================================================================
#include "concurrency_study/log.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <queue>
#include <thread>
#include <vector>

// MSVC 会就 alignas 造成的结构填充给出 C4324——这正是我们刻意要的对齐填充
// （让独立写者的原子各占一条缓存行，减少伪共享 false sharing，见模块 J），
// 故在本文件作用域内显式忽略该警告。
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4324)
#endif

// =====================================================================
// (1) SPSC 无锁环形缓冲（复习练习 G-3，作为 MPMC 的对照基线）。
//
//   容量约定：内部数组 Capacity 个槽位，“留一格”区分满与空，实际可用 Capacity-1。
//     - 空：head == tail
//     - 满：(tail + 1) % Capacity == head
//
//   下标所有权（SPSC 的灵魂）：tail_ 只由生产者写、head_ 只由消费者写。
//   每个原子各自只有一个写者 → 无写-写竞争 → 不需要 CAS。
//   跨线程要传递的是“槽位数据”的可见性，靠 release(写) / acquire(读) 配对完成。
// =====================================================================
template <class T, std::size_t Capacity>
class SpscRingBuffer {
    static_assert(Capacity >= 2, "Capacity 至少为 2（留一格法需要）。");

public:
    SpscRingBuffer() = default;
    SpscRingBuffer(const SpscRingBuffer&)            = delete;
    SpscRingBuffer& operator=(const SpscRingBuffer&) = delete;

    // push（仅生产者调用）。队列满则返回 false。
    //   内存序：relaxed 读自己的 tail（唯一写者）；acquire 读 head（对方写的，
    //   要 acquire 才能看到消费者最新推进，正确判满）；写好数据后 release 提交
    //   tail —— release 保证“写槽位”happens-before 消费者 acquire 读到该 tail。
    bool push(const T& value) {
        const std::size_t tail = tail_.load(std::memory_order_relaxed);
        const std::size_t next = (tail + 1) % Capacity;
        if (next == head_.load(std::memory_order_acquire)) {
            return false; // 满（留一格）
        }
        buf_[tail] = value;                              // 先写数据
        tail_.store(next, std::memory_order_release);    // 再 release 发布 tail
        return true;
    }

    // pop（仅消费者调用）。队列空则返回 false。
    //   内存序：relaxed 读自己的 head（唯一写者）；acquire 读 tail（与 push 的
    //   release 配对，看到生产者刚发布的数据）；取数据后 release 提交 head，
    //   让生产者 acquire 读到时知道这一格已空出可复用。
    bool pop(T& out) {
        const std::size_t head = head_.load(std::memory_order_relaxed);
        if (head == tail_.load(std::memory_order_acquire)) {
            return false; // 空
        }
        out = buf_[head];
        const std::size_t next = (head + 1) % Capacity;
        head_.store(next, std::memory_order_release);
        return true;
    }

private:
    // 让 head_ 与 tail_ 落在不同缓存行可减少伪共享（false sharing，模块 J）。
    alignas(64) std::atomic<std::size_t> head_{0}; // 仅消费者写
    alignas(64) std::atomic<std::size_t> tail_{0}; // 仅生产者写
    alignas(64) T buf_[Capacity]{};
};

// =====================================================================
// (2) MPMC 有界队列（Dmitry Vyukov 的 bounded MPMC queue）。
//
//   核心思想——“每槽一个 sequence 闸门 + 在 tail/head 票据上 CAS”：
//     固定容量 Capacity（必须是 2 的幂，用位与代替取模）。每个 cell 维护一个
//     atomic<size_t> sequence_，配合两个单调递增的票据计数 enqueue_pos_ / dequeue_pos_。
//
//   生产者 enqueue(pos = enqueue_pos_)：
//     看 cell[pos & mask].sequence：
//       - seq == pos      → 该格“轮到被写”，CAS 把 enqueue_pos_ 从 pos 推到 pos+1
//                           抢到这张 ticket（失败说明别的生产者抢先，重读重试）；
//       - seq <  pos      → 该格还没被对应的消费者取走 → 队列【满】，返回 false；
//       - seq >  pos      → 别的生产者已推进，重读 enqueue_pos_ 继续。
//     抢到 ticket 后写数据，再把该 cell.sequence 置为 pos+1 —— 这一步是【发布】，
//     告诉消费者“这格已可读”。
//
//   消费者 dequeue(pos = dequeue_pos_)：对称：
//       - seq == pos+1    → 该格“轮到被读”，CAS 抢 dequeue_pos_；
//       - seq <  pos+1    → 还没人写进这格 → 队列【空】，返回 false；
//       - seq >  pos+1    → 别的消费者已推进，重读继续。
//     取走数据后把该 cell.sequence 置为 pos + Capacity —— 让该格在下一圈重新
//     对生产者“轮到被写”（pos+Capacity 正是下一圈生产者看到的 pos）。
//
//   为什么不需要回收？所有 cell 在数组里【永久存活】，只是 sequence 在变；
//   没有 new/delete，没有指针被释放，所以【天然无 ABA、无 use-after-free】。
//   这正是“有界”设计相对 Treiber 栈/链式无锁队列（练习 G-1）的最大省心处。
//
//   进展保证：enqueue/dequeue 的 CAS 失败后重读重试 —— 整体 lock-free
//   （总有某个线程的 CAS 成功而前进），但非 wait-free（单个线程可能反复重试）。
// =====================================================================
template <class T>
class MpmcBoundedQueue {
public:
    explicit MpmcBoundedQueue(std::size_t capacity)
        : buffer_(capacity), capacity_mask_(capacity - 1) {
        // 容量必须是 2 的幂：才能用 (pos & mask) 取代昂贵的取模，且环绕正确。
        // （这里不抛异常，仅断言；非 2 的幂会让 mask 失效。）
        // 初始化每个 cell 的 sequence：第 i 格初值为 i，表示“第一圈轮到第 i 个写”。
        for (std::size_t i = 0; i < capacity; ++i) {
            buffer_[i].sequence.store(i, std::memory_order_relaxed);
        }
        enqueue_pos_.store(0, std::memory_order_relaxed);
        dequeue_pos_.store(0, std::memory_order_relaxed);
    }

    MpmcBoundedQueue(const MpmcBoundedQueue&)            = delete;
    MpmcBoundedQueue& operator=(const MpmcBoundedQueue&) = delete;

    // -----------------------------------------------------------------
    // enqueue：多生产者安全入队。队列满返回 false（不阻塞）。
    // -----------------------------------------------------------------
    bool enqueue(const T& value) {
        // -------------------------------------------------------------
        // TODO [必做 4]: 实现 Vyukov enqueue 的 CAS + sequence 协议。
        //   真正实现（理解后请遮住注释重写一遍，逐处说明 memory_order 的理由）：
        //
        //     Cell* cell;
        //     std::size_t pos = enqueue_pos_.load(std::memory_order_relaxed);
        //     for (;;) {
        //         cell = &buffer_[pos & capacity_mask_];
        //         // acquire：要看到该 cell 上一轮消费者发布的 sequence，
        //         //   从而保证“判定空/满”和后续读写都建立在最新可见状态上。
        //         std::size_t seq = cell->sequence.load(std::memory_order_acquire);
        //         std::intptr_t diff =
        //             static_cast<std::intptr_t>(seq) - static_cast<std::intptr_t>(pos);
        //         if (diff == 0) {
        //             // 这格轮到我写：尝试把 enqueue_pos_ 从 pos 抢到 pos+1。
        //             // relaxed 足矣：真正的“数据发布”由后面那次 cell.sequence
        //             //   的 release-store 承担；这里只是抢 ticket（一个计数），
        //             //   它本身不携带需要被对端可见的数据载荷。
        //             if (enqueue_pos_.compare_exchange_weak(
        //                     pos, pos + 1, std::memory_order_relaxed)) {
        //                 break; // 抢到了，pos 即我的 ticket
        //             }
        //             // CAS 失败：pos 已被更新为最新值，继续循环重试。
        //         } else if (diff < 0) {
        //             return false; // seq < pos → 这格还没被取走 → 队列满
        //         } else {
        //             // seq > pos → 别的生产者已推进，重读 enqueue_pos_。
        //             pos = enqueue_pos_.load(std::memory_order_relaxed);
        //         }
        //     }
        //     cell->data = value;                       // 写数据（先于发布）
        //     // release：把“数据已写好”发布给将来读这格的消费者；与消费者
        //     //   dequeue 里对 cell.sequence 的 acquire 配对，建立 happens-before。
        //     cell->sequence.store(pos + 1, std::memory_order_release);
        //     return true;
        //
        // 占位实现：用一把内部 mutex 串行化 enqueue/dequeue，保证可编译运行、
        //   不丢不重；但这【不是无锁】，吞吐也低。基准里它会和加锁版表现相近。
        // -------------------------------------------------------------
        std::lock_guard<std::mutex> lk(placeholder_mutex_);
        const std::size_t cap = capacity_mask_ + 1;
        if (placeholder_size_ >= cap) {
            return false; // 满
        }
        Cell& cell = buffer_[placeholder_tail_ & capacity_mask_];
        cell.data  = value;
        placeholder_tail_++;
        placeholder_size_++;
        return true;
    }

    // -----------------------------------------------------------------
    // dequeue：多消费者安全出队。队列空返回 false（不阻塞）。
    // -----------------------------------------------------------------
    bool dequeue(T& out) {
        // -------------------------------------------------------------
        // TODO [必做 5]: 实现 Vyukov dequeue 的 CAS + sequence 协议（与 enqueue 对称）。
        //   真正实现：
        //
        //     Cell* cell;
        //     std::size_t pos = dequeue_pos_.load(std::memory_order_relaxed);
        //     for (;;) {
        //         cell = &buffer_[pos & capacity_mask_];
        //         // acquire：与生产者 enqueue 的 release-store(pos+1) 配对，
        //         //   看到“数据已写好”这一发布，从而读取的数据是可见且正确的。
        //         std::size_t seq = cell->sequence.load(std::memory_order_acquire);
        //         std::intptr_t diff =
        //             static_cast<std::intptr_t>(seq)
        //             - static_cast<std::intptr_t>(pos + 1);
        //         if (diff == 0) {
        //             // 这格轮到我读：抢 dequeue_pos_ ticket（relaxed 同 enqueue 理由）。
        //             if (dequeue_pos_.compare_exchange_weak(
        //                     pos, pos + 1, std::memory_order_relaxed)) {
        //                 break;
        //             }
        //         } else if (diff < 0) {
        //             return false; // seq < pos+1 → 还没人写进这格 → 队列空
        //         } else {
        //             pos = dequeue_pos_.load(std::memory_order_relaxed);
        //         }
        //     }
        //     out = cell->data;                          // 取数据（在发布之前读完）
        //     // release：把该格置为“下一圈可写”（pos + capacity）。下一圈生产者
        //     //   看到的 pos 正好等于这个值，于是 diff==0 轮到它写；与生产者
        //     //   enqueue 里对 cell.sequence 的 acquire 配对，传递“这格已空”。
        //     cell->sequence.store(pos + capacity_mask_ + 1, std::memory_order_release);
        //     return true;
        //
        // 占位实现：与 enqueue 占位共用同一把 mutex（FIFO 串行）。
        // -------------------------------------------------------------
        std::lock_guard<std::mutex> lk(placeholder_mutex_);
        if (placeholder_size_ == 0) {
            return false; // 空
        }
        Cell& cell = buffer_[placeholder_head_ & capacity_mask_];
        out        = cell.data;
        placeholder_head_++;
        placeholder_size_--;
        return true;
    }

private:
    // 每个 cell：一个数据槽 + 一个 sequence 闸门原子。整体对齐到缓存行，避免
    // 相邻 cell 的 sequence 互相伪共享（false sharing，模块 J）。
    struct alignas(64) Cell {
        std::atomic<std::size_t> sequence{0};
        T data{};
    };

    std::vector<Cell> buffer_;
    const std::size_t capacity_mask_; // capacity - 1（capacity 须为 2 的幂）

    // 两个票据计数器各占一条缓存行：生产者群只 CAS enqueue_pos_，消费者群只
    // CAS dequeue_pos_，把它们隔开能显著减少跨群伪共享。
    alignas(64) std::atomic<std::size_t> enqueue_pos_{0};
    alignas(64) std::atomic<std::size_t> dequeue_pos_{0};

    // ---- 占位实现专用状态（真正实现 Vyukov 后这些都应删除）----
    alignas(64) std::mutex placeholder_mutex_;
    std::size_t placeholder_head_{0};
    std::size_t placeholder_tail_{0};
    std::size_t placeholder_size_{0};
};

// =====================================================================
// 加锁版基线：std::mutex + std::queue，用于和无锁版做吞吐对比。
//   这是最朴素的“一把大锁保护整个队列”的并发队列；正确但争用下吞吐受限。
// =====================================================================
template <class T>
class LockedQueue {
public:
    explicit LockedQueue(std::size_t capacity) : capacity_(capacity) {}

    LockedQueue(const LockedQueue&)            = delete;
    LockedQueue& operator=(const LockedQueue&) = delete;

    bool enqueue(const T& value) {
        std::lock_guard<std::mutex> lk(mtx_);
        if (q_.size() >= capacity_) return false; // 满
        q_.push(value);
        return true;
    }

    bool dequeue(T& out) {
        std::lock_guard<std::mutex> lk(mtx_);
        if (q_.empty()) return false; // 空
        out = q_.front();
        q_.pop();
        return true;
    }

private:
    const std::size_t capacity_;
    std::mutex mtx_;
    std::queue<T> q_;
};

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

// =====================================================================
// 测试驱动与基准：全部用 stdout 打印（cs::logf / cs::println）。
// =====================================================================

// 必做 1：SPSC 正确性 —— 单生产者单消费者跑通，验证 FIFO 顺序与不丢数据。
static void demo_spsc_correctness() {
    cs::println("======== 必做 1：SPSC 正确性（FIFO + 不丢数据）========");

    constexpr std::size_t kCapacity = 1024; // 实际可用 1023（留一格）
    constexpr int kTotal            = 200000;

    SpscRingBuffer<int, kCapacity> ring;
    std::atomic<bool> order_ok{true};
    std::atomic<int> consumed{0};

    std::thread producer([&] {
        for (int i = 0; i < kTotal; ++i) {
            while (!ring.push(i)) std::this_thread::yield(); // 满则自旋等
        }
    });
    std::thread consumer([&] {
        int expected = 0, value = 0;
        while (expected < kTotal) {
            if (ring.pop(value)) {
                if (value != expected) order_ok.store(false, std::memory_order_relaxed);
                ++expected;
                consumed.fetch_add(1, std::memory_order_relaxed);
            } else {
                std::this_thread::yield();
            }
        }
    });
    producer.join();
    consumer.join();

    const int got = consumed.load();
    cs::logf("[SPSC] 消费总数 = ", got, "（期望 ", kTotal, "）-- ",
             (got == kTotal ? "不丢数据 OK" : "丢数据 MISMATCH"));
    cs::logf("[SPSC] FIFO 顺序检查 -- ",
             (order_ok.load() ? "严格递增、顺序正确 OK" : "顺序错乱 FAIL"));
    cs::println("");
}

// 必做 2 & 3：MPMC 正确性 —— 多生产者多消费者，验证“不丢、不重、总数守恒”。
//   每个元素是唯一编号；消费者用一张标记表核对“每个编号恰好被消费一次”。
//   注意：MPMC 下不保证全局 FIFO（多消费者并发取），故只验证“集合相等”。
static void demo_mpmc_correctness() {
    cs::println("======== 必做 2&3：MPMC 正确性（不丢 / 不重 / 守恒）========");

    constexpr std::size_t kCapacity   = 1024;   // 须为 2 的幂
    constexpr int kProducers          = 4;
    constexpr int kConsumers          = 4;
    constexpr int kPerProducer        = 50000;
    constexpr int kTotal              = kProducers * kPerProducer;

    MpmcBoundedQueue<int> queue(kCapacity);

    // 每个编号一个标记位；消费者 CAS 翻位以检测重复消费。
    std::vector<std::atomic<int>> seen(kTotal);
    for (auto& s : seen) s.store(0, std::memory_order_relaxed);

    std::atomic<int> produced{0};
    std::atomic<int> consumed{0};
    std::atomic<bool> dup_or_oob{false}; // 出现重复或越界即置 true
    std::atomic<bool> producers_done{false};

    std::vector<std::thread> threads;
    for (int p = 0; p < kProducers; ++p) {
        threads.emplace_back([&, p] {
            for (int i = 0; i < kPerProducer; ++i) {
                const int value = p * kPerProducer + i; // 全局唯一编号
                while (!queue.enqueue(value)) std::this_thread::yield(); // 满则等
                produced.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    for (int c = 0; c < kConsumers; ++c) {
        threads.emplace_back([&] {
            int value = 0;
            for (;;) {
                if (queue.dequeue(value)) {
                    if (value < 0 || value >= kTotal) {
                        dup_or_oob.store(true, std::memory_order_relaxed);
                    } else if (seen[value].fetch_add(1, std::memory_order_relaxed) != 0) {
                        dup_or_oob.store(true, std::memory_order_relaxed); // 重复消费
                    }
                    consumed.fetch_add(1, std::memory_order_relaxed);
                } else if (producers_done.load(std::memory_order_acquire) &&
                           consumed.load(std::memory_order_relaxed) >= kTotal) {
                    break; // 生产结束且已全部消费 → 退出
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    // 等所有生产者完成后再放行消费者退出条件。
    for (int p = 0; p < kProducers; ++p) threads[p].join();
    producers_done.store(true, std::memory_order_release);
    for (int c = kProducers; c < kProducers + kConsumers; ++c) threads[c].join();

    // 核对：每个编号恰好被消费一次。
    bool all_once = true;
    for (int i = 0; i < kTotal; ++i) {
        if (seen[i].load(std::memory_order_relaxed) != 1) { all_once = false; break; }
    }
    cs::logf("[MPMC] 生产 = ", produced.load(), "，消费 = ", consumed.load(),
             "（期望各 ", kTotal, "）");
    cs::logf("[MPMC] 守恒/不丢/不重 -- ",
             ((produced.load() == kTotal && consumed.load() == kTotal && all_once &&
               !dup_or_oob.load())
                  ? "每个编号恰好一次 OK"
                  : "出现丢失/重复 MISMATCH"));
    cs::println("");
}

// 通用吞吐基准：固定生产/消费总量，多生产者多消费者并发跑，计时算 ops/sec。
//   Q 需提供 enqueue(const int&)->bool 与 dequeue(int&)->bool。
template <class Q>
static double bench_throughput(Q& queue, const char* name, int producers,
                               int consumers, int per_producer) {
    const int total = producers * per_producer;
    std::atomic<int> consumed{0};
    std::atomic<bool> producers_done{false};
    std::vector<std::thread> threads;

    const auto t0 = std::chrono::steady_clock::now();

    for (int p = 0; p < producers; ++p) {
        threads.emplace_back([&, p] {
            for (int i = 0; i < per_producer; ++i) {
                const int value = p * per_producer + i;
                while (!queue.enqueue(value)) std::this_thread::yield();
            }
        });
    }
    for (int c = 0; c < consumers; ++c) {
        threads.emplace_back([&] {
            int value = 0;
            for (;;) {
                if (queue.dequeue(value)) {
                    consumed.fetch_add(1, std::memory_order_relaxed);
                } else if (producers_done.load(std::memory_order_acquire) &&
                           consumed.load(std::memory_order_relaxed) >= total) {
                    break;
                } else {
                    std::this_thread::yield();
                }
            }
        });
    }

    for (int p = 0; p < producers; ++p) threads[p].join();
    producers_done.store(true, std::memory_order_release);
    for (int c = producers; c < producers + consumers; ++c) threads[c].join();

    const auto t1 = std::chrono::steady_clock::now();
    const double secs =
        std::chrono::duration_cast<std::chrono::duration<double>>(t1 - t0).count();
    // 一次 enqueue + 一次 dequeue 记作 2 个 op。
    const double ops      = static_cast<double>(total) * 2.0;
    const double ops_per_s = secs > 0 ? ops / secs : 0.0;

    cs::logf("[bench] ", name, "：", total, " 元素，", producers, "P/", consumers,
             "C，用时 ", static_cast<long long>(secs * 1000), " ms，吞吐 ≈ ",
             static_cast<long long>(ops_per_s), " ops/sec");
    return ops_per_s;
}

// 必做 6：吞吐对比 —— MPMC 无锁队列 vs 加锁版（std::mutex + std::queue）。
static void demo_benchmark() {
    cs::println("======== 必做 6：吞吐对比（MPMC 无锁 vs 加锁版）========");

    const int hw         = static_cast<int>(std::thread::hardware_concurrency());
    const int producers  = hw >= 4 ? hw / 2 : 2;
    const int consumers  = producers;
    const int per_prod   = 200000;
    constexpr std::size_t kCapacity = 1024; // 2 的幂

    cs::logf("[bench] 配置：", producers, " 生产者 + ", consumers,
             " 消费者，每生产者 ", per_prod, " 元素，容量 ", kCapacity);

    MpmcBoundedQueue<int> lockfree(kCapacity);
    const double lf = bench_throughput(lockfree, "MPMC-lockfree", producers,
                                       consumers, per_prod);

    LockedQueue<int> locked(kCapacity);
    const double lk = bench_throughput(locked, "Locked(mutex+queue)", producers,
                                       consumers, per_prod);

    if (lk > 0) {
        cs::logf("[bench] 无锁/加锁 吞吐比 ≈ ",
                 static_cast<long long>((lf / lk) * 100), " %（>100% 表示无锁更快）");
    }
    cs::println("注：占位实现下 MPMC 内部也是一把锁，故此比值≈100%；填完 Vyukov");
    cs::println("    的 CAS 协议后，无锁版在高争用下通常明显领先。");
    cs::println("");
}

int main() {
    cs::println("==== Capstone2：无锁队列（SPSC 环形 + MPMC Vyukov 有界）====\n");

    demo_spsc_correctness();   // 必做 1
    demo_mpmc_correctness();   // 必做 2 & 3（占位实现下亦应通过）
    demo_benchmark();          // 必做 6

    cs::println("要点回顾：");
    cs::println("  - SPSC：head/tail 各独占写者 → 无 CAS，仅 release/acquire 配对；");
    cs::println("  - MPMC：每槽 sequence 闸门 + 在 tail/head 票据上 CAS（Vyukov）；");
    cs::println("  - 有界数组复用槽位 → 无 new/delete → 天然规避 ABA 与 use-after-free，");
    cs::println("    因此【无需】hazard pointer / RCU 这类回收机制（对比模块 I）。");
    cs::println("\n==== 演示结束。请对照文档“验收点/复盘问题”自检。 ====");
    return 0;
}
