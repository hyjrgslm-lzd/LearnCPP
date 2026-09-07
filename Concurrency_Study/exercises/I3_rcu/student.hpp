#pragma once
#include "concurrency_study/rcu.hpp"
#include "../../topics/reclamation/experiment_support.hpp"
#include <atomic>
#include <memory>
#include <stdexcept>

namespace student {
struct config : cs::rcu_obj_base<config> {
    const int version, payload, checksum;
    reclamation_experiment::audit& counts;
    config(int v, reclamation_experiment::audit& a)
        : version(v), payload(v * 3), checksum(v * 4), counts(a) { ++counts.created; }
    ~config() { ++counts.destroyed; }
};

// Part A：use 必须在读区内调用；不准把裸指针/引用带出这个回调。
template<class Use>
void read(const std::atomic<config*>& source, cs::rcu_domain& domain, Use use) {
    (void)source; (void)domain; (void)use;
    // TODO A: 先创建 rcu_reader，再 load 源，检查非空并 use(*p)。
    throw std::logic_error("TODO A: reader");
}

inline void replace(std::atomic<config*>& source, std::unique_ptr<config> fresh,
                    cs::rcu_domain& domain) {
    (void)source; (void)fresh; (void)domain;
    // TODO B1: SC exchange 发布 fresh，旧值在同域 retire；处理 nullptr。
    throw std::logic_error("TODO B1: replace");
}

inline void checkpoint(cs::rcu_domain& domain) {
    (void)domain;
    // TODO B2: 等本域已退休回调结束；synchronize 不能代替此操作。
    throw std::logic_error("TODO B2: checkpoint");
}

// Part C：调用者已停止更新并 join 全部读者。
inline void shutdown(std::atomic<config*>& source, cs::rcu_domain& domain) {
    (void)source; (void)domain;
    // TODO C: 摘除最终值，同域 retire，然后完成所有删除回调。
    throw std::logic_error("TODO C: shutdown");
}
} // namespace student
