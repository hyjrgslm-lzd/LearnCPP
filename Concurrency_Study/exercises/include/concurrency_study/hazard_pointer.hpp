// =====================================================================
// concurrency_study/hazard_pointer.hpp
//   风险指针（hazard pointer）的【教学版】实现，header-only，跨平台（MSVC/Clang/GCC）。
//
//   对应文档：Concurrency_Study/11-模块I-安全内存回收.md 的 练习 I-2
//
//   ★★★ 重要声明 ★★★
//   C++26 在 <hazard_pointer> 头、std 命名空间下标准化了风险指针
//   （提案 P2530R3 "Hazard Pointers for C++26"）。但截至本材料编写时，
//   主流编译器（含 MSVC VS2026）尚未提供该标准实现。
//   因此本文件在 cs:: 命名空间下提供一个【简化教学实现】，API 命名刻意贴近
//   std:: 的 C++26 版本，方便你日后无痛迁移。
//
//   >>> 这是教学用途的简化实现，不是标准、不是生产级：
//       - 槽位数固定（编译期常量），超额申请会抛异常（标准版可动态扩容）；
//       - 回收扫描是线性 O(槽位数 × retire 列表长度），无分代/批量优化；
//       - retire 列表是 per-thread 的，线程退出时机简化处理；
//       - 删除器仅支持无状态可默认构造的 D（够本套练习用）。
//       生产环境请用 folly::hazptr 或等待编译器的 std::hazard_pointer。
//
//   学习目标：
//     - 理解风险指针解决的核心难题：无锁结构里“线程 A 读到指向节点 N 的
//       指针、尚未解引用时，线程 B 把 N pop 出来并 delete”→ A 解引用悬垂
//       指针，即 use-after-free。这正是模块 G Treiber 栈“pop 故意泄漏”
//       的真正解法所在。
//     - 掌握保护-回收协议（protect / retire / reclaim）：
//         · 读者把“我正在用的指针”登记到一个全局可见的 hazard 槽里（protect）；
//         · 写者摘下节点后不立即 delete，而是放进 retire 列表（retire）；
//         · 回收时扫描所有 hazard 槽，只 delete 那些“没有任何读者正在保护”的节点。
//     - 对照 std:: C++26 API 与本 cs:: 教学 API 的差异（见文档映射表）。
//
//   官方参考：
//     - 提案 P2530R3 "Hazard Pointers for C++26"
//       https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2530r3.pdf
//     - 早期提案 P0566（并发数据结构 hazard pointer/RCU 的最初引入）
//     - cppreference（C++26）<hazard_pointer>：
//       https://en.cppreference.com/w/cpp/header/hazard_pointer
//     - folly Hazptr（对照阅读，工业级实现）：
//       https://github.com/facebook/folly/blob/main/folly/synchronization/Hazptr.h
//     - Maged M. Michael, "Hazard Pointers: Safe Memory Reclamation for
//       Lock-Free Objects", IEEE TPDS 2004（原始论文）
// =====================================================================
#ifndef CONCURRENCY_STUDY_HAZARD_POINTER_HPP
#define CONCURRENCY_STUDY_HAZARD_POINTER_HPP

#include <atomic>
#include <cstddef>
#include <memory>     // std::default_delete
#include <stdexcept>
#include <vector>

