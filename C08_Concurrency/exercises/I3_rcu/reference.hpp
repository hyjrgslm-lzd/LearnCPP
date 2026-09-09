#pragma once
#include "concurrency_study/rcu.hpp"
#include "concurrency_study/exercise_check.hpp"
#include "../../topics/reclamation/experiment_support.hpp"
#include <atomic>
#include <future>
#include <iostream>
#include <memory>
#include <vector>

namespace rcu_exercise {
struct config : cs::rcu_obj_base<config> {
    const int version, payload, checksum;
    reclamation_experiment::audit& counts;
    inline static std::atomic<int> destroyed{0};
    explicit config(int v, reclamation_experiment::audit& a)
        : version(v), payload(v * 3), checksum(v * 4), counts(a) { ++counts.created; }
    ~config() { ++destroyed; ++counts.destroyed; }
};
inline void run(reclamation_experiment::failure fault = reclamation_experiment::failure::none,
                reclamation_experiment::audit* observed = nullptr) {
    namespace lab = reclamation_experiment;
    lab::audit local;
    auto& counts = observed ? *observed : local;
    constexpr int readers = 4, reads = 2000, updates = 64;
    const int before = config::destroyed.load();
    cs::rcu_domain domain;
    std::atomic<config*> current{nullptr};
    std::vector<std::future<int>> tasks;
    const auto finish = [&] {
        // 无父线程控制门闩，读者循环有限；仍须全部收束后才能删除当前值。
        for (auto& task : tasks) if (task.valid()) task.wait();
        delete current.exchange(nullptr);
        cs::rcu_barrier(domain);
    };
    try {
        tasks.reserve(readers);
        current.store(new config(0, counts));
        for (int r = 0; r < readers; ++r) {
            tasks.push_back(std::async(std::launch::async, [&] {
                lab::worker_lifetime lifetime(counts);
                lab::inject(fault, lab::failure::before_registration);
                int valid = 0;
                for (int i = 0; i < reads; ++i) {
                    cs::rcu_reader guard(domain);
                    const config* p = current.load();
                    cs::check(p && p->checksum == p->version + p->payload, "RCU torn snapshot");
                    ++valid;
                }
                return valid;
            }));
        }
        for (int v = 1; v <= updates; ++v) {
            if (v == 9) lab::inject(fault, lab::failure::before_allocation);
            auto fresh = std::make_unique<config>(v, counts);
            if (v == 9) lab::inject(fault, lab::failure::before_publish);
            current.exchange(fresh.release())->retire({}, domain);
            if (v % 16 == 0) cs::rcu_barrier(domain);
        }
        int valid = 0;
        for (auto& task : tasks) valid += task.get();
        current.exchange(nullptr)->retire({}, domain);
        finish();
        cs::check(valid == readers * reads, "RCU must validate EVERY read");
        cs::check(domain.pending() == 0, "RCU callbacks remain");
        cs::check(config::destroyed.load() - before == updates + 1, "RCU exact destruction");
    } catch (...) {
        finish();
        throw;
    }
    std::cout << "RCU: 8000/8000 consistent reads, 65 destructions PASS\n";
}
} // namespace rcu_exercise
