#ifndef CONCURRENCY_STUDY_QUEUE_LINKED_HPP
#define CONCURRENCY_STUDY_QUEUE_LINKED_HPP
#include "hazard_pointer.hpp"
#include <atomic>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace cs::queue_lab {
// This header relies on cs HP's ALL-SC source-pointer contract. Never weaken
// head/tail/next operations independently. The domain uses locks and allocation;
// the COMPLETE operations below do not promise lock-freedom.
template<class T>
class treiber_stack {
    static_assert(std::is_nothrow_copy_assignable_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);
    struct node : cs::hazard_pointer_obj_base<node> {
        explicit node(const T& v) : value(v) {}
        T value;
        node* next = nullptr; // immutable after publication
    };
public:
    treiber_stack() = default;
    treiber_stack(const treiber_stack&) = delete;
    ~treiber_stack() {
        // Quiescence required. Live nodes were never retired.
        auto* p = head_.load();
        while (p) { auto* next = p->next; delete p; p = next; }
        cs::hazard_pointer_cleanup();
    }
    bool try_push(const T& value) {
        auto p = std::make_unique<node>(value);
        p->next = head_.load();
        while (!head_.compare_exchange_weak(p->next, p.get())) {}
        p.release();
        return true;
    }
    bool try_pop(T& value) {
        auto guard = cs::make_hazard_pointer();
        for (;;) {
            auto* old = guard.protect(head_);
            if (!old) return false;
            auto* next = old->next;
            if (head_.compare_exchange_strong(old, next)) {
                value = std::as_const(old->value);
                old->retire();
                return true;
            }
            // CAS's updated old has no protection; restart protect, not deref.
        }
    }
    bool atomics_lock_free() const noexcept { return head_.is_lock_free(); }
private:
    std::atomic<node*> head_{nullptr};
};

template<class T>
class ms_queue {
    static_assert(std::is_nothrow_copy_assignable_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);
    struct node : cs::hazard_pointer_obj_base<node> {
        node() = default;
        explicit node(const T& v) : value(v) {}
        const std::optional<T> value; // NEVER moved from/reset by pop
        std::atomic<node*> next{nullptr};
    };
public:
    ms_queue() {
        auto* dummy = new node;
        head_.store(dummy);
        tail_.store(dummy);
    }
    ms_queue(const ms_queue&) = delete;
    ~ms_queue() {
        // All operations and their guards must have ended before destruction.
        auto* p = head_.load();
        while (p) { auto* next = p->next.load(); delete p; p = next; }
        cs::hazard_pointer_cleanup();
    }
    bool try_push(const T& value) {
        auto p = std::make_unique<node>(value);
        auto guard = cs::make_hazard_pointer(); // may throw before publication
        for (;;) {
            auto* tail = guard.protect(tail_);
            auto* next = tail->next.load();
            if (tail != tail_.load()) continue;
            if (next) {
                // next is only a CAS value, never dereferenced here. Protected
                // tail prevents its address being recycled to fool this CAS.
                tail_.compare_exchange_strong(tail, next);
            } else if (tail->next.compare_exchange_strong(next, p.get())) {
                auto* published = p.release(); // link CAS linearizes enqueue
                tail_.compare_exchange_strong(tail, published);
                return true;
            }
        }
    }
    bool try_pop(T& value) { return try_pop(value, []() noexcept {}); }
    template<class Hook>
    bool try_pop(T& value, Hook removed) {
        static_assert(std::is_nothrow_invocable_v<Hook>);
        auto head_guard = cs::make_hazard_pointer();
        auto next_guard = cs::make_hazard_pointer();
        for (;;) {
            auto* head = head_guard.protect(head_);
            auto* tail = tail_.load(); // compare only, no dereference
            auto* next = next_guard.protect(head->next);
            // A retired head keeps its old next forever. Validating that
            // stale source alone is insufficient; revalidate the ROOT before
            // dereferencing next. All pointer accesses here are SC.
            if (head != head_.load()) continue;
            if (!next) return false; // linearizes at null next load
            if (head == tail) {
                tail_.compare_exchange_strong(tail, next);
                continue;
            }
            if (head_.compare_exchange_strong(head, next)) {
                removed(); // test-only pause with BOTH protections still held
                // Other consumers may also read this payload. Copy immutable
                // data under next protection, including AFTER successful CAS.
                value = *next->value; // const optional yields const T&, matching the trait
                head->retire();
                return true;
            }
        }
    }
    bool atomics_lock_free() const noexcept {
        return head_.is_lock_free() && tail_.is_lock_free()
            && head_.load()->next.is_lock_free(); // call only while quiescent
    }
private:
    std::atomic<node*> head_{nullptr}, tail_{nullptr};
};
} // namespace cs::queue_lab
#endif
