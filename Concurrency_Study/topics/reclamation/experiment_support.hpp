#pragma once
// 仅供本专题实验/故障注入；不属于 cs 回收 API。
#include <atomic>
#include <exception>
#include <future>
#include <new>

namespace reclamation_experiment {
// 主线程控制的放行门：只存原子状态，可重复 open，不会二次满足 promise。
class gate {
    std::atomic<bool> open_{false};
public:
    void open() noexcept { open_.store(true); open_.notify_all(); }
    void wait() const noexcept { open_.wait(false); }
};

// 移入 worker；主线程只保留 get_future() 的结果，并用 get 接异常。
// 单个通知的发布动作由同一 worker（或它同步执行的回调）完成。
class signal {
    std::promise<void> promise_;
    bool sent_ = false;
public:
    std::future<void> get_future() { return promise_.get_future(); }
    void ready() {
        if (!sent_) { promise_.set_value(); sent_ = true; }
    }
    void fail(std::exception_ptr error) {
        if (!sent_) { promise_.set_exception(error); sent_ = true; }
    }
};

enum class failure { none, before_registration, before_allocation, before_publish,
                     before_report, before_secondary_launch, before_callback };
struct injected_bad_alloc : std::bad_alloc {
    failure point;
    explicit injected_bad_alloc(failure p) : point(p) {}
    const char* what() const noexcept override { return "reclamation injected bad_alloc"; }
};
inline void inject(failure requested, failure here) {
    if (requested == here) throw injected_bad_alloc(here);
}

// 只观察实验拥有的对象/线程。注入一次异常，不替换全局 new 或耗尽内存。
struct audit {
    std::atomic<int> created{0}, destroyed{0}, started{0}, finished{0};
};
struct value {
    int payload;
    audit& counts;
    value(int v, audit& a) : payload(v), counts(a) { ++counts.created; }
    ~value() { ++counts.destroyed; }
};
struct worker_lifetime {
    audit& counts;
    explicit worker_lifetime(audit& a) : counts(a) { ++counts.started; }
    ~worker_lifetime() { ++counts.finished; }
};
} // namespace reclamation_experiment
