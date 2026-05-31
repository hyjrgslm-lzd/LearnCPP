// =====================================================================
// concurrency_study/rcu.hpp
//   RCU（read-copy-update，读-复制-更新）的【教学版】实现，header-only，
//   跨平台（MSVC/Clang/GCC）。
//
//   对应文档：Concurrency_Study/11-模块I-安全内存回收.md 的 练习 I-3
//
//   ★★★ 重要声明 ★★★
//   C++26 在 <rcu> 头、std 命名空间下标准化了 RCU
//   （提案 P2545R4 "Read-Copy Update (RCU)"）。但截至本材料编写时，
//   主流编译器（含 MSVC VS2026）尚未提供该标准实现。
//   因此本文件在 cs:: 命名空间下提供一个【简化教学实现】，API 命名刻意贴近
//   std:: 的 C++26 版本，方便你日后无痛迁移。
//
//   >>> 这是教学用途的简化实现，不是标准、不是生产级：
//       - 只提供一个全局默认域（rcu_default_domain），不支持多域并存的全部语义；
//       - synchronize 用“轮询所有读者计数归零”的朴素自旋等待，非工业级；
//       - retire 后的回收在下一次 synchronize 时统一进行（简化的宽限期）；
//       - 不处理读侧重入计数溢出等极端情形。
//       生产环境请用 folly::rcu 或等待编译器的 std::rcu_*。
//
//   学习目标：
//     - 理解 RCU 的灵魂：读者侧【几乎零开销】——进入读临界区只是标记一下
//       （本实现用 per-thread 计数 +1），不加锁、不写共享原子的争用热点；
//       写者负责“复制-修改-发布新版本”，旧版本不立即释放，而是 retire，
//       等到【所有当前读者都离开了它们的读临界区】（宽限期 grace period 结束）
//       才安全回收。
//     - 区分它与 hazard pointer：hazard pointer 精确登记“我在用哪个指针”，
//       RCU 用更粗的“宽限期”——不追踪具体指针，只确保“老读者都走光了”。
//       RCU 读侧更便宜，但回收延迟更不可控（要等最慢的读者离开）。
//     - 掌握 rcu_domain::lock/unlock 标记读临界区、rcu_retire/rcu_synchronize
//       的写者协议。
//
//   官方参考：
//     - 提案 P2545R4 "Read-Copy Update (RCU)"
//       https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2545r4.pdf
//     - 早期提案 P0566（hazard pointer / RCU 的最初引入）
//     - cppreference（C++26）<rcu>：
//       https://en.cppreference.com/w/cpp/header/rcu
//     - folly RCU（对照阅读，工业级实现）：
//       https://github.com/facebook/folly/blob/main/folly/synchronization/Rcu.h
//     - Paul E. McKenney 等关于 Linux 内核 RCU 的系列文章（RCU 的发源地）
// =====================================================================
#ifndef CONCURRENCY_STUDY_RCU_HPP
#define CONCURRENCY_STUDY_RCU_HPP

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

