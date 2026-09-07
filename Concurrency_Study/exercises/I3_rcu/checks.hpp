#pragma once
#include "student.hpp"
#include "concurrency_study/exercise_check.hpp"
#include <future>
#include <iostream>
#include <vector>

namespace student_checks {
inline void preflight() {
    reclamation_experiment::audit counts;
    cs::rcu_domain domain;
    std::atomic<student::config*> source{new student::config(0, counts)};
    const auto finish = [&] { delete source.exchange(nullptr); domain.barrier(); };
    try {
        bool visited = false;
        student::read(source, domain, [&](const student::config& p) {
            visited = true;
            cs::check(p.version == 0 && p.checksum == p.version + p.payload, "student read payload");
            student::replace(source, std::make_unique<student::config>(1, counts), domain);
            cs::check(source.load() && source.load()->version == 1, "student replace source");
            cs::check(domain.pending() == 1, "student replace must retire old value");
            cs::check(domain.collect() == 0 && counts.destroyed == 0,
                      "student reader must cover use callback");
        });
        cs::check(visited, "student read must call use");
        student::checkpoint(domain);
        cs::check(counts.destroyed == 1 && domain.pending() == 0, "checkpoint must finish callback");
        student::shutdown(source, domain);
        cs::check(source.load() == nullptr && counts.destroyed == 2 && domain.pending() == 0,
                  "student shutdown must remove and reclaim final version");
    } catch (...) { finish(); throw; }
    finish();
}

inline void run() {
    preflight();
    std::cout << "preflight PASS; starting student workers\n";
    namespace lab = reclamation_experiment;
    lab::audit counts;
    cs::rcu_domain domain;
    std::atomic<student::config*> source{nullptr};
    std::vector<std::future<int>> readers;
    const auto finish = [&] {
        for (auto& r : readers) if (r.valid()) r.wait();
        delete source.exchange(nullptr);
        domain.barrier();
    };
    try {
        readers.reserve(4);
        source.store(new student::config(0, counts));
        for (int r = 0; r < 4; ++r) {
            readers.push_back(std::async(std::launch::async, [&] {
                int completed = 0;
                for (int i = 0; i < 2000; ++i) {
                    student::read(source, domain, [&](const student::config& p) {
                        cs::check(p.checksum == p.version + p.payload, "student snapshot checksum");
                        ++completed;
                    });
                }
                return completed;
            }));
        }
        for (int v = 1; v <= 64; ++v) {
            student::replace(source, std::make_unique<student::config>(v, counts), domain);
            cs::check(domain.pending() <= 16, "student update backlog exceeded budget");
            if (v % 16 == 0) student::checkpoint(domain);
        }
        int completed = 0;
        for (auto& r : readers) completed += r.get();
        student::shutdown(source, domain);
        cs::check(completed == 8000, "student must validate every read");
        cs::check(!source.load() && domain.pending() == 0 && counts.destroyed == 65,
                  "student shutdown must complete all callbacks");
    } catch (...) { finish(); throw; }
    finish();
    std::cout << "I3 student PASS: 8000 reads, 65 destructions, bounded retirement\n";
}
} // namespace student_checks
