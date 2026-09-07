#pragma once
#include "concurrency_study/epoch.hpp"
#include "../../topics/reclamation/experiment_support.hpp"
#include <atomic>
#include <memory>
#include <stdexcept>

namespace student {
using value = reclamation_experiment::value;

// Part A：返回作用域 guard；C++17 起保证返回 prvalue 的复制消除。
inline cs::epoch_guard pin(cs::epoch_domain& domain) {
    (void)domain;
    // TODO A: 返回本域 epoch_guard，调用者在 guard 存活期间读取源。
    throw std::logic_error("TODO A: pin");
}

// Part B：输入已经从共享源摘除，独占所有权须转交 domain。
inline void retire(std::unique_ptr<value> old, cs::epoch_domain& domain) {
    (void)old; (void)domain;
    // TODO B1: domain.retire 接管 old；不能在长读者存活时直接删除。
    throw std::logic_error("TODO B1: retire");
}
inline std::size_t collect(cs::epoch_domain& domain) {
    (void)domain;
    // TODO B2: 尝试回收，不等长读者；返回本次完成回调数。
    throw std::logic_error("TODO B2: collect");
}

// Part C：由检查器先结束读区并 join，再调用此函数。
inline void shutdown(std::atomic<value*>& source, cs::epoch_domain& domain) {
    (void)source; (void)domain;
    // TODO C: 清空源并退休最终值，等待退休回调全部完成。
    throw std::logic_error("TODO C: shutdown");
}
} // namespace student
