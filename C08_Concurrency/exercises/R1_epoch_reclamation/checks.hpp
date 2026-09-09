#pragma once
#include "student.hpp"
#include "concurrency_study/exercise_check.hpp"
#include <future>
#include <iostream>

namespace student_checks {
inline void preflight() {
    namespace lab = reclamation_experiment;
    lab::audit counts;
    cs::epoch_domain domain;
    std::atomic<student::value*> source{new student::value(42, counts)};
    const auto finish = [&] { delete source.exchange(nullptr); domain.barrier(); };
    try {
        {
            auto outer = student::pin(domain);
            { auto nested = student::pin(domain); }
            auto fresh = std::make_unique<student::value>(43, counts);
            student::retire(std::unique_ptr<student::value>{source.exchange(fresh.release())}, domain);
            cs::check(domain.pending() == 1 && counts.destroyed == 0, "student retire ownership");
            cs::check(student::collect(domain) == 0 && counts.destroyed == 0,
                      "student pin must protect until outer guard ends");
        }
        cs::check(student::collect(domain) == 1 && counts.destroyed == 1,
                  "student collect must reclaim after unpin");
        student::shutdown(source, domain);
        cs::check(!source.load() && domain.pending() == 0 && counts.destroyed == 2,
                  "student shutdown must reclaim current value");
    } catch (...) { finish(); throw; } // guard 已经展开，finish 不在读区等待。
    finish();
}

inline void run() {
    preflight();
    std::cout << "preflight PASS; starting student workers\n";
    namespace lab = reclamation_experiment;
    lab::audit counts;
    cs::epoch_domain domain;
    std::atomic<student::value*> source{nullptr};
    lab::signal entered;
    auto ready = entered.get_future();
    lab::gate release;
    std::future<void> reader;
    const auto finish = [&] {
        release.open();
        if (reader.valid()) reader.wait();
        delete source.exchange(nullptr);
        domain.barrier();
    };
    try {
        source.store(new student::value(42, counts));
        reader = std::async(std::launch::async, [&, started = std::move(entered)]() mutable {
            try {
                auto outer = student::pin(domain);
                auto* borrowed = source.load();
                { auto nested = student::pin(domain); }
                started.ready();
                release.wait();
                cs::check(borrowed->payload == 42, "student long reader lost its value");
            } catch (...) { started.fail(std::current_exception()); throw; }
        });
        ready.get();
        for (int i = 0; i < 64; ++i) {
            auto fresh = std::make_unique<student::value>(i, counts);
            student::retire(std::unique_ptr<student::value>{source.exchange(fresh.release())}, domain);
        }
        cs::check(student::collect(domain) == 0 && counts.destroyed == 0 && domain.pending() == 64,
                  "student long reader must retain 64 objects");
        release.open();
        reader.get();
        cs::check(student::collect(domain) == 64 && counts.destroyed == 64,
                  "student unpin must enable reclamation");
        student::shutdown(source, domain);
        cs::check(!source.load() && counts.destroyed == 65 && domain.pending() == 0,
                  "student final drain incomplete");
    } catch (...) { finish(); throw; }
    finish();
    std::cout << "R1 student PASS: nested pin, 64 retained/reclaimed, final 65\n";
}
} // namespace student_checks
