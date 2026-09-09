#ifndef COROUTINE_STUDY_EXERCISE_CHECK_HPP
#define COROUTINE_STUDY_EXERCISE_CHECK_HPP

#include <stdexcept>
#include <string>
#include <string_view>

namespace coroutine_study {

inline void check(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

} // namespace coroutine_study

#endif // COROUTINE_STUDY_EXERCISE_CHECK_HPP
