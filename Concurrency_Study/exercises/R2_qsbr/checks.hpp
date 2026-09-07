#pragma once
#include "student.hpp"
#include "concurrency_study/exercise_check.hpp"
#include <future>
#include <iostream>
#include <memory>

namespace student_checks {
inline void preflight() {
    namespace lab = reclamation_experiment;
    lab::audit counts;
    cs::epoch_domain domain;
    std::atomic<student::value*> source{new student::value(42, counts)};
    const auto finish = [&] { delete source.exchange(nullptr); domain.barrier(); };
    try {
        {
            auto participant = student::online(domain);
            cs::check(student::read(source) == 42, "student read must copy payload");
            auto fresh = std::make_unique<student::value>(43, counts);
            domain.retire(source.exchange(fresh.release())); // 写者侧已给出，本题重点是报告。
            cs::check(domain.collect() == 0 && counts.destroyed == 0, "online must block old generation");
            student::quiescent(participant);
            cs::check(domain.collect() == 1 && counts.destroyed == 1, "quiescent must change reclamation");
            student::offline(participant);
            auto next = std::make_unique<student::value>(44, counts);
            domain.retire(source.exchange(next.release()));
            cs::check(domain.collect() == 1 && counts.destroyed == 2, "offline must unblock new retirement");
        }
        student::shutdown(source, domain);
        cs::check(!source.load() && counts.destroyed == 3 && domain.pending() == 0,
                  "student shutdown must finish all callbacks");
    } catch (...) { finish(); throw; } // participant 先析构下线，再运行 finish。
    finish();
}

inline void run() {
    preflight();
    std::cout << "preflight PASS; starting student workers\n";
    namespace lab = reclamation_experiment;
    lab::audit counts;
    cs::epoch_domain domain;
    std::atomic<student::value*> source{nullptr};
    lab::signal entered, reported;
    auto ready = entered.get_future();
    auto done_reporting = reported.get_future();
    lab::gate report, leave;
    std::future<void> reader;
    const auto finish = [&] {
        report.open(); leave.open(); // 所有控制门都打开以后才能 wait。
        if (reader.valid()) reader.wait();
        delete source.exchange(nullptr);
        domain.barrier();
    };
    try {
        source.store(new student::value(42, counts));
        reader = std::async(std::launch::async,
            [&, started = std::move(entered), reported_signal = std::move(reported)]() mutable {
                try {
                    auto participant = student::online(domain);
                    const int copied = student::read(source);
                    started.ready();
                    report.wait();
                    student::quiescent(participant);
                    reported_signal.ready();
                    leave.wait();
                    student::offline(participant);
                    cs::check(copied == 42, "student read lost copied value");
                } catch (...) {
                    const auto error = std::current_exception();
                    started.fail(error); reported_signal.fail(error);
                    throw;
                }
            });
        ready.get();
        for (int i = 0; i < 64; ++i) {
            auto fresh = std::make_unique<student::value>(i, counts);
            domain.retire(source.exchange(fresh.release()));
        }
        cs::check(domain.collect() == 0 && domain.pending() == 64 && counts.destroyed == 0,
                  "unreported student participant must retain 64");
        report.open();
        done_reporting.get();
        cs::check(domain.collect() == 64 && counts.destroyed == 64,
                  "student report must release the old generation while online");
        leave.open();
        reader.get();
        student::shutdown(source, domain);
        cs::check(!source.load() && counts.destroyed == 65 && domain.pending() == 0,
                  "student final drain incomplete");
    } catch (...) { finish(); throw; }
    finish();
    std::cout << "R2 student PASS: online, copied read, explicit quiescent/offline, final 65\n";
}
} // namespace student_checks
