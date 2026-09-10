#include "async_logging_submission.hpp"
#include "checks.hpp"

#include <exception>
#include <iostream>

int main() {
    try {
        u01::run_all<async_logging_submission>();
        std::cout << "U01_async_logging_reference OK\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "check failed: " << e.what() << '\n';
        return 1;
    }
}
