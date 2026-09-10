#include "async_logging_submission.hpp"
#include "checks.hpp"

#include <exception>
#include <iostream>

int main() {
    if (!async_logging_submission::complete) {
        std::cerr << "STARTER INCOMPLETE: U01 Part 1-4 未完成；未启动 async logger。\n";
        return 1;
    }
    try {
        u01::run_all<async_logging_submission>();
        std::cout << "U01_async_logging student OK\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "check failed: " << e.what() << '\n';
        return 1;
    }
}
