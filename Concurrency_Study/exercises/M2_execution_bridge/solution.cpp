#include "concurrency_study/exercise_check.hpp"
#include <iostream>
#if CS_HAS_STDEXEC
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
#include <atomic>
#include <tuple>

// A typed stopped completion: advertises one possible value signature, sends
// stopped at runtime. This also satisfies N5050 sync_wait's value-type constraint.
struct stopped_int {
    using sender_concept = stdexec::sender_tag;
    using completion_signatures = stdexec::completion_signatures<
        stdexec::set_value_t(int), stdexec::set_stopped_t()>;
    template<class R> struct operation {
        using operation_state_concept = stdexec::operation_state_tag;
        R receiver;
        void start() noexcept { stdexec::set_stopped(std::move(receiver)); }
    };
    template<class R> operation<R> connect(R receiver) const { return {std::move(receiver)}; }
};

int main() try {
    exec::static_thread_pool pool(2);
    auto scheduler = pool.get_scheduler();
    auto pipeline = stdexec::schedule(scheduler) | stdexec::then([] { return 21; })
        | stdexec::then([](int value) { return value * 2; });
    auto value = stdexec::sync_wait(std::move(pipeline));
    cs::check(value && std::get<0>(*value) == 42, "value completion");

    std::atomic<int> completed{0};
    auto branch = [&](int n) {
        return stdexec::schedule(scheduler) | stdexec::then([&, n] { ++completed; return n; });
    };
    auto joined = stdexec::when_all(branch(100), branch(23))
        | stdexec::then([](int a, int b) { return a + b; });
    auto result = stdexec::sync_wait(std::move(joined));
    cs::check(result && std::get<0>(*result) == 123 && completed == 2, "join waits for both values");
    bool caught = false;
    auto error = stdexec::when_all(branch(1), stdexec::schedule(scheduler) |
        stdexec::then([]() -> int { throw std::runtime_error("pipeline-error"); }));
    try { (void)stdexec::sync_wait(std::move(error)); }
    catch (const std::runtime_error& e) { caught = std::string_view(e.what()) == "pipeline-error"; }
    cs::check(caught, "when_all error crosses sync_wait");

    bool then_ran = false;
    auto stopped = stopped_int{} | stdexec::then([&](int n) { then_ran = true; return n; });
    auto empty = stdexec::sync_wait(std::move(stopped));
    cs::check(!empty && !then_ran, "stopped bypasses then and returns disengaged optional");
    auto stopped_join = stdexec::sync_wait(stdexec::when_all(stopped_int{}, branch(2)));
    cs::check(!stopped_join, "when_all propagates stopped after joining children");
    auto recovered = stdexec::sync_wait(stopped_int{} | stdexec::upon_stopped([] { return 7; }));
    cs::check(recovered && std::get<0>(*recovered) == 7, "explicit stopped-to-value adaptation");
    std::cout << "M2 pinned stdexec OK: value/error/stopped/when_all/recovery\n";
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
#else
int main() { std::cerr << "SKIP: CS_HAS_STDEXEC=0; enable pinned nvhpc-26.05 dependency\n"; return 77; }
#endif
