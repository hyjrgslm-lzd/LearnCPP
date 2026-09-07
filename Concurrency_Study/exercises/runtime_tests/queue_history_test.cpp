#include "concurrency_study/queue_baseline.hpp"
#include "concurrency_study/queue_checks.hpp"
#include "concurrency_study/queue_linked.hpp"
#include <deque>
#include <future>
#include <iostream>

namespace {
struct operation {
    unsigned begin, end;
    bool push, success;
    std::size_t value;
};
// Exhaustive search of a SMALL completed history. Real-time predecessors must
// be placed first. capacity=0 means unbounded; relaxed_false accepts no-op false.
bool legal(const std::vector<operation>& h, std::size_t capacity,
           bool relaxed_false = false, bool lifo = false,
           unsigned used = 0, std::deque<std::size_t> state = {}) {
    if (used == (1u << h.size()) - 1) return true;
    for (std::size_t i = 0; i < h.size(); ++i) {
        if (used & (1u << i)) continue;
        bool blocked = false;
        for (std::size_t j = 0; j < h.size(); ++j)
            if (!(used & (1u << j)) && h[j].end < h[i].begin) blocked = true;
        if (blocked) continue;
        auto next = state;
        const auto& op = h[i];
        if (op.success) {
            if (op.push) {
                if (capacity && next.size() == capacity) continue;
                next.push_back(op.value);
            } else {
                if (next.empty() || (lifo ? next.back() : next.front()) != op.value) continue;
                if (lifo) next.pop_back(); else next.pop_front();
            }
        } else if (!relaxed_false) {
            if (op.push ? (!capacity || next.size() < capacity) : !next.empty()) continue;
        }
        if (legal(h, capacity, relaxed_false, lifo, used | (1u << i), next)) return true;
    }
    return false;
}

template<class Queue>
void history(Queue& q, std::size_t capacity, bool relaxed_false, bool lifo = false,
             bool single_producer = false, bool single_consumer = false) {
    std::atomic<unsigned> clock{0};
    std::vector<std::future<std::vector<operation>>> tasks;
    for (unsigned t = 0; t < 3; ++t) {
        tasks.push_back(std::async(std::launch::async, [&, t] {
            std::vector<operation> local;
            for (unsigned round = 0; round < 2; ++round) {
                auto call = [&](bool push) {
                    std::size_t value = t * 10 + round;
                    const auto begin = clock.fetch_add(1);
                    const bool success = push ? q.try_push(value) : q.try_pop(value);
                    const auto end = clock.fetch_add(1);
                    local.push_back({begin, end, push, success, value});
                };
                if (!single_producer || t == 0) call(true);
                if (!single_consumer || t == 2) call(false);
            }
            return local;
        }));
    }
    std::vector<operation> h;
    for (auto& task : tasks) {
        auto part = task.get();
        h.insert(h.end(), part.begin(), part.end());
    }
    cs::check(h.size() < 16 && legal(h, capacity, relaxed_false, lifo), "recorded invocation/response history");
}
}

int main() {
    using namespace cs::queue_lab;
    // First test the oracle itself with a legal and an impossible fixed history.
    cs::check(legal({{0,1,true,true,10},{2,3,false,true,10}}, 2), "oracle positive");
    cs::check(!legal({{0,1,true,true,10},{2,3,false,true,20}}, 2), "oracle rejects wrong ID");
    // P0 reserves first and publishes last; P1 completes; pop returns false.
    const std::vector<operation> gap{{0,5,true,true,10},{1,2,true,true,20},{3,4,false,false,99}};
    cs::check(!legal(gap, 4) && legal(gap, 4, true), "strict FIFO differs from transient-failure contract");
    for (int repeat = 0; repeat < 20; ++repeat) {
        mutex_queue<std::size_t> mutex(2);
        mutex_ring<std::size_t> ring(2);
        spsc_ring<std::size_t> spsc(2);
        spsc_ring<std::size_t, true> cached(2);
        mpsc_ring<std::size_t> mpsc(2);
        mpmc_ring<std::size_t> mpmc(2);
        ms_queue<std::size_t> ms;
        treiber_stack<std::size_t> stack;
        history(mutex, 2, false);
        history(ring, 2, false);
        history(spsc, 2, true, false, true, true);
        history(cached, 2, true, false, true, true);
        history(mpsc, 2, true, false, false, true);
        history(mpmc, 2, true);
        history(ms, 0, false);
        history(stack, 0, false, true);
    }
    check_reservation_gap<mpsc_ring<std::size_t>>();
    check_reservation_gap<mpmc_ring<std::size_t>>();
    check_narrow_wrap();
    std::cout << "queue_history OK: actual small histories, oracle counterexamples, gap/wrap models\n";
}
