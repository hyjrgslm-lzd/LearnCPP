#pragma once
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace c10 {
struct unfinished : std::runtime_error {
  using std::runtime_error::runtime_error;
};
struct skip : std::runtime_error {
  using std::runtime_error::runtime_error;
};
inline void require(bool condition, std::string_view message) {
  if (!condition)
    throw std::runtime_error(std::string(message));
}
template <class F> int test_main(F &&check) {
  try {
    std::forward<F>(check)();
    std::cout << "PASS: all requested checks completed\n";
    return 0;
  } catch (const unfinished &error) {
    std::cerr << "UNFINISHED: " << error.what() << '\n';
    return 2;
  } catch (const skip &error) {
    std::cout << "SKIP: " << error.what() << '\n';
    return 77;
  } catch (const std::exception &error) {
    std::cerr << "FAIL: " << error.what() << '\n';
    return 1;
  } catch (...) {
    std::cerr << "FAIL: non-standard exception\n";
    return 1;
  }
}
} // namespace c10
