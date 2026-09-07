#ifndef CONCURRENCY_STUDY_EXERCISE_CHECK_HPP
#define CONCURRENCY_STUDY_EXERCISE_CHECK_HPP

#include <stdexcept>
#include <string>
#include <string_view>

namespace cs {

inline void check(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

} // namespace cs
#endif
