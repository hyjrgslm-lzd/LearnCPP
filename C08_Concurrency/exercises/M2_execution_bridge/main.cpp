#include "concurrency_study/exercise_check.hpp"
#include <iostream>
#if CS_HAS_STDEXEC
#include <stdexec/execution.hpp>
#include <exec/static_thread_pool.hpp>
#include <tuple>

int main() try {
    std::cout << "OBSERVATION: value pipeline only; when_all/error/stopped Parts require separate work and Reference checks.\n";
    exec::static_thread_pool pool(2);
    auto pipeline = stdexec::schedule(pool.get_scheduler())
        | stdexec::then([] { return 21; })
        | stdexec::then([](int value) { return value * 2; });
    auto result = stdexec::sync_wait(std::move(pipeline));
    cs::check(result && std::get<0>(*result) == 42, "observed value pipeline");
    std::cout << "42; this check does not complete all Parts.\n";
} catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
#else
int main() { std::cerr << "SKIP: CS_HAS_STDEXEC=0; pinned nvhpc-26.05 dependency unavailable\n"; return 77; }
#endif