namespace cs {

namespace detail {

// per-thread 读侧深度计数：>0 表示本线程当前正处在 RCU 读临界区内。
//   读者进入临界区只是把它 +1、离开 -1，全是【本线程私有】的原子，
//   没有跨线程争用——这就是 RCU“读侧几乎零开销”的来源。
struct ReaderState {
    std::atomic<std::uint64_t> depth{0}; // 嵌套读临界区深度
    std::atomic<bool> registered{false}; // 是否已登记到全局读者表
};

// 全局读者表：收集所有线程的 ReaderState 指针，供写者 synchronize 时轮询。
//   用一把互斥锁保护“注册”这一低频动作（每线程一生只注册一次）。
struct ReaderRegistry {
    std::mutex mtx;
    std::vector<ReaderState*> readers;
};

inline ReaderRegistry& reader_registry() {
    static ReaderRegistry reg;
    return reg;
}

// 本线程的读侧状态（首次访问时创建并登记到全局表）。
inline ReaderState& this_reader() {
    static thread_local ReaderState state;
    if (!state.registered.load(std::memory_order_relaxed)) {
        state.registered.store(true, std::memory_order_relaxed);
        ReaderRegistry& reg = reader_registry();
        std::lock_guard<std::mutex> lk(reg.mtx);
        reg.readers.push_back(&state);
    }
    return state;
}

// 一条待回收记录：退休对象指针 + 类型擦除的删除器。
struct RcuRetired {
    void* raw;
    void (*deleter)(void*);
};

// 全局 retire 列表（写者把旧版本放这里，等下一个宽限期统一回收）。
//   写者远比读者少，这里用锁保护足够；语义清晰优先于极致性能。
struct RetireBuffer {
    std::mutex mtx;
    std::vector<RcuRetired> pending;
};

inline RetireBuffer& retire_buffer() {
    static RetireBuffer buf;
    return buf;
}

} // namespace detail

// =====================================================================
// rcu_domain
//   读临界区的标记器。读者调用 lock() 进入、unlock() 离开（可嵌套）。
//   本教学实现里 rcu_domain 是无状态的轻量壳，所有 lock/unlock 都作用在
//   调用线程的 per-thread 计数上。
//
//   对应 std::rcu_domain（C++26）。标准版 lock/unlock 也满足 BasicLockable，
//   因此可以配合 std::scoped_lock 使用（见下方 rcu_reader 辅助类）。
// =====================================================================
class rcu_domain {
public:
    // 进入读临界区：本线程读深度 +1。
    //   acquire 语义：进入临界区后对受保护数据的读，不应被重排到“标记进入”之前。
    //   （acquire 管「之后的读不前移」；release 管「之前的写不后移」——方向勿混。）
    void lock() noexcept {
        detail::this_reader().depth.fetch_add(1, std::memory_order_acquire);
    }

