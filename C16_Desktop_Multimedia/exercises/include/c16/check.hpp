#pragma once
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace c16 {
inline void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error("check failed: " + std::string(message));
}
template<class F> int run(F&& body) {
    try {
        std::forward<F>(body)();
        std::cout << "PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    } catch (...) {
        std::cerr << "unexpected non-standard exception\n";
        return 2;
    }
}
}