namespace cs {

// ---------------------------------------------------------------------
// 配置：全局 hazard 槽的总数。
//   每个线程同时持有的 hazard_pointer 个数受此上限约束。
//   实践经验：每线程 1~2 个槽通常够用（一次只“盯住”少量指针）。
//   教学实现用固定上限，超额 make_hazard_pointer() 会抛 bad_alloc。
//   （标准 std::make_hazard_pointer 失败时抛 bad_alloc，这里语义对齐。）
// ---------------------------------------------------------------------
inline constexpr std::size_t kMaxHazardPointers = 128;

namespace detail {

// 单个 hazard 槽：active 标记该槽是否被某线程占用；ptr 是被保护的指针。
//   ptr 用 void* 存放，protect 时存原始地址；回收方只需比较地址相等性，
//   不需要知道真实类型，因此 void* 足矣。
struct HazardSlot {
    std::atomic<bool> active{false};   // 槽是否已被某 hazard_pointer 占用
    std::atomic<void*> ptr{nullptr};   // 当前被保护的地址（nullptr = 未保护任何东西）
};

// 全局槽数组（整个进程共享一份）。函数内静态保证首次使用时初始化、线程安全。
inline HazardSlot* hazard_slots() {
    static HazardSlot slots[kMaxHazardPointers];
    return slots;
}

// 申请一个空闲槽：CAS 把 active 从 false 抢成 true。抢不到任何槽则返回 nullptr。
inline HazardSlot* acquire_slot() {
    HazardSlot* slots = hazard_slots();
    for (std::size_t i = 0; i < kMaxHazardPointers; ++i) {
        bool expected = false;
        // acq_rel：抢占成功后，本线程后续对 ptr 的写要对扫描者可见。
        if (slots[i].active.compare_exchange_strong(
                expected, true, std::memory_order_acq_rel, std::memory_order_relaxed)) {
            return &slots[i];
        }
    }
    return nullptr; // 所有槽都被占用
}

// 释放一个槽：先清空保护指针，再把 active 置回 false 供他人复用。
inline void release_slot(HazardSlot* slot) {
    if (slot == nullptr) return;
    slot->ptr.store(nullptr, std::memory_order_release);
    slot->active.store(false, std::memory_order_release);
}

// 判断地址 p 是否正被任何 hazard 槽保护（回收前的安全检查）。
inline bool is_hazardous(void* p) {
    HazardSlot* slots = hazard_slots();
    for (std::size_t i = 0; i < kMaxHazardPointers; ++i) {
        // acquire：与 protect 里对 ptr 的 release 配对，确保看到最新保护值。
        if (slots[i].ptr.load(std::memory_order_acquire) == p) {
            return true;
        }
    }
    return false;
}

// 一条待回收记录：被退休的对象指针 + 知道如何删除它的回收函数。
struct RetiredNode {
    void* raw;                 // 对象的“可识别地址”（与 hazard 槽里登记的地址一致）
    void (*deleter)(void*);    // 类型擦除后的删除器：内部还原真实类型再 delete
};

// per-thread 的 retire 列表：本线程退休的对象先攒在这里，攒够阈值就扫描回收。
//   做成 thread_local 是为了避免 retire 本身又要加锁——这正是 hazard pointer
//   的卖点之一：写者退休对象时几乎无争用。
inline std::vector<RetiredNode>& retire_list() {
    static thread_local std::vector<RetiredNode> list;
    return list;
}

// 扫描并回收：遍历本线程 retire 列表，凡是【没有被任何 hazard 槽保护】的对象，
//   立即调用其删除器 delete 掉；仍被保护的留到下次再试。
inline void scan_and_reclaim() {
    std::vector<RetiredNode>& list = retire_list();
    std::vector<RetiredNode> survivors;
    survivors.reserve(list.size());
    for (RetiredNode& node : list) {
        if (is_hazardous(node.raw)) {
            survivors.push_back(node);   // 还有人在用，暂不回收
        } else {
            node.deleter(node.raw);      // 没人保护，安全 delete
        }
    }
    list.swap(survivors);
}

// retire 列表攒到多少就触发一次扫描。设得比槽数大，摊销扫描成本。
inline constexpr std::size_t kReclaimThreshold = kMaxHazardPointers * 2;

} // namespace detail

// =====================================================================
// hazard_pointer_obj_base<T, D>
//   想被风险指针保护并安全回收的对象，必须【公有继承】此基类（CRTP 风格，
//   T 是派生类自身）。它提供 retire()：把对象交给回收系统延迟删除。
//
//   对应 std::hazard_pointer_obj_base<T, D>（C++26）。
//   D 是删除器类型，默认 std::default_delete<T>（即 delete）。教学实现
//   只要求 D 可默认构造、可对 T* 调用。
// =====================================================================
template <class T, class D = std::default_delete<T>>
class hazard_pointer_obj_base {
public:
    // 退休本对象：不立即 delete，而是登记到回收系统，等“确认没有读者正在
    //   保护它”之后再真正删除。这是写者侧的核心动作。
    //   语义对齐 std::hazard_pointer_obj_base::retire()。
    void retire(D deleter = D{}) noexcept {
        // 类型擦除：把“如何删除”封进一个普通函数指针。
        //   注意：retire 后绝不能再访问 *this（对象逻辑上已交出所有权）。
        detail::RetiredNode node;
        node.raw = static_cast<void*>(static_cast<T*>(this));
        // 把删除器实例存进静态以便无捕获 lambda 取用；教学实现要求 D 无状态。
        node.deleter = [](void* p) {
            D d{};
            d(static_cast<T*>(p));
        };
        (void)deleter; // 教学实现忽略传入实例，统一用默认构造的 D（D 须无状态）
        detail::retire_list().push_back(node);

        // 攒够阈值就扫描回收一批，摊销开销。
        if (detail::retire_list().size() >= detail::kReclaimThreshold) {
            detail::scan_and_reclaim();
        }
    }

protected:
    hazard_pointer_obj_base() = default;
    ~hazard_pointer_obj_base() = default;
};

// =====================================================================
// hazard_pointer
//   一个 RAII 句柄，持有一个全局 hazard 槽。读者用它来“保护”一个指针：
//   只要本对象活着且 protect 了 p，回收系统就不会 delete p 指向的对象。
//
//   对应 std::hazard_pointer（C++26）。通过 make_hazard_pointer() 构造，
//   只可移动、不可拷贝（独占一个槽）。
// =====================================================================
class hazard_pointer {
public:
    // 空句柄（不持有任何槽）。is_empty() 为真。
    hazard_pointer() noexcept = default;

