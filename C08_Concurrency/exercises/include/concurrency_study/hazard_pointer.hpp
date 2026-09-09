#ifndef CONCURRENCY_STUDY_HAZARD_POINTER_HPP
#define CONCURRENCY_STUDY_HAZARD_POINTER_HPP

// C++23 教学协议；源指针的 load/store/exchange/CAS 全部 SC。
// 完整证明和契约：topics/reclamation/02-hazard-pointers.md。
#include <atomic>
#include <cstddef>
#include <exception>
#include <functional>
#include <list>
#include <memory>
#include <mutex>
#include <utility>

namespace cs {
inline constexpr std::size_t kMaxHazardPointers = 128;
namespace hp_detail {
struct slot {
    std::atomic<bool> active{false};
    std::atomic<const void*> pointer{nullptr};
};
struct retired {
    const void* address;
    std::move_only_function<void()> destroy;
};
struct domain {
    slot slots[kMaxHazardPointers];
    std::mutex pending_mutex;
    std::mutex collector_mutex;
    std::list<retired> pending;
    ~domain() { if (!pending.empty()) std::terminate(); }
};
inline domain& global() { static domain d; return d; }
inline thread_local bool collecting = false;
inline void release(slot* s) noexcept {
    if (!s) return;
    s->pointer.store(nullptr); // 先结束保护，再转交 active 槽。
    s->active.store(false);
}

// ponytail: O(退休数 × 128) 扫描与串行回调；需吞吐时换成熟 HP 库。
inline std::size_t cleanup(bool wait) noexcept {
    if (collecting) return 0; // 回调可 retire/cleanup；外层处理新增记录。
    auto& d = global();
    std::unique_lock collector(d.collector_mutex, std::defer_lock);
    if (wait) collector.lock();
    else if (!collector.try_lock()) return 0;
    collecting = true;
    std::size_t completed = 0;
    for (;;) {
        std::list<retired> ready;
        {
            std::lock_guard lock(d.pending_mutex);
            // 候选记录必须已经退休，不能先扫描再取新退休记录。
            for (auto it = d.pending.begin(); it != d.pending.end();) {
                bool protected_now = false;
                for (auto& s : d.slots) {
                    if (s.pointer.load() == it->address) {
                        protected_now = true;
                        break;
                    }
                }
                auto candidate = it++;
                if (!protected_now) ready.splice(ready.end(), d.pending, candidate);
            }
        }
        if (ready.empty()) break;
        for (auto& r : ready) {
            // 无 pending 锁，允许删除器重入 retire；异常越过 noexcept 时终止。
            r.destroy();
            ++completed;
        }
    }
    collecting = false;
    return completed;
}
} // namespace hp_detail

// 不等待保护解除。并发外层 cleanup 串行；同线程重入返回 0。
// 最终清场：停止生产、join 使用者、释放全部句柄，再调用一次。
inline std::size_t hazard_pointer_cleanup() noexcept { return hp_detail::cleanup(true); }

template<class T, class D = std::default_delete<T>>
class hazard_pointer_obj_base {
public:
    void retire(D deleter = D{}) noexcept {
        auto* p = static_cast<T*>(this);
        auto& d = hp_detail::global();
        // 在锁外构造类型擦除记录，删除器移动/销毁也不占 pending 锁。
        std::list<hp_detail::retired> incoming;
        incoming.push_back({p, [p, del = std::move(deleter)]() mutable { del(p); }});
        bool scan;
        {
            std::lock_guard lock(d.pending_mutex);
            // 保留实际实例，支持只能移动的删除器；分配/移动失败终止。
            d.pending.splice(d.pending.end(), incoming);
            scan = d.pending.size() >= 2 * kMaxHazardPointers;
        }
        if (scan) hp_detail::cleanup(false);
    }
protected:
    hazard_pointer_obj_base() = default;
    ~hazard_pointer_obj_base() = default;
};

class hazard_pointer {
public:
    hazard_pointer() noexcept = default;
    hazard_pointer(const hazard_pointer&) = delete;
    hazard_pointer& operator=(const hazard_pointer&) = delete;
    hazard_pointer(hazard_pointer&& other) noexcept
        : slot_(std::exchange(other.slot_, nullptr)) {}
    hazard_pointer& operator=(hazard_pointer&& other) noexcept {
        if (this != &other) {
            hp_detail::release(slot_);
            slot_ = std::exchange(other.slot_, nullptr);
        }
        return *this;
    }
    ~hazard_pointer() { hp_detail::release(slot_); }
    bool empty() const noexcept { return slot_ == nullptr; }
    template<class T>
    T* protect(const std::atomic<T*>& source) noexcept {
        T* p = source.load();
        while (!try_protect(p, source)) {}
        return p;
    }
    template<class T>
    bool try_protect(T*& expected, const std::atomic<T*>& source) noexcept {
        if (!slot_) std::terminate();
        slot_->pointer.store(expected);
        T* current = source.load();
        if (current == expected) {
            expected = current; // 返回刚从源取得的指针值。
            return true;
        }
        expected = current;
        slot_->pointer.store(nullptr); // 失败不授予 expected 解引用权。
        return false;
    }
    // p 必须已有独立的有效保护/独占所有权；此函数不校验源。
    template<class T>
    void reset_protection(T* p) noexcept {
        if (!slot_) std::terminate();
        // 与完整扫描互斥，防止扫描先读新槽空值、再读转交后旧槽空值。
        // p 的独立保护须一直维持到本函数返回。
        std::lock_guard lock(hp_detail::global().pending_mutex);
        slot_->pointer.store(p);
    }
    // 空/移动后句柄允许清空；reset 不释放 active 槽。
    void reset_protection(std::nullptr_t = nullptr) noexcept {
        if (slot_) slot_->pointer.store(nullptr);
    }
private:
    friend hazard_pointer make_hazard_pointer();
    explicit hazard_pointer(hp_detail::slot* s) noexcept : slot_(s) {}
    hp_detail::slot* slot_ = nullptr;
};

inline hazard_pointer make_hazard_pointer() {
    for (auto& s : hp_detail::global().slots) {
        bool unused = false;
        if (s.active.compare_exchange_strong(unused, true)) return hazard_pointer(&s);
    }
    throw std::bad_alloc();
}
} // namespace cs
#endif
