#pragma once
#include "concurrency_study/epoch.hpp"
#include "concurrency_study/exercise_check.hpp"
#include "../../topics/reclamation/experiment_support.hpp"
#include <atomic>
#include <future>
#include <iostream>
#include <memory>

namespace ebr_exercise {
inline void run(reclamation_experiment::failure fault = reclamation_experiment::failure::none,
                reclamation_experiment::audit* observed = nullptr) {
    namespace lab = reclamation_experiment;
    lab::audit local;
    auto& counts = observed ? *observed : local;
    cs::epoch_domain domain;
    std::atomic<lab::value*> source{nullptr};
    lab::signal entered;
    auto ready = entered.get_future();
    lab::gate release;
    std::future<void> reader;
    const auto finish = [&] {
        release.open(); // 必须先放行，再等待；重复调用也安全。
        if (reader.valid()) reader.wait();
        delete source.exchange(nullptr); // 已 join，仍发布的版本由本实验收回。
        domain.barrier(); // 退休记录仍由域执行，绝不再次直接 delete。
    };
    try {
        source.store(new lab::value(42, counts));
        reader = std::async(std::launch::async, [&, started = std::move(entered)]() mutable {
            lab::worker_lifetime lifetime(counts);
            try {
                lab::inject(fault, lab::failure::before_registration);
                cs::epoch_guard outer(domain);
                auto* borrowed = source.load();
                { cs::epoch_guard nested(domain); } // 内层退出不撤销外层保护。
                started.ready();
                release.wait();
                cs::check(borrowed->payload == 42, "EBR protected value");
            } catch (...) {
                started.fail(std::current_exception());
                throw;
            }
        });
        ready.get(); // 注册失败立即重抛原异常，不能只 wait。
        // 第 9 次分配前注入：读者已停住，已有 8 份退休记录和 1 份当前值。
        for (int i = 0; i < 64; ++i) {
            if (i == 8) lab::inject(fault, lab::failure::before_allocation);
            auto fresh = std::make_unique<lab::value>(i, counts);
            if (i == 8) lab::inject(fault, lab::failure::before_publish);
            domain.retire(source.exchange(fresh.release()));
        }
        const auto reclaimed_while_reading = domain.collect();
        const auto backlog = domain.pending();
        const int early = counts.destroyed.load();
        release.open();
        reader.get();
        const auto reclaimed_after_exit = domain.collect();
        domain.retire(source.exchange(nullptr));
        finish();
        cs::check(reclaimed_while_reading == 0 && early == 0, "EBR reclaimed too early");
        cs::check(backlog == 64, "EBR bounded backlog");
        cs::check(reclaimed_after_exit == 64 && counts.destroyed == 65, "EBR exact reclamation");
        cs::check(domain.pending() == 0, "EBR final drain");
    } catch (...) {
        finish(); // 放行、收束、清理之后，保留当前正在传播的原异常。
        throw;
    }
    std::cout << "EBR: long reader retained 64 objects (" << 64 * sizeof(int)
              << " payload bytes); exit reclaimed 64; final total 65 PASS\n";
}
} // namespace ebr_exercise