    // 离开读临界区：本线程读深度 -1。
    //   release 语义：临界区内的读必须在“标记离开”之前完成，
    //   这样写者一旦看到 depth 归零，就能确信本读者已不再触碰旧版本。
    void unlock() noexcept {
        detail::this_reader().depth.fetch_sub(1, std::memory_order_release);
    }
};

// 进程级默认域。对应 std::rcu_default_domain()。
inline rcu_domain& rcu_default_domain() noexcept {
    static rcu_domain dom;
    return dom;
}

// =====================================================================
// rcu_synchronize
//   写者调用：阻塞等待，直到“当前所有正处于读临界区的读者都至少离开过一次”
//   ——即宽限期（grace period）结束。返回后，凡是【在本次调用之前 retire 的
//   旧版本】都已不可能被任何读者触碰，可安全回收。
//
//   教学实现采用最朴素的策略：拍一张“此刻谁在读”的快照，然后自旋等待这些
//   读者的 depth 归零（哪怕只归零过一瞬）。真实 RCU 用更精巧的 epoch 推进
//   避免无限期等待新读者，这里为可读性做了简化。
//
//   对应 std::rcu_synchronize()（无参版作用于默认域）。
// =====================================================================
inline void rcu_synchronize(rcu_domain& = rcu_default_domain()) {
    detail::ReaderRegistry& reg = detail::reader_registry();

    // 1) 快照：记录此刻处于读临界区的读者（depth > 0）。
    std::vector<detail::ReaderState*> snapshot;
    {
        std::lock_guard<std::mutex> lk(reg.mtx);
        for (detail::ReaderState* r : reg.readers) {
            if (r->depth.load(std::memory_order_acquire) > 0) {
                snapshot.push_back(r);
            }
        }
    }

    // 2) 等待快照中的每个老读者至少离开一次它当时的读临界区。
    //    注意：我们只关心“拍快照时正在读的人”离开；之后新进入的读者
    //    必然看到的是新版本（写者已 publish），不关心旧版本，无需等待。
    for (detail::ReaderState* r : snapshot) {
        while (r->depth.load(std::memory_order_acquire) > 0) {
            std::this_thread::yield(); // 让出 CPU，别忙等烧满核
        }
    }
}

// =====================================================================
// rcu_obj_base<T, D>
//   想被 RCU 延迟回收的对象继承此基类（CRTP，T 为派生类自身）。
//   提供 retire()：把旧版本登记到全局 retire 列表，等下一个宽限期后回收。
//
//   对应 std::rcu_obj_base<T, D>（C++26）。
// =====================================================================
template <class T, class D = std::default_delete<T>>
class rcu_obj_base {
public:
    // 退休本对象：登记到全局待回收列表，不立即 delete。
    //   语义对齐 std::rcu_obj_base::retire()。
    //   教学实现要求 D 无状态（可默认构造）。
    void retire(D = D{}) noexcept {
        detail::RcuRetired rec;
        rec.raw = static_cast<void*>(static_cast<T*>(this));
        rec.deleter = [](void* p) {
            D d{};
            d(static_cast<T*>(p));
        };
        detail::RetireBuffer& buf = detail::retire_buffer();
        std::lock_guard<std::mutex> lk(buf.mtx);
        buf.pending.push_back(rec);
    }

protected:
    rcu_obj_base() = default;
    ~rcu_obj_base() = default;
};

// =====================================================================
// rcu_retire
//   非侵入式退休：把任意 T* 交给 RCU 延迟回收（无需继承 rcu_obj_base）。
//   对应 std::rcu_retire<T, D>(T* p, D d)。教学实现要求 D 无状态。
// =====================================================================
template <class T, class D = std::default_delete<T>>
inline void rcu_retire(T* p, D = D{}) {
    if (p == nullptr) return;
    detail::RcuRetired rec;
    rec.raw = static_cast<void*>(p);
    rec.deleter = [](void* q) {
        D d{};
        d(static_cast<T*>(q));
    };
    detail::RetireBuffer& buf = detail::retire_buffer();
    std::lock_guard<std::mutex> lk(buf.mtx);
    buf.pending.push_back(rec);
}

// =====================================================================
// rcu_barrier
//   阻塞直到所有【已 retire】的对象都被实际回收。教学实现：先等一个宽限期，
//   再把当前 pending 列表全部删除。对应 std::rcu_barrier()。
//
//   注意：这会回收“调用时刻 pending 列表里的全部对象”。它既是显式回收点，
//   也是“确保析构都跑完”的同步点（如程序退出前清场）。
// =====================================================================
inline void rcu_barrier(rcu_domain& dom = rcu_default_domain()) {
    rcu_synchronize(dom); // 等当前读者离开，保证 pending 中的旧版本无人引用

    std::vector<detail::RcuRetired> to_delete;
    {
        detail::RetireBuffer& buf = detail::retire_buffer();
        std::lock_guard<std::mutex> lk(buf.mtx);
        to_delete.swap(buf.pending);
    }
    for (detail::RcuRetired& rec : to_delete) {
        rec.deleter(rec.raw); // 宽限期已过，安全删除
    }
}

// =====================================================================
// rcu_reader（便利 RAII 包装，非标准 API，仅教学加糖）
//   在作用域内进入/离开默认域的读临界区，避免手写 lock/unlock 漏配对。
//   标准里你可以直接 std::scoped_lock lk(std::rcu_default_domain());
// =====================================================================
class rcu_reader {
public:
    explicit rcu_reader(rcu_domain& dom = rcu_default_domain()) : dom_(dom) {
        dom_.lock();
    }
    ~rcu_reader() { dom_.unlock(); }
    rcu_reader(const rcu_reader&) = delete;
    rcu_reader& operator=(const rcu_reader&) = delete;

private:
    rcu_domain& dom_;
};

} // namespace cs

#endif // CONCURRENCY_STUDY_RCU_HPP
