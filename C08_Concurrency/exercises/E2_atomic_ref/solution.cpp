#include "concurrency_study/exercise_check.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <future>
#include <iostream>
#include <type_traits>

// Align EACH element, not just the first address of an int array.
struct alignas(std::atomic_ref<int>::required_alignment) cell { int value = 0; };
static_assert(std::is_trivially_copyable_v<int>);
static_assert(alignof(cell) >= std::atomic_ref<int>::required_alignment);

int main() {
    std::array<cell, 4> data{};
    for (const auto& item : data)
        cs::check(reinterpret_cast<std::uintptr_t>(&item.value) %
                  std::atomic_ref<int>::required_alignment == 0, "each element aligned");
    std::array<std::future<void>, 4> workers;
    for (auto& task : workers)
        task = std::async(std::launch::async, [&] {
            std::atomic_ref<int> ref(data[2].value);
            for (int i = 0; i < 2000; ++i) ref.fetch_add(1, std::memory_order_relaxed);
        });
    for (auto& task : workers) task.get();
    // Every worker-local atomic_ref is destroyed before these ordinary reads.
    cs::check(data[2].value == 8000, "all increments retained");
    cs::check(data[0].value == 0 && data[1].value == 0 && data[3].value == 0,
              "neighbours untouched");
    {
        std::atomic_ref<int> ref(data[2].value);
        const auto alias = ref; // const ref can modify a non-const referenced object.
        alias.store(123);
        cs::check(ref.load() == 123, "copies reference the same object");
        std::cout << "required_alignment=" << decltype(ref)::required_alignment
                  << ", alignof(int)=" << alignof(int)
                  << ", lock-free=" << ref.is_lock_free() << '\n';
    }
    cs::check(data[2].value == 123, "ordinary access resumes after all refs die");
    std::cout << "E2_reference OK: per-element alignment and access phases\n";
}
