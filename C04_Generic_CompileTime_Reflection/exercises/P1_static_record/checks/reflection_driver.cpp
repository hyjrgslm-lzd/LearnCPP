#include <iostream>
#if __has_include(<meta>)
#define C04_RECORD_HAS_META 1
#else
#define C04_RECORD_HAS_META 0
#endif
#if C04_RECORD_HAS_META && defined(__cpp_impl_reflection) && __cpp_impl_reflection >= 202506L && defined(__cpp_expansion_statements) && __cpp_expansion_statements >= 202506L
#define C04_RECORD_ANNOTATIONS 1
#include <record_ops.hpp>
#include "record_checks.hpp"
int main() {
    c04_record::checks::run<c04_record::implementation>();
    std::cout << "P1 real reflection passed the same field contract\n";
}
#else
int main() {
    std::cout << "SKIP P1 real reflection: meta header=" << C04_RECORD_HAS_META;
#ifdef __cpp_impl_reflection
    std::cout << " reflection=" << __cpp_impl_reflection;
#else
    std::cout << " reflection=absent";
#endif
#ifdef __cpp_expansion_statements
    std::cout << " expansion=" << __cpp_expansion_statements;
#else
    std::cout << " expansion=absent";
#endif
    std::cout << "; real backend not compiled or run\n";
    return 77;
}
#endif
