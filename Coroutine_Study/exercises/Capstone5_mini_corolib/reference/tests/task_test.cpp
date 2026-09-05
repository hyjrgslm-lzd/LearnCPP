#include "mini_ref/mini.hpp"

#include <cstdlib>
#include <memory>
#include <stdexcept>
#include <type_traits>

static mini_ref::task<int> compute() {
    co_return 42;
}

static mini_ref::task<std::unique_ptr<int>> owned_value() {
    co_return std::make_unique<int>(7);
}

static mini_ref::task<void> no_value() { co_return; }

static mini_ref::task<void> consume_once() {
    auto source = owned_value();
    auto value = co_await source;
    if (!value || *value != 7) std::abort();
    bool caught = false;
    try { (void)co_await source; }
    catch (const std::logic_error&) { caught = true; }
    if (!caught) std::abort();

    auto empty = no_value();
    co_await empty;
    caught = false;
    try { co_await empty; }
    catch (const std::logic_error&) { caught = true; }
    if (!caught) std::abort();
}

int main() {
    static_assert(!std::is_copy_constructible_v<mini_ref::task<int>>);
    static_assert(std::is_same_v<
                  mini_ref::task<int>::completion_signatures,
                  mini_ref::completion_signatures<int>>);
    auto result = mini_ref::sync_wait(compute());
    if (!result || std::get<0>(*result) != 42) std::abort();

    auto t = compute();
    t.start();
    bool caught = false;
    try {
        t.start();
    } catch (const std::logic_error&) {
        caught = true;
    }
    if (!caught) std::abort();
    if (t.await_resume() != 42) std::abort();
    caught = false;
    try { (void)t.await_resume(); }
    catch (const std::logic_error&) { caught = true; }
    if (!caught) std::abort();

    (void)mini_ref::sync_wait(consume_once());
}
