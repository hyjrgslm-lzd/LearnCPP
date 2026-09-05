#ifndef COROUTINE_STUDY_RUNTIME_TEST_CHECK_HPP
#define COROUTINE_STUDY_RUNTIME_TEST_CHECK_HPP

#include <cstdlib>

inline void check(bool ok) {
    if (!ok) std::abort();
}

#endif
