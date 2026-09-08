#include "check.hpp"
#include <string_view>

#ifndef A1_CONFIG_NAME
#define A1_CONFIG_NAME ""
#endif
#ifndef A1_EXPECT_NDEBUG
#define A1_EXPECT_NDEBUG 0
#endif

int main()
{
    constexpr std::string_view config = A1_CONFIG_NAME;
#if defined(NDEBUG)
    constexpr bool actual_ndebug = true;
#else
    constexpr bool actual_ndebug = false;
#endif
    constexpr bool expected_ndebug = A1_EXPECT_NDEBUG != 0;

    check(__cplusplus >= 202302L || __cplusplus >= 202100L, "target is configured for a modern C++ language mode");
    check(actual_ndebug == expected_ndebug, "CMake configuration and observed NDEBUG macro agree for this target");
    if (config == "Debug") {
        check(!actual_ndebug, "Debug config keeps NDEBUG undefined");
    }
    if (config == "Release") {
        check(actual_ndebug, "Release config defines NDEBUG");
    }
}
