#include "std_task_probe_config.hpp"

#if H2_HAS_STD_EXECUTION_TASK
#include <execution>
#include <thread>
#include <tuple>
#endif

#include <cstdio>

#if H2_HAS_STD_EXECUTION_TASK
std::execution::task<int> h2_std_execution_task_probe()
{
    co_return 1;
}
#endif

int main()
{
#if !H2_HAS_CXX26_PREVIEW_FLAG
    std::puts("SKIP: C++26 preview flag unavailable for std::execution::task probe.");
    return 77;
#elif !H2_HAS_STD_EXECUTION_TASK
    std::puts("SKIP: std::execution::task coroutine body + sync_wait did not compile in the C++26 preview probe.");
    return 77;
#else
    auto result = std::this_thread::sync_wait(h2_std_execution_task_probe());
    if (!result || std::get<0>(*result) != 1) {
        std::puts("FAIL: std::execution::task coroutine body compiled but produced wrong result.");
        return 1;
    }
    std::puts("PASS: std::execution::task coroutine body + sync_wait compiled, linked, and ran.");
    return 0;
#endif
}
