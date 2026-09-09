#pragma once
// 契约检查只调用模板参数 Channel，不包含 Reference 实现。
#include "concurrency_study/exercise_check.hpp"
#include <algorithm>
#include <array>
#include <future>
#include <iostream>
#include <memory>

#include <optional>
#include <stdexcept>
#include <vector>

namespace channel_checks {
template<template<class> class Channel>
void sequential() {
    bool invalid = false;
    try { Channel<int> q(0); } catch (const std::invalid_argument&) { invalid = true; }
    cs::check(invalid, "zero capacity rejected");
    Channel<std::unique_ptr<int>> q(2);
    cs::check(q.push(std::make_unique<int>(10)), "move-only first");
    cs::check(q.push(std::make_unique<int>(20)), "move-only second");
    auto first = q.pop();
    cs::check(first && **first == 10, "FIFO first");
    cs::check(q.push(std::make_unique<int>(30)), "ring slot reused");
    q.close(); q.close();
    cs::check(!q.push(std::make_unique<int>(40)), "closed push fails");
    auto second = q.pop(), third = q.pop();
    cs::check(second && **second == 20 && third && **third == 30, "closed queue drains FIFO");
    cs::check(!q.pop(), "end-of-stream after drain");
}

template<template<class> class Channel>
void concurrent(int consumers) {
    Channel<int> q(1);
    constexpr int producers = 3, per_producer = 200;
    std::vector<std::future<void>> writers;
    std::vector<std::future<std::vector<int>>> readers;
    writers.reserve(producers); readers.reserve(consumers);
    try {
        for (int i = 0; i < consumers; ++i)
            readers.push_back(std::async(std::launch::async, [&] {
                try {
                    std::vector<int> result;
                    while (auto item = q.pop()) result.push_back(*item);
                    return result;
                } catch (...) { q.close(); throw; }
            }));
        for (int p = 0; p < producers; ++p)
            writers.push_back(std::async(std::launch::async, [&, p] {
                try {
                    for (int i = 0; i < per_producer; ++i)
                        cs::check(q.push(p * per_producer + i), "producer accepted");
                } catch (...) { q.close(); throw; }
            }));
        for (auto& writer : writers) writer.get();
    } catch (...) { q.close(); throw; }
    q.close();
    std::vector<int> received;
    for (auto& reader : readers) {
        auto part = reader.get();
        received.insert(received.end(), part.begin(), part.end());
    }
    cs::check(received.size() == producers * per_producer, "total IDs");
    if (consumers == 1) {
        std::array<int, producers> next{};
        for (int id : received) {
            cs::check(id >= 0 && id < producers * per_producer, "ID range");
            cs::check(id % per_producer == next[id / per_producer]++, "per-producer order");
        }
    }
    std::sort(received.begin(), received.end());
    for (int i = 0; i < producers * per_producer; ++i)
        cs::check(received[i] == i, "no loss, duplicate, or unexpected ID");
}

template<template<class> class Channel>
void close_waiters() {
    Channel<int> full(1), empty(1);
    cs::check(full.push(1), "fill single slot");
    std::vector<std::future<bool>> producers;
    std::vector<std::future<std::optional<int>>> consumers;
    producers.reserve(3); consumers.reserve(3);
    try {
        for (int i = 0; i < 3; ++i) {
            producers.push_back(std::async(std::launch::async, [&] { return full.push(2); }));
            consumers.push_back(std::async(std::launch::async, [&] { return empty.pop(); }));
        }
    } catch (...) { full.close(); empty.close(); throw; }
    full.close(); empty.close();
    for (auto& p : producers) cs::check(!p.get(), "close rejects full-queue producers");
    for (auto& c : consumers) cs::check(!c.get(), "close releases empty-queue consumers");
    cs::check(full.pop() == 1 && !full.pop(), "close retains accepted value");
}

template<template<class> class Channel>
void run() {
    sequential<Channel>(); concurrent<Channel>(1); concurrent<Channel>(2); close_waiters<Channel>();
    std::cout << "C2 OK: capacity=1, FIFO, MPMC IDs, move-only, close/drain\n";
}
} // namespace channel_checks
