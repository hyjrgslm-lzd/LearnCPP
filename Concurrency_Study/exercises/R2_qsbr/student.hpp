#pragma once
#include "concurrency_study/epoch.hpp"
#include "../../topics/reclamation/experiment_support.hpp"
#include <atomic>
#include <stdexcept>

namespace student {
using value = reclamation_experiment::value;

inline cs::qsbr_participant online(cs::epoch_domain& domain) {
    (void)domain;
    // TODO A1: 创建并返回本线程在 domain 的在线 participant。
    throw std::logic_error("TODO A1: online");
}
inline int read(const std::atomic<value*>& source) {
    (void)source;
    // TODO A2: 调用者已在线；SC load，检查非空并复制 payload，不返回裸借用。
    throw std::logic_error("TODO A2: read");
}
inline void quiescent(cs::qsbr_participant& participant) {
    (void)participant;
    // TODO B: 之前的全部借用已经结束，向 participant 报告静默。
    throw std::logic_error("TODO B: quiescent");
}
inline void offline(cs::qsbr_participant& participant) {
    (void)participant;
    // TODO C1: 下线，离线期间不得读取源；不能只等待析构碰巧帮你下线。
    throw std::logic_error("TODO C1: offline");
}

// 检查器已先放行工作线程，并 get 等待它结束。
inline void shutdown(std::atomic<value*>& source, cs::epoch_domain& domain) {
    (void)source; (void)domain;
    // TODO C2: 摘除最终值、同域 retire，并等待全部回调结束。
    throw std::logic_error("TODO C2: shutdown");
}
} // namespace student
