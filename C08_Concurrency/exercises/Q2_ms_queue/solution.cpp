#include "concurrency_study/queue_checks.hpp"
#include "concurrency_study/queue_linked.hpp"
#include <chrono>
#include <iostream>

struct payload {
    inline static std::atomic<int> live{0};
    inline static std::atomic<int> destroyed10{0};
    std::size_t id = 0;
    payload() { ++live; }
    payload(const payload& other) : id(other.id) { ++live; }
    payload& operator=(const payload& other) noexcept { id = other.id; return *this; }
    ~payload() { if (id == 10) ++destroyed10; --live; }
};

int main() {
    using namespace cs::queue_lab;
    check_const_copy([] { return ms_queue<copy_overload_probe>{}; });
    {
        ms_queue<std::size_t> q;
        std::size_t value = 99;
        cs::check(!q.try_pop(value) && value == 99, "empty preserves output");
        cs::check(q.try_push(10) && q.try_push(20), "enqueue");
        cs::check(q.try_pop(value) && value == 10, "FIFO first");
        cs::check(q.try_pop(value) && value == 20, "FIFO second");
        check_producer_order(transfer(q, 12007, 4, 1), 12007, 4);
        transfer(q, 12007, 1, 4); // SPMC
        transfer(q, 24007, 4, 4); // triggers concurrent retirement/cleanup
        cs::check(!q.try_pop(value), "no extra nodes");
        std::cout << "MS pointer atomics lock-free=" << q.atomics_lock_free()
                  << "; whole operation may allocate and acquire HP locks\n";
    }
    {
        ms_queue<payload> q;
        payload input, slow_output, fast_output;
        input.id = 10; q.try_push(input);
        input.id = 20; q.try_push(input);
        input.id = 0;
        payload::destroyed10.store(0);
        std::atomic<bool> paused{false}, resume{false};
        auto slow = std::async(std::launch::async, [&] {
            return q.try_pop(slow_output, [&]() noexcept {
                paused.store(true, std::memory_order_release);
                while (!resume.load(std::memory_order_acquire)) std::this_thread::yield();
            });
        });
        while (!paused.load(std::memory_order_acquire)) {
            if (slow.wait_for(std::chrono::seconds{0}) == std::future_status::ready) {
                slow.get(); // propagate even a failure before the pause hook
                cs::check(false, "worker ended before expected pause");
            }
            std::this_thread::yield();
        }
        bool fast = false;
        int destroyed = -1;
        try {
            fast = q.try_pop(fast_output); // retires node containing slow reader's 10
            cs::hazard_pointer_cleanup();
            destroyed = payload::destroyed10.load();
        } catch (...) {
            resume.store(true, std::memory_order_release);
            slow.wait();
            throw;
        }
        resume.store(true, std::memory_order_release);
        cs::check(slow.get() && fast && slow_output.id == 10 && fast_output.id == 20,
                  "overlapping pops return their own immutable payload");
        cs::check(destroyed == 0, "next HP delays reclamation while winner is paused");
        cs::hazard_pointer_cleanup();
        cs::check(payload::destroyed10.load() == 1, "protected retired payload later reclaimed");
    }
    {
        ms_queue<payload> q;
        payload in, out;
        for (std::size_t i = 0; i < 1000; ++i) {
            in.id = i;
            cs::check(q.try_push(in) && q.try_pop(out) && out.id == i, "nontrivial payload");
        }
        cs::hazard_pointer_cleanup();
        cs::check(payload::live.load() == 3, "only input/output/current dummy payload remain");
        in.id = 1001;
        q.try_push(in); // destructor also releases a nonempty live chain
    }
    cs::hazard_pointer_cleanup();
    cs::check(payload::live.load() == 0, "all payloads including retired nodes destroyed");
    std::cout << "Q2 OK: FIFO, MPSC/SPMC/MPMC IDs, in-run reclamation and final drain\n";
}
