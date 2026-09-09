#include "std_task_probe_config.hpp"

#include <cstdio>

int main()
{
#if H2_HAS_STD_EXECUTION_TASK
    std::puts("std::execution::task<void> compiled in the CMake probe.");
#else
    std::puts("std::execution::task<void> did not compile in the CMake probe.");
#endif
    return 0;
}
