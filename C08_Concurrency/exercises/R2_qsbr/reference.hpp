#pragma once
#include "concurrency_study/epoch.hpp"
#include "concurrency_study/exercise_check.hpp"
#include "../../topics/reclamation/experiment_support.hpp"
#include <atomic>
#include <future>
#include <iostream>
#include <memory>

namespace qsbr_exercise {
inline void run(reclamation_experiment::failure fault = reclamation_experiment::failure::none,
                reclamation_experiment::audit* observed = nullptr) {
    namespace lab = reclamation_experiment;
    lab::audit local;
    auto& counts = observed ? *observed : local;
    cs::epoch_domain domain;
    std::atomic<lab::value*> source{nullptr};
    lab::signal entered, reported;
    auto ready = entered.get_future();
    auto report_done = reported.get_future();
    lab::gate report, leave;
    std::future<void> worker;
    const auto finish = [&] {
        report.open();
        leave.open(); // 两道门全部放行，才可以 wait worker。
        if (worker.valid()) worker.wait();
        delete source.exchange(nullptr);
        domain.barrier();
    };
    try {
        source.store(new lab::value(42, counts));
        worker = std::async(std::launch::async,
            [&, started = std::move(entered), completed = std::move(reported)]() mutable {
                lab::worker_lifetime lifetime(counts);
                try {
                    lab::inject(fault, lab::failure::before_registration);
                    cs::qsbr_participant participant(domain);
                    auto* borrowed = source.load();
                    const int value = borrowed->payload;
                    borrowed = nullptr; // 停止全部旧借用，但还没有报告静默。
                    started.ready();
                    report.wait();
                    lab::inject(fault, lab::failure::before_report);
                    participant.quiescent();
                    completed.ready();
                    leave.wait();
                    participant.offline();
                    cs::check(value == 42, "QSBR copied value");
                } catch (...) {
                    const auto error = std::current_exception();
                    started.fail(error);
                    completed.fail(error); // 启动或报告阶段失败都要通知对应 get。
                    throw;
                }
            });
        ready.get();
        for (int i = 0; i < 64; ++i) {
            if (i == 8) lab::inject(fault, lab::failure::before_allocation);
            auto fresh = std::make_unique<lab::value>(i, counts);
            if (i == 8) lab::inject(fault, lab::failure::before_publish);
            domain.retire(source.exchange(fresh.release()));
        }
        const auto before_report = domain.collect();
        const auto backlog = domain.pending();
        report.open();
        report_done.get();
        const auto after_report = domain.collect();
        leave.open();
        worker.get();
        domain.retire(source.exchange(nullptr));
        finish();
        cs::check(before_report == 0 && backlog == 64, "QSBR missing report must retain batch");
        cs::check(after_report == 64, "QSBR report should release old generation");
        cs::check(counts.destroyed == 65 && domain.pending() == 0, "QSBR final exact drain");
    } catch (...) {
        finish();
        throw;
    }
    std::cout << "QSBR: no report retains 64; quiescent releases 64 while online; total 65 PASS\n";
}
} // namespace qsbr_exercise
