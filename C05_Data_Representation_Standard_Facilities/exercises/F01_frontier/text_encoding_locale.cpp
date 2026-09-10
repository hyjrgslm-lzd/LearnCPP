#include <iostream>
#include <locale>
#include <string_view>
#include <version>

#if __has_include(<text_encoding>)
#include <text_encoding>
#define C05_HAS_TEXT_ENCODING_HEADER 1
#else
#define C05_HAS_TEXT_ENCODING_HEADER 0
#endif

int main() {
#if !C05_HAS_TEXT_ENCODING_HEADER
    std::cout << "SKIP <text_encoding> is not available\n";
    return 77;
#elif !defined(__cpp_lib_text_encoding) || __cpp_lib_text_encoding < 202306L
    std::cout << "SKIP __cpp_lib_text_encoding is missing or below 202306L\n";
    return 77;
#else
    const std::text_encoding utf8{"utf8"};
    if (utf8.mib() != std::text_encoding::UTF8 || std::string_view{utf8.name()}.empty()) {
        std::cerr << "FAIL text_encoding does not recognize utf8\n";
        return 1;
    }

    const std::text_encoding literal = std::text_encoding::literal();
    const std::text_encoding env     = std::text_encoding::environment();
    const std::text_encoding loc     = std::locale("").encoding();
    if (literal.name() == nullptr || env.name() == nullptr || loc.name() == nullptr) {
        std::cerr << "FAIL encoding names must be non-null\n";
        return 1;
    }

    std::cout << "PASS text_encoding literal=" << literal.name() << " environment=" << env.name()
              << " locale=" << loc.name() << '\n';
    return 0;
#endif
}
