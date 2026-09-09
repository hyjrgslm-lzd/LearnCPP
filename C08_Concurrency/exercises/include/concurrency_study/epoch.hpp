#ifndef CONCURRENCY_STUDY_EPOCH_HPP
#define CONCURRENCY_STUDY_EPOCH_HPP

// 教学分代回收：EBR/QSBR/RCU 共用核心；非任一工业库的仿制。
// 源指针全部 SC；发布后不可变；域须长于所有参与者及回调。
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <utility>

namespace cs {
class qsbr_participant;
class epoch_domain {
    struct reader { std::uint64_t generation; std::size_t depth; bool qsbr; };
    struct retired { std::uint64_t generation; std::move_only_function<void()> destroy; };
    // ponytail: 短锁保护分代表、串行回调；完整操作不承诺 lock-free。
    std::mutex mutex_;
    std::condition_variable changed_;
    std::mutex collector_;
    std::map<std::thread::id, reader> readers_;
    std::list<retired> pending_;
    std::uint64_t generation_ = 1;
    std::size_t outstanding_ = 0; // 包括已取走、尚未完成的回调。
    inline static thread_local unsigned callback_depth_ = 0;

    std::uint64_t advance_locked() noexcept {
        if (generation_ == std::numeric_limits<std::uint64_t>::max()) std::terminate();
        return generation_++;
    }
    bool past_locked(std::uint64_t cutoff) const {
        for (const auto& [id, r] : readers_) {
            (void)id;
            if (r.generation <= cutoff) return false;
        }
        return true;
    }
    void check_wait_allowed() {
        if (callback_depth_) throw std::logic_error("wait inside reclamation callback");
        std::lock_guard lock(mutex_);
        if (readers_.contains(std::this_thread::get_id()))
            throw std::logic_error("wait inside this domain's read region");
    }
    std::size_t invoke(std::list<retired>& batch) noexcept {
        const auto count = batch.size();
        ++callback_depth_;
        for (auto& r : batch) {
            r.destroy(); // 回调异常终止，同一记录不重试。
            r.destroy = nullptr; // 删除器捕获状态也在 barrier 返回前销毁。
            std::lock_guard lock(mutex_);
            --outstanding_;
        }
        --callback_depth_;
        return count;
    }
    void enter(bool qsbr) {
        std::lock_guard lock(mutex_);
        auto [it, fresh] = readers_.try_emplace(std::this_thread::get_id(),
                                               reader{generation_, 0, qsbr});
        if (!fresh && (qsbr || it->second.qsbr))
            throw std::logic_error("one QSBR participant per thread/domain; no mixed guards");
        if (it->second.depth == std::numeric_limits<std::size_t>::max()) std::terminate();
        ++it->second.depth; // 嵌套 EBR/RCU 不刷新最外层 generation。
    }
    void quiescent() {
        std::lock_guard lock(mutex_);
        auto it = readers_.find(std::this_thread::get_id());
        if (it == readers_.end() || !it->second.qsbr)
            throw std::logic_error("quiescent requires an online QSBR participant");
        it->second.generation = generation_;
        changed_.notify_all();
    }
    friend class qsbr_participant;
public:
    epoch_domain() = default;
    epoch_domain(const epoch_domain&) = delete;
    epoch_domain& operator=(const epoch_domain&) = delete;
    ~epoch_domain() {
        // 不在析构中隐藏阻塞/回调；应用显式 join + barrier。
        if (!readers_.empty() || outstanding_ != 0) std::terminate();
    }
    void lock() { enter(false); }
    void unlock() noexcept {
        std::lock_guard lock(mutex_);
        auto it = readers_.find(std::this_thread::get_id());
        if (it == readers_.end()) std::terminate();
        if (--it->second.depth == 0) readers_.erase(it);
        changed_.notify_all();
    }
    template<class T, class D = std::default_delete<T>>
    void retire(T* p, D deleter = D{}) noexcept {
        if (!p) return;
        std::list<retired> incoming;
        incoming.push_back({0, [p, del = std::move(deleter)]() mutable { del(p); }});
        std::lock_guard lock(mutex_);
        incoming.front().generation = generation_;
        pending_.splice(pending_.end(), incoming);
        ++outstanding_;
        advance_locked(); // 此后进入的读者不阻挡这一记录。
    }
    std::size_t pending() {
        std::lock_guard lock(mutex_);
        return outstanding_;
    }
    // 不等待读者或其他收集者；短锁/回调本身可能阻塞。
    std::size_t collect() {
        if (callback_depth_) return 0;
        std::unique_lock serial(collector_, std::try_to_lock);
        if (!serial.owns_lock()) return 0;
        std::list<retired> ready;
        {
            std::lock_guard lock(mutex_);
            for (auto it = pending_.begin(); it != pending_.end();) {
                auto candidate = it++;
                if (past_locked(candidate->generation))
                    ready.splice(ready.end(), pending_, candidate);
            }
        }
        return invoke(ready);
    }
    void synchronize() {
        check_wait_allowed();
        std::unique_lock lock(mutex_);
        const auto cutoff = advance_locked();
        changed_.wait(lock, [&] { return past_locked(cutoff); });
        // 仅等待读区；不取退休队列，不执行回调。
    }
    void barrier() {
        check_wait_allowed(); // 抢 collector_ 前检测自等待。
        std::lock_guard serial(collector_); // 覆盖其他收集者已经取走的回调。
        std::list<retired> batch;
        {
            std::unique_lock lock(mutex_);
            batch.splice(batch.end(), pending_); // 固定批次后才开始宽限期。
            if (batch.empty()) return;
            const auto cutoff = advance_locked();
            changed_.wait(lock, [&] { return past_locked(cutoff); });
        }
        invoke(batch);
        // 回调中或取批次后新 retire 的记录留待下次 barrier。
    }
};

class epoch_guard {
    epoch_domain& domain_;
    const std::thread::id owner_ = std::this_thread::get_id();
public:
    explicit epoch_guard(epoch_domain& d) : domain_(d) { d.lock(); }
    ~epoch_guard() {
        if (owner_ != std::this_thread::get_id()) std::terminate();
        domain_.unlock();
    }
    epoch_guard(const epoch_guard&) = delete;
    epoch_guard& operator=(const epoch_guard&) = delete;
};

class qsbr_participant {
    epoch_domain& domain_;
    const std::thread::id owner_ = std::this_thread::get_id();
    bool online_ = false;
    void check_owner() const {
        if (owner_ != std::this_thread::get_id())
            throw std::logic_error("QSBR participant is thread-affine");
    }
public:
    explicit qsbr_participant(epoch_domain& d) : domain_(d) { online(); }
    ~qsbr_participant() { offline(); }
    qsbr_participant(const qsbr_participant&) = delete;
    qsbr_participant& operator=(const qsbr_participant&) = delete;
    void online() {
        check_owner();
        if (!online_) { domain_.enter(true); online_ = true; }
    }
    // 应用承诺此前全部借用已结束；此后必须重新加载源指针。
    void quiescent() {
        check_owner();
        if (!online_) throw std::logic_error("quiescent while offline");
        domain_.quiescent();
    }
    void offline() noexcept {
        if (owner_ != std::this_thread::get_id()) std::terminate();
        if (online_) { domain_.unlock(); online_ = false; }
    }
};
} // namespace cs
#endif