    // 只移动不拷贝：一个槽只能被一个句柄拥有。
    hazard_pointer(const hazard_pointer&) = delete;
    hazard_pointer& operator=(const hazard_pointer&) = delete;

    hazard_pointer(hazard_pointer&& other) noexcept : slot_(other.slot_) {
        other.slot_ = nullptr;
    }
    hazard_pointer& operator=(hazard_pointer&& other) noexcept {
        if (this != &other) {
            detail::release_slot(slot_);
            slot_ = other.slot_;
            other.slot_ = nullptr;
        }
        return *this;
    }

    ~hazard_pointer() { detail::release_slot(slot_); }

    // 是否是空句柄（未持有槽）。对应 std::hazard_pointer::empty()。
    bool empty() const noexcept { return slot_ == nullptr; }

    // -----------------------------------------------------------------
    // protect：保护并返回 src 的当前值，保证“返回的指针在本 hazard_pointer
    //   reset/析构前不会被回收”。
    //
    //   为什么要循环重读？因为 store 到 hazard 槽 与 读 src 之间存在时间窗：
    //   登记前 src 可能已被改并回收。协议是【登记 → 重读校验】：
    //     1) p = src.load();
    //     2) 把 p 写进 hazard 槽（发布“我在保护 p”）；
    //     3) 重读 src，若仍等于 p，说明在我登记期间它没被换走，保护成立；
    //        否则用新值重试。
    //
    //   对应 std::hazard_pointer::protect(const std::atomic<T*>&)。
    // -----------------------------------------------------------------
    template <class T>
    T* protect(const std::atomic<T*>& src) noexcept {
        T* p = src.load(std::memory_order_acquire);
        while (true) {
            // 发布“本线程正在保护 p”。seq_cst/release 确保扫描者能看到。
            slot_->ptr.store(static_cast<void*>(p), std::memory_order_seq_cst);
            T* p2 = src.load(std::memory_order_acquire);
            if (p2 == p) {
                return p; // 登记期间未被换走，保护成立
            }
            p = p2;       // 被换走了，用新值重试
        }
    }

    // -----------------------------------------------------------------
    // try_protect：尝试把 expected 登记为被保护指针，并重读 src 校验。
    //   若 src 当前值仍等于 expected → 返回 true（保护成立）；
    //   否则把 expected 更新为 src 的最新值并返回 false（由调用方决定是否重试）。
    //   对应 std::hazard_pointer::try_protect(T*&, const std::atomic<T*>&)。
    // -----------------------------------------------------------------
    template <class T>
    bool try_protect(T*& expected, const std::atomic<T*>& src) noexcept {
        T* p = expected;
        slot_->ptr.store(static_cast<void*>(p), std::memory_order_seq_cst);
        T* current = src.load(std::memory_order_acquire);
        if (current == p) {
            return true;
        }
        expected = current; // 回填最新值供调用方决定重试
        return false;
    }

    // 直接把某个已知指针登记为被保护（不读 atomic 源）。
    //   对应 std::hazard_pointer::reset_protection(const T*) 的“设值”用法。
    template <class T>
    void reset_protection(T* p) noexcept {
        slot_->ptr.store(static_cast<void*>(const_cast<T*>(p)),
                         std::memory_order_seq_cst);
    }

    // 清除保护（不再保护任何指针）。对应 reset_protection(nullptr)。
    void reset_protection(std::nullptr_t = nullptr) noexcept {
        slot_->ptr.store(nullptr, std::memory_order_release);
    }

private:
    friend hazard_pointer make_hazard_pointer();
    explicit hazard_pointer(detail::HazardSlot* slot) noexcept : slot_(slot) {}

    detail::HazardSlot* slot_ = nullptr;
};

// =====================================================================
// make_hazard_pointer
//   工厂函数：申请一个空闲全局槽并返回持有它的 hazard_pointer。
//   槽耗尽时抛 std::bad_alloc（语义对齐 std::make_hazard_pointer）。
// =====================================================================
inline hazard_pointer make_hazard_pointer() {
    detail::HazardSlot* slot = detail::acquire_slot();
    if (slot == nullptr) {
        throw std::bad_alloc(); // 槽用尽：调小并发保护数或增大 kMaxHazardPointers
    }
    return hazard_pointer(slot);
}

} // namespace cs

#endif // CONCURRENCY_STUDY_HAZARD_POINTER_HPP
