#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <new>

static std::atomic<bool> fail_next_allocation{false};
void* operator new(std::size_t size) {
    if (fail_next_allocation.exchange(false)) throw std::bad_alloc{};
    if (void* p = std::malloc(size ? size : 1)) return p;
    throw std::bad_alloc{};
}
void operator delete(void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }

#include "mini_ref/mini.hpp"

mini_ref::task<void> work(int& starts) { ++starts; co_return; }

int main() {
    mini_ref::task_scope scope;
    int starts = 0;
    auto rejected = work(starts);
    fail_next_allocation = true;
    bool caught = false;
    try { scope.spawn(std::move(rejected)); }
    catch (const std::bad_alloc&) { caught = true; }
    std::fprintf(stderr, "caught=%d in_flight=%d starts=%d; waiting for drain\n",
                 caught, scope.in_flight(), starts);
    scope.wait_empty();
    if (!caught || starts != 0 || scope.in_flight() != 0) return 1;
    scope.spawn(work(starts));
    scope.wait_empty();
    if (starts != 1 || scope.in_flight() != 0) return 1;
    std::puts("rejected work rolled back; accepted work drained");
}
